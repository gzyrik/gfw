#include "config.h"
#include "xmain.h"
#include "xthread.h"
#include "xmem.h"
#include "xwakeup.h"
#include "xarray.h"
#include "xslist.h"
#include "xprintf.h"
#include "xmsg.h"
#include "xatomic.h"
#include "xslice.h"
#include "xhook.h"
#include "xtest.h"
#include "xpoll.h"
#include "xlist.h"
#include "xutil.h"
#include <errno.h>
#include <string.h>
typedef struct {
    xSource *head;
    xSource *tail;
    xint    priority;
} SourceList;

typedef struct {
    xCond   *cond;
    xMutex  *mutex;
} MainWaiter;

typedef struct {
    xint    depth;
    xSList  *sources;/* dispatching sources */
} MainDispatch;
typedef struct _PollRecord PollRecord;
struct _PollRecord
{
    xPollFd     *fd;
    PollRecord  *prev;
    PollRecord  *next;
    xint        priority;
};
#define LOCK_CONTEXT(context) x_mutex_lock (&context->mutex)
#define UNLOCK_CONTEXT(context) x_mutex_unlock (&context->mutex)

#define SOURCE_DESTROYED(source) (((source)->flags & X_HOOK_FLAG_ACTIVE) == 0)
#define SOURCE_BLOCKED(source) (((source)->flags & X_SOURCE_BLOCKED) != 0)
#define SOURCE_UNREF(source, context)  X_STMT_START {                   \
    if ((source)->ref_count > 1)                                        \
    (source)->ref_count--;                                          \
    else                                                                \
    source_unref_internal ((source), (context), TRUE);                       \
} X_STMT_END

enum SourceFlags {
    X_SOURCE_READY = 1 << X_HOOK_FLAG_USER_SHIFT,
    X_SOURCE_CAN_RECURSE = 1 << (X_HOOK_FLAG_USER_SHIFT + 1),
    X_SOURCE_BLOCKED = 1 << (X_HOOK_FLAG_USER_SHIFT + 2)
};

struct _xMainContext
{
    /* used for both sources and poll records */
    xMutex          mutex;
    xCond           cond;
    xThread         *owner;
    xint            owner_count;
    xSList          *waiters;
    xint            ref_count;
    xPtrArray       *dispatches;
    xbool            in_check_or_prepare;
    /* timeout for current iteration */
    xint            timeout;
    xuint           next_id;
    xList           *source_lists;
    xPollFd         *cached_poll_fds;
    xuint           cached_poll_nfds;
    xWakeup         *wakeup;
    xint64          time;
    xbool           time_is_fresh;
    PollRecord      *poll_records;
    PollRecord      *poll_records_tail;
    xuint           n_poll_records;
    xbool           poll_changed;
    xPollFd         wake_up_rec;
};
struct _xMainLoop
{
    xMainContext    *context;
    xbool           is_running;
    xint            ref_count;
};
struct _xSourcePrivate
{
    xSList  *poll_fds;
    xSource *prev;
    xSource *next;
    xSList  *child_sources;
    xSource *parent_source;
};

typedef struct {
    xMainContext *context;
    xbool  may_modify;
    xList *current_list;
    xSource *source;
} xSourceIter;


typedef struct {
    xint        ref_count;
    xSourceFunc func;
    xptr        data;
    xFreeFunc   notify;
} SourceCallback;

X_LOCK_DEFINE_STATIC(main_context_list);
static xSList       *main_context_list;
static xbool       _x_main_poll_debug;
xMainContext*
x_main_context_new      (void)
{
    xMainContext *context;

    context = x_new0 (xMainContext, 1);
    x_mutex_init (&context->mutex);
    x_cond_init (&context->cond);

    context->owner = NULL;
    context->waiters = NULL;

    context->ref_count = 1;
    context->next_id = 1;

    context->dispatches = x_ptr_array_new_full (8, NULL);
    context->wakeup = x_wakeup_new ();

    X_LOCK (main_context_list);
    main_context_list = x_slist_prepend (main_context_list, context);

    if (_x_main_poll_debug)
        x_printf ("created context=%p\n", context);

    X_UNLOCK (main_context_list);

    return context;
}
xMainContext*
x_main_context_ref      (xMainContext   *context)
{
    x_return_val_if_fail (context != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&context->ref_count) > 0, NULL); 

    x_atomic_int_inc (&context->ref_count);

    return context;
}

xint
x_main_context_unref    (xMainContext   *context)
{
    xint ref_count;
    x_return_val_if_fail (context != NULL, -1);
    x_return_val_if_fail (x_atomic_int_get (&context->ref_count) > 0, -1); 

    ref_count = x_atomic_int_dec (&context->ref_count);
    if X_LIKELY (ref_count != 0)
        return ref_count;
    X_LOCK (main_context_list);
    main_context_list = x_slist_remove (main_context_list, context);
    X_UNLOCK (main_context_list);

    return ref_count;
}

xMainContext*
x_main_context_default  (void)
{
    static xMainContext *default_context = NULL;
    if (!x_atomic_ptr_get (&default_context)
        && x_once_init_enter (&default_context) ){
        default_context = x_main_context_new ();
        x_once_init_leave (&default_context);
    }
    return default_context;
}

xMainLoop*
x_main_loop_new         (xMainContext   *context,
                         xbool          running)
{
    xMainLoop *loop;

    x_return_val_if_fail (context != NULL, NULL);

    x_main_context_ref (context);

    loop = x_new0 (xMainLoop, 1);
    loop->context = context;
    loop->is_running = (running != FALSE);
    loop->ref_count = 1;

    return loop;
}
/* make current thread to be owner of context */
static xbool 
main_context_acquire_U  (xMainContext   *context,
                         xThread        *self)
{
    xbool result = FALSE;

    LOCK_CONTEXT (context);

    if (!context->owner) {
        context->owner = self;
        x_assert (context->owner_count == 0);
    }

    if (context->owner == self) {
        context->owner_count++;
        result = TRUE;
    }

    UNLOCK_CONTEXT (context);

    return result;
}
/* wait unitl can be owner of the context */
static xbool
main_context_wait_L     (xMainContext   *context,
                         xCond          *cond,
                         xMutex         *mutex,
                         xThread        *self)
{
    xbool result = FALSE;
    xbool loop_internal_waiter;
    loop_internal_waiter = (mutex == &context->mutex);
    if (!loop_internal_waiter)
        LOCK_CONTEXT (context);
    if (context->owner && context->owner != self) {
        MainWaiter waiter = {cond, mutex};
        context->waiters = x_slist_prepend (context->waiters, &waiter);
        if (!loop_internal_waiter)
            UNLOCK_CONTEXT (context);
        x_cond_wait (cond, mutex);
        if (!loop_internal_waiter)
            LOCK_CONTEXT (context);
        context->waiters = x_slist_remove(context->waiters, &waiter);
    }
    if (!context->owner) {
        context->owner = self;
        x_assert (context->owner_count == 0);
    }
    if (context->owner == self) {
        context->owner_count++;
        result = TRUE;
    }
    if (!loop_internal_waiter)
        UNLOCK_CONTEXT (context);

    return result;
}
static void
main_context_release_U  (xMainContext   *context)
{
    LOCK_CONTEXT (context);
    context->owner_count--;
    if (context->owner_count == 0) {
        context->owner = NULL;
        if (context->waiters) {
            xbool loop_internal_waiter;
            MainWaiter* waiter = context->waiters->data;
            context->waiters = context->waiters->next;
            loop_internal_waiter = (waiter->mutex == &context->mutex); 
            if (!loop_internal_waiter)
                x_mutex_lock (waiter->mutex);

            x_cond_signal (waiter->cond);

            if (!loop_internal_waiter)
                x_mutex_lock (waiter->mutex);
        }
    }
    UNLOCK_CONTEXT (context);
}
static void
main_dispatch_delete_I  (xptr           dispatch)
{
    x_slice_free (MainDispatch, dispatch);
}
static MainDispatch*
get_dispatch_I          (void)
{
    static xPrivate depth_private = X_PRIVATE_INIT (main_dispatch_delete_I);
    MainDispatch *dispatch;

    dispatch = x_private_get (&depth_private);
    if (!dispatch) {
        dispatch = x_slice_new0 (MainDispatch);
        x_private_set (&depth_private, dispatch);
    }
    return dispatch;
}
static void
main_context_remove_poll_unlocked (xMainContext *context,
                                   xPollFd      *fd)
{
    PollRecord *pollrec, *prevrec, *nextrec;

    prevrec = NULL;
    pollrec = context->poll_records;

    while (pollrec)
    {
        nextrec = pollrec->next;
        if (pollrec->fd == fd)
        {
            if (prevrec != NULL)
                prevrec->next = nextrec;
            else
                context->poll_records = nextrec;

            if (nextrec != NULL)
                nextrec->prev = prevrec;
            else
                context->poll_records_tail = prevrec;

            x_slice_free (PollRecord, pollrec);

            context->n_poll_records--;
            break;
        }
        prevrec = pollrec;
        pollrec = nextrec;
    }

    context->poll_changed = TRUE;

    /* Now wake up the main loop if it is waiting in the poll() */
    x_wakeup_signal (context->wakeup);
}
static void 
main_context_add_poll_L (xMainContext   *context,
                         xint           priority,
                         xPollFd        *fd)
{
    PollRecord *prevrec, *nextrec;
    PollRecord *newrec = x_slice_new (PollRecord);

    /* This file descriptor may be checked before we ever poll */
    fd->revents = 0;
    newrec->fd = fd;
    newrec->priority = priority;

    prevrec = context->poll_records_tail;
    nextrec = NULL;
    while (prevrec && priority < prevrec->priority)
    {
        nextrec = prevrec;
        prevrec = prevrec->prev;
    }

    if (prevrec)
        prevrec->next = newrec;
    else
        context->poll_records = newrec;

    newrec->prev = prevrec;
    newrec->next = nextrec;

    if (nextrec)
        nextrec->prev = newrec;
    else 
        context->poll_records_tail = newrec;

    context->n_poll_records++;

    context->poll_changed = TRUE;

    /* Now wake up the main loop if it is waiting in the poll() */
    x_wakeup_signal (context->wakeup);
}
static void
main_context_remove_poll_L (xMainContext *context,
                            xPollFd      *fd)
{
    PollRecord *pollrec, *prevrec, *nextrec;

    prevrec = NULL;
    pollrec = context->poll_records;

    while (pollrec) {
        nextrec = pollrec->next;
        if (pollrec->fd == fd) {
            if (prevrec != NULL)
                prevrec->next = nextrec;
            else
                context->poll_records = nextrec;

            if (nextrec != NULL)
                nextrec->prev = prevrec;
            else
                context->poll_records_tail = prevrec;

            x_slice_free (PollRecord, pollrec);

            context->n_poll_records--;
            break;
        }
        prevrec = pollrec;
        pollrec = nextrec;
    }

    context->poll_changed = TRUE;

    /* Now wake up the main loop if it is waiting in the poll() */
    x_wakeup_signal (context->wakeup);
}

static void
block_source            (xSource        *source)
{
    xSList *tmp_list;

    x_return_if_fail (!SOURCE_BLOCKED(source));

    source->flags |= X_SOURCE_BLOCKED;

    tmp_list = source->priv->poll_fds;
    while (tmp_list){
        main_context_remove_poll_unlocked (source->context, tmp_list->data);
        tmp_list = tmp_list->next;
    }

    if (source->priv && source->priv->child_sources){
        tmp_list = source->priv->child_sources;
        while (tmp_list){
            block_source (tmp_list->data);
            tmp_list = tmp_list->next;
        }
    }
}
static void
unblock_source_L        (xSource        *source)
{
    xSList *tmp_list;

    x_return_if_fail (SOURCE_BLOCKED (source)); /* Source already unblocked */
    x_return_if_fail (!SOURCE_DESTROYED (source));

    source->flags &= ~X_SOURCE_BLOCKED;

    tmp_list = source->priv->poll_fds;
    while (tmp_list){
        main_context_add_poll_L (source->context, source->priority, tmp_list->data);
        tmp_list = tmp_list->next;
    }

    if (source->priv && source->priv->child_sources){
        tmp_list = source->priv->child_sources;
        while (tmp_list){
            unblock_source_L (tmp_list->data);
            tmp_list = tmp_list->next;
        }
    }
}
/* Holds context's lock
*/
static SourceList *
find_source_list_for_priority (xMainContext *context,
                               xint          priority,
                               xbool      create)
{
    xList *iter, *last;
    SourceList *source_list;

    last = NULL;
    for (iter = context->source_lists; iter != NULL; last = iter, iter = iter->next)
    {
        source_list = iter->data;

        if (source_list->priority == priority)
            return source_list;

        if (source_list->priority > priority)
        {
            if (!create)
                return NULL;

            source_list = x_slice_new0 (SourceList);
            source_list->priority = priority;
            context->source_lists = x_list_insert_before (context->source_lists,
                                                          iter,
                                                          source_list);
            return source_list;
        }
    }

    if (!create)
        return NULL;

    source_list = x_slice_new0 (SourceList);
    source_list->priority = priority;

    if (!last)
        context->source_lists = x_list_append (NULL, source_list);
    else
    {
        /* This just appends source_list to the end of
         * context->source_lists without having to walk the list again.
         */
        last = x_list_append (last, source_list);
    }
    return source_list;
}

static void
source_remove_from_context (xSource      *source,
                            xMainContext *context)
{
    SourceList *source_list;

    source_list = find_source_list_for_priority (context, source->priority, FALSE);
    x_return_if_fail (source_list != NULL);

    if (source->priv->prev)
        source->priv->prev->priv->next = source->priv->next;
    else
        source_list->head = source->priv->next;

    if (source->priv->next)
        source->priv->next->priv->prev = source->priv->prev;
    else
        source_list->tail = source->priv->prev;

    source->priv->prev = NULL;
    source->priv->next = NULL;

    if (source_list->head == NULL)
    {
        context->source_lists = x_list_remove (context->source_lists, source_list);
        x_slice_free (SourceList, source_list);
    }
}

static void
source_unref_internal (xSource      *source,
                       xMainContext *context,
                       xbool      have_lock)
{
    xptr old_cb_data = NULL;
    xSourceCallback *old_cb_funcs = NULL;

    x_return_if_fail (source != NULL);

    if (!have_lock && context)
        LOCK_CONTEXT (context);

    source->ref_count--;
    if (source->ref_count == 0)
    {
        old_cb_data = source->callback_data;
        old_cb_funcs = source->callback_funcs;

        source->callback_data = NULL;
        source->callback_funcs = NULL;

        if (context)
        {
            if (!SOURCE_DESTROYED (source))
                x_warning (X_STRLOC ": ref_count == 0, but source was still attached to a context!");
            source_remove_from_context (source, context);
        }

        if (source->source_funcs->finalize)
        {
            if (context)
                UNLOCK_CONTEXT (context);
            source->source_funcs->finalize (source);
            if (context)
                LOCK_CONTEXT (context);
        }

        x_free (source->name);
        source->name = NULL;

        x_slist_free (source->priv->poll_fds);
        source->priv->poll_fds = NULL;

        x_slice_free (xSourcePrivate, source->priv);
        source->priv = NULL;

        x_free (source);
    }

    if (!have_lock && context)
        UNLOCK_CONTEXT (context);

    if (old_cb_funcs)
    {
        if (have_lock)
            UNLOCK_CONTEXT (context);

        old_cb_funcs->unref (old_cb_data);

        if (have_lock)
            LOCK_CONTEXT (context);
    }
}
static void
source_destroy_internal (xSource      *source,
                         xMainContext *context,
                         xbool      have_lock);
static void
child_source_remove_internal (xSource *child_source,
                              xMainContext *context)
{
    xSource *parent_source = child_source->priv->parent_source;

    parent_source->priv->child_sources =
        x_slist_remove (parent_source->priv->child_sources, child_source);
    child_source->priv->parent_source = NULL;

    source_destroy_internal (child_source, context, TRUE);
    source_unref_internal (child_source, context, TRUE);
}
static void
source_destroy_internal (xSource      *source,
                         xMainContext *context,
                         xbool      have_lock)
{
    if (!have_lock)
        LOCK_CONTEXT (context);

    if (!SOURCE_DESTROYED (source))
    {
        xSList *tmp_list;
        xptr old_cb_data;
        xSourceCallback *old_cb_funcs;

        source->flags &= ~X_HOOK_FLAG_ACTIVE;

        old_cb_data = source->callback_data;
        old_cb_funcs = source->callback_funcs;

        source->callback_data = NULL;
        source->callback_funcs = NULL;

        if (old_cb_funcs)
        {
            UNLOCK_CONTEXT (context);
            old_cb_funcs->unref (old_cb_data);
            LOCK_CONTEXT (context);
        }

        if (!SOURCE_BLOCKED (source))
        {
            tmp_list = source->priv->poll_fds;
            while (tmp_list)
            {
                main_context_remove_poll_unlocked (context, tmp_list->data);
                tmp_list = tmp_list->next;
            }
        }

        while (source->priv->child_sources)
            child_source_remove_internal (source->priv->child_sources->data, context);

        if (source->priv->parent_source)
            child_source_remove_internal (source, context);

        source_unref_internal (source, context, TRUE);
    }

    if (!have_lock)
        UNLOCK_CONTEXT (context);
}

static void
source_destroy (xSource *source)
{
    xMainContext *context;

    x_return_if_fail (source != NULL);

    context = source->context;

    if (context)
        source_destroy_internal (source, context, FALSE);
    else
        source->flags &= ~X_HOOK_FLAG_ACTIVE;
}
static void
main_dispatch_L         (xMainContext   *context)
{
    MainDispatch *current = get_dispatch_I ();
    xsize i;

    for (i=0; i<context->dispatches->len; ++i) {
        xSource *source = context->dispatches->data[i];
        context->dispatches->data[i] = NULL;
        x_assert (source);

        source->flags &= ~X_SOURCE_READY;

        if (!SOURCE_DESTROYED (source)) {
            xbool was_in_call;
            xptr  user_data = NULL;
            xSourceFunc callback = NULL;
            xSourceCallback *cb_funcs;
            xptr cb_data;
            xbool need_destroy;

            xbool (*dispatch) (xSource*, xSourceFunc, xptr);

            xSList  current_source_link;

            dispatch = source->source_funcs->dispatch;
            cb_funcs = source->callback_funcs;
            cb_data  = source->callback_data;

            if (cb_funcs)
                cb_funcs->ref (cb_data);
            if (source->flags & X_SOURCE_CAN_RECURSE)
                block_source (source);

            was_in_call = source->flags & X_HOOK_FLAG_IN_CALL;
            source->flags |= X_HOOK_FLAG_IN_CALL;

            if (cb_funcs)
                cb_funcs->get (cb_data, source, &callback, &user_data);

            UNLOCK_CONTEXT (context);

            current->depth++;
            current_source_link.data = source;
            current_source_link.next = current->sources;
            current->sources = &current_source_link;
            need_destroy = !dispatch (source, callback, user_data);

            x_assert (current->sources == &current_source_link);
            current->sources = current_source_link.next;
            current->depth--;

            if (cb_funcs)
                cb_funcs->unref (cb_data);

            LOCK_CONTEXT (context);

            if (!was_in_call)
                source->flags &= ~X_HOOK_FLAG_IN_CALL;

            if (SOURCE_BLOCKED (source) && !SOURCE_DESTROYED (source))
                unblock_source_L (source);
            if (need_destroy && !SOURCE_DESTROYED (source)) {
                x_assert (source->context == context);
                source_destroy_internal (source, context, TRUE);
            }
        }
        SOURCE_UNREF (source, context);
    }
    context->dispatches->len = 0;
}
static void
main_context_dispatch_U   (xMainContext   *context)
{
    LOCK_CONTEXT (context);

    if (context->dispatches->len > 0)
        main_dispatch_L (context);
    UNLOCK_CONTEXT (context);
}
/* Holds context's lock */
static void
source_iter_init_L      (xSourceIter    *iter,
                         xMainContext   *context,
                         xbool          may_modify)
{
    iter->context = context;
    iter->current_list = NULL;
    iter->source = NULL;
    iter->may_modify = may_modify;
}
/* Holds context's lock */
static xbool
source_iter_next_L      (xSourceIter    *iter,
                         xSource        **source)
{
    xSource *next_source;

    if (iter->source)
        next_source = iter->source->priv->next;
    else
        next_source = NULL;

    if (!next_source)
    {
        if (iter->current_list)
            iter->current_list = iter->current_list->next;
        else
            iter->current_list = iter->context->source_lists;

        if (iter->current_list)
        {
            SourceList *source_list = iter->current_list->data;

            next_source = source_list->head;
        }
    }

    /* Note: unreffing iter->source could potentially cause its
     * SourceList to be removed from source_lists (if iter->source is
     * the only source in its list, and it is destroyed), so we have to
     * keep it reffed until after we advance iter->current_list, above.
     */

    if (iter->source && iter->may_modify)
        SOURCE_UNREF (iter->source, iter->context);
    iter->source = next_source;
    if (iter->source && iter->may_modify)
        iter->source->ref_count++;

    *source = iter->source;
    return *source != NULL;
}
/* Holds context's lock. Only necessary to call if you broke out of
 * the g_source_iter_next() loop early.
 */
static void
source_iter_clear_L     (xSourceIter    *iter)
{
    if (iter->source && iter->may_modify) {
        SOURCE_UNREF (iter->source, iter->context);
        iter->source = NULL;
    }
}
xbool
main_context_prepare_U  (xMainContext *context,
                         xint         *priority)
{
    xsize i;
    xint n_ready = 0;
    xint current_priority = X_MAXINT;
    xSource *source;
    xSourceIter iter;

    LOCK_CONTEXT (context);

    context->time_is_fresh = FALSE;

    if (context->in_check_or_prepare) {
        x_warning ("main_context_prepare() called recursively from "
                   "within a source's check() or prepare() member.");
        UNLOCK_CONTEXT (context);
        return FALSE;
    }

#if 0
    /* If recursing, finish up current dispatch, before starting over */
    if (context->pending_dispatches)
    {
        if (dispatch)
            g_main_dispatch (context, &current_time);

        UNLOCK_CONTEXT (context);
        return TRUE;
    }
#endif

    /* If recursing, clear list of pending dispatches */

    for (i = 0; i < context->dispatches->len; i++) {
        if (context->dispatches->data[i])
            SOURCE_UNREF ((xSource *)context->dispatches->data[i], context);
    }
    context->dispatches->len = 0;

    /* Prepare all sources, find min wait time */
    context->timeout = -1;

    source_iter_init_L (&iter, context, TRUE);
    while (source_iter_next_L (&iter, &source)) {
        xint source_timeout = -1;

        if (SOURCE_DESTROYED (source) || SOURCE_BLOCKED (source))
            continue;
        if ((n_ready > 0) && (source->priority > current_priority))
            break;

        if (!(source->flags & X_SOURCE_READY)) {
            xbool result;
            xbool (*prepare)  (xSource  *source, 
                               xint     *timeout);

            prepare = source->source_funcs->prepare;
            context->in_check_or_prepare++;
            UNLOCK_CONTEXT (context);

            result = (*prepare) (source, &source_timeout);

            LOCK_CONTEXT (context);
            context->in_check_or_prepare--;

            if (result) {
                xSource *ready_source = source;

                while (ready_source) {
                    ready_source->flags |= X_SOURCE_READY;
                    ready_source = ready_source->priv->parent_source;
                }
            }
        }

        if (source->flags & X_SOURCE_READY) {
            n_ready++;
            current_priority = source->priority;
            context->timeout = 0;
        }

        if (source_timeout >= 0) {
            if (context->timeout < 0)
                context->timeout = source_timeout;
            else
                context->timeout = MIN (context->timeout, source_timeout);
        }
    }
    source_iter_clear_L (&iter);
    UNLOCK_CONTEXT (context);

    if (priority)
        *priority = current_priority;

    return (n_ready > 0);
}
static void
main_context_poll_U     (xMainContext   *context,
                         xint           timeout,
                         xint           priority,
                         xPollFd        *fds,
                         xint           n_fds)
{
#ifdef  X_MAIN_POLL_DEBUG
    xTimer *poll_timer;
    PollRecord *pollrec;
    xint i;
#endif

    if (n_fds || timeout != 0) {
#ifdef	X_MAIN_POLL_DEBUG
        if (_x_main_poll_debug) {
            x_printf ("polling context=%p n=%d timeout=%d\n",
                      context, n_fds, timeout);
            poll_timer = x_timer_new ();
        }
#endif
        if (x_poll(fds, n_fds, timeout) < 0 && errno != EINTR) {
            x_warning ("poll(2) failed due to: %s.", strerror (errno));
        }

#ifdef	X_MAIN_POLL_DEBUG
        if (_x_main_poll_debug) {
            LOCK_CONTEXT (context);

            x_print ("g_main_poll(%d) timeout: %d - elapsed %12.10f seconds",
                     n_fds,
                     timeout,
                     x_timer_elapsed (poll_timer, NULL));
            x_timer_destroy (poll_timer);
            pollrec = context->poll_records;

            while (pollrec != NULL) {
                i = 0;
                while (i < n_fds)
                {
                    if (fds[i].fd == pollrec->fd->fd &&
                        pollrec->fd->events &&
                        fds[i].revents)
                    {
                        g_print (" [" G_POLLFD_FORMAT " :", fds[i].fd);
                        if (fds[i].revents & G_IO_IN)
                            g_print ("i");
                        if (fds[i].revents & G_IO_OUT)
                            g_print ("o");
                        if (fds[i].revents & G_IO_PRI)
                            g_print ("p");
                        if (fds[i].revents & G_IO_ERR)
                            g_print ("e");
                        if (fds[i].revents & G_IO_HUP)
                            g_print ("h");
                        if (fds[i].revents & G_IO_NVAL)
                            g_print ("n");
                        g_print ("]");
                    }
                    i++;
                }
                pollrec = pollrec->next;
            }
            x_printf ("\n");

            UNLOCK_CONTEXT (context);
        }
#endif
    } /* if (n_fds || timeout != 0) */
}
xbool
main_context_check_U    (xMainContext   *context,
                         xint           max_priority,
                         xPollFd        *fds,
                         xsize          n_fds)
{
    xSource *source;
    xSourceIter iter;
    PollRecord *pollrec;
    xint n_ready = 0;
    xsize i;

    LOCK_CONTEXT (context);

    if (context->in_check_or_prepare) {
        x_warning ("main_context_check_U() called recursively from "
                   "within a source's check() or prepare() member.");
        UNLOCK_CONTEXT (context);
        return FALSE;
    }

    if (context->wake_up_rec.revents)
        x_wakeup_ack (context->wakeup);

    /* If the set of poll file descriptors changed, bail out
     * and let the main loop rerun
     */
    if (context->poll_changed) {
        UNLOCK_CONTEXT (context);
        return FALSE;
    }

    pollrec = context->poll_records;
    for (i=0; i<n_fds; ++i) {
        if (pollrec->fd->events)
            pollrec->fd->revents = fds[i].revents;

        pollrec = pollrec->next;
    }

    source_iter_init_L (&iter, context, TRUE);
    while (source_iter_next_L (&iter, &source)) {
        if (SOURCE_DESTROYED (source) || SOURCE_BLOCKED (source))
            continue;
        if ((n_ready > 0) && (source->priority > max_priority))
            break;

        if (!(source->flags & X_SOURCE_READY)) {
            xbool result;
            xbool (*check) (xSource  *source);

            check = source->source_funcs->check;

            context->in_check_or_prepare++;
            UNLOCK_CONTEXT (context);

            result = (*check) (source);

            LOCK_CONTEXT (context);
            context->in_check_or_prepare--;

            if (result) {
                xSource *ready_source = source;

                while (ready_source) {
                    ready_source->flags |= X_SOURCE_READY;
                    ready_source = ready_source->priv->parent_source;
                }
            }
        }

        if (source->flags & X_SOURCE_READY) {
            source->ref_count++;
            x_ptr_array_append1 (context->dispatches, source);

            n_ready++;

            /* never dispatch sources with less priority than the first
             * one we choose to dispatch
             */
            max_priority = source->priority;
        }
    }
    source_iter_clear_L (&iter);

    UNLOCK_CONTEXT (context);

    return n_ready > 0;
}
xint
main_context_query_U    (xMainContext   *context,
                         xint           max_priority,
                         xint           *timeout,
                         xPollFd        *fds,
                         xint           n_fds)
{
    xint n_poll;
    PollRecord *pollrec;

    LOCK_CONTEXT (context);

    pollrec = context->poll_records;
    n_poll = 0;
    while (pollrec && max_priority >= pollrec->priority) {
        /* We need to include entries with fd->events == 0 in the array because
         * otherwise if the application changes fd->events behind our back and 
         * makes it non-zero, we'll be out of sync when we check the fds[] array.
         * (Changing fd->events after adding an FD wasn't an anticipated use of 
         * this API, but it occurs in practice.) */
        if (n_poll < n_fds) {
            fds[n_poll].fd = pollrec->fd->fd;
            /* In direct contradiction to the Unix98 spec, IRIX runs into
             * difficulty if you pass in POLLERR, POLLHUP or POLLNVAL
             * flags in the events field of the pollfd while it should
             * just ignoring them. So we mask them out here.
             */
            fds[n_poll].events = pollrec->fd->events & ~(X_IO_ERR|X_IO_HUP|X_IO_NVAL);
            fds[n_poll].revents = 0;
        }

        pollrec = pollrec->next;
        n_poll++;
    }

    context->poll_changed = FALSE;

    if (timeout)
        *timeout = context->timeout;

    UNLOCK_CONTEXT (context);
    return n_poll;
}

static xbool
main_context_iterate_L  (xMainContext   *context,
                         xbool          block,
                         xbool          dispatch,
                         xThread        *self)
{
    xint max_priority;
    xint timeout;
    xbool some_ready;
    xint nfds, allocated_nfds;
    xPollFd *fds = NULL;

    UNLOCK_CONTEXT (context);

    if (!main_context_acquire_U (context, self)) {
        xbool got_ownership;

        LOCK_CONTEXT (context);
        if (!block) /* not block waiting for ownership */
            return FALSE;
        got_ownership = main_context_wait_L (context,
                                             &context->cond,
                                             &context->mutex,
                                             self);
        if (!got_ownership) /* failed */
            return FALSE;
    }
    else
        LOCK_CONTEXT (context);

    if (!context->cached_poll_fds) {
        context->cached_poll_nfds = context->n_poll_records;
        context->cached_poll_fds = x_new (xPollFd, context->n_poll_records);
    }
    allocated_nfds = context->cached_poll_nfds;
    fds = context->cached_poll_fds;

    UNLOCK_CONTEXT (context);

    main_context_prepare_U (context, &max_priority);

    while ((nfds = main_context_query_U (context, max_priority,
                                         &timeout, fds,
                                         allocated_nfds)) > allocated_nfds) {
        LOCK_CONTEXT (context);
        x_free (fds);
        context->cached_poll_nfds= allocated_nfds = nfds;
        context->cached_poll_fds = fds = x_new (xPollFd, nfds);
        UNLOCK_CONTEXT (context);
    }
    if (!block)
        timeout = 0;
    else if (context->time_is_fresh) {
        xint delta = (xint)((x_mtime () - context->time + 999)/1000);
        if (context->timeout >= delta)
            timeout = context->timeout - delta;
        else
            timeout = 0;
        context->time_is_fresh = FALSE;
    }
    main_context_poll_U (context, timeout, max_priority, fds, nfds);
    some_ready = main_context_check_U (context, max_priority, fds, nfds);

    if (dispatch)
        main_context_dispatch_U (context);

    main_context_release_U (context);

    LOCK_CONTEXT (context);

    return some_ready;
}
void 
x_main_loop_quit        (xMainLoop      *loop)
{
    x_return_if_fail (loop != NULL);
    x_return_if_fail (loop->context != NULL);
    x_return_if_fail (x_atomic_int_get (&loop->ref_count) > 0);

    LOCK_CONTEXT (loop->context);
    loop->is_running = FALSE;
    x_wakeup_signal (loop->context->wakeup);

    x_cond_broadcast (&loop->context->cond);

    UNLOCK_CONTEXT (loop->context);
}

void
x_main_loop_run         (xMainLoop      *loop)
{
    xThread *self = x_thread_self();

    x_return_if_fail (loop != NULL);
    x_return_if_fail (x_atomic_int_get (&loop->ref_count) > 0);

    if (!main_context_acquire_U (loop->context, self)) {
        xbool got_ownership = FALSE;

        /* Another thread owns this context */
        LOCK_CONTEXT (loop->context);
        x_main_loop_ref (loop);

        if (!loop->is_running)
            loop->is_running = TRUE;

        while (loop->is_running && !got_ownership) {
            got_ownership =
                main_context_wait_L (loop->context,
                                     &loop->context->cond,
                                     &loop->context->mutex,
                                     self);
        }

        if (!loop->is_running) { /* failed */
            UNLOCK_CONTEXT (loop->context);
            if (got_ownership)
                main_context_release_U (loop->context);
            x_main_loop_unref (loop);
            return;
        }

        x_assert (got_ownership);
    }
    else {
        LOCK_CONTEXT (loop->context);
        x_main_loop_ref (loop);
    }

    if (loop->context->in_check_or_prepare) {
        x_warning ("x_main_loop_run(): called recursively from within a source's "
                   "check() or prepare() member, iteration not possible.");
    }
    else {
        loop->is_running = TRUE;
        while (loop->is_running)
            main_context_iterate_L (loop->context, TRUE, TRUE, self);
    }

    UNLOCK_CONTEXT (loop->context);
    main_context_release_U (loop->context);
    x_main_loop_unref (loop);
}

void            x_main_loop_quit        (xMainLoop      *loop);

xMainLoop*
x_main_loop_ref         (xMainLoop      *loop)
{
    x_return_val_if_fail (loop != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&loop->ref_count) > 0, NULL);

    x_atomic_int_inc (&loop->ref_count);

    return loop;
}

xint
x_main_loop_unref       (xMainLoop      *loop)
{
    xint ref;

    x_return_val_if_fail (loop != NULL, -1);
    x_return_val_if_fail (x_atomic_int_get (&loop->ref_count) > 0, -1);

    ref = x_atomic_int_dec (&loop->ref_count);
    if X_LIKELY(ref != 0)
        return ref;

    x_main_context_unref (loop->context);
    x_free (loop);

    return ref;
}

xMainContext*
x_main_loop_context     (xMainLoop      *loop)
{
    x_return_val_if_fail (loop != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&loop->ref_count) > 0, NULL);

    return loop->context;
}

xSource*
x_source_new            (xSourceFuncs *source_funcs,
                         xsize         struct_size)
{
    xSource *source;

    x_return_val_if_fail (source_funcs != NULL, NULL);
    x_return_val_if_fail (struct_size >= sizeof (xSource), NULL);

    source = (xSource*) x_malloc0 (struct_size);
    source->priv = x_slice_new0 (xSourcePrivate);
    source->source_funcs = source_funcs;
    source->ref_count = 1;

    source->priority = X_PRIORITY_DEFAULT;

    source->flags = X_HOOK_FLAG_ACTIVE;

    return source;
}
void
x_source_unref (xSource *source)
{
    x_return_if_fail (source != NULL);

    source_unref_internal (source, source->context, FALSE);
}
static void
source_add_to_context_L (xSource      *source,
                         xMainContext *context)
{
    SourceList *source_list;
    xSource *prev, *next;

    source_list = find_source_list_for_priority (context, source->priority, TRUE);

    if (source->priv->parent_source) {
        x_assert (source_list->head != NULL);

        /* Put the source immediately before its parent */
        prev = source->priv->parent_source->priv->prev;
        next = source->priv->parent_source;
    }
    else {
        prev = source_list->tail;
        next = NULL;
    }

    source->priv->next = next;
    if (next)
        next->priv->prev = source;
    else
        source_list->tail = source;

    source->priv->prev = prev;
    if (prev)
        prev->priv->next = source;
    else
        source_list->head = source;
}
static void
source_remove_from_context_L (xSource      *source,
                              xMainContext *context)
{
    SourceList *source_list;

    source_list = find_source_list_for_priority (context, source->priority, FALSE);
    x_return_if_fail (source_list != NULL);

    if (source->priv->prev)
        source->priv->prev->priv->next = source->priv->next;
    else
        source_list->head = source->priv->next;

    if (source->priv->next)
        source->priv->next->priv->prev = source->priv->prev;
    else
        source_list->tail = source->priv->prev;

    source->priv->prev = NULL;
    source->priv->next = NULL;

    if (source_list->head == NULL) {
        context->source_lists = x_list_remove (context->source_lists, source_list);
        x_slice_free (SourceList, source_list);
    }
}
static xuint
source_attach_L (xSource      *source,
                 xMainContext *context)
{
    xuint result = 0;
    xSList *tmp_list;

    source->context = context;
    result = source->source_id = context->next_id++;

    source->ref_count++;
    source_add_to_context_L (source, context);

    tmp_list = source->priv->poll_fds;
    while (tmp_list) {
        main_context_add_poll_L (context, source->priority, tmp_list->data);
        tmp_list = tmp_list->next;
    }

    tmp_list = source->priv->child_sources;
    while (tmp_list) {
        source_attach_L (tmp_list->data, context);
        tmp_list = tmp_list->next;
    }

    return result;
}
static void
source_set_priority_L (xSource      *source,
                       xMainContext *context,
                       xint          priority)
{
    xSList *tmp_list;

    x_return_if_fail (source->priv->parent_source == NULL ||
                      source->priv->parent_source->priority == priority);

    if (context) {
        /* Remove the source from the context's source and then
         * add it back after so it is sorted in the correct place
         */
        source_remove_from_context_L (source, source->context);
    }

    source->priority = priority;

    if (context) {
        source_add_to_context_L (source, context);

        if (!SOURCE_BLOCKED (source)) {
            tmp_list = source->priv->poll_fds;
            while (tmp_list) {
                main_context_remove_poll_L (context, tmp_list->data);
                main_context_add_poll_L (context, priority, tmp_list->data);

                tmp_list = tmp_list->next;
            }
        }
    }

    if (source->priv->child_sources) {
        tmp_list = source->priv->child_sources;
        while (tmp_list) {
            source_set_priority_L (tmp_list->data, context, priority);
            tmp_list = tmp_list->next;
        }
    }
}
xint64
x_source_get_time (xSource *source)
{
    xMainContext *context;
    xint64 result;

    x_return_val_if_fail (source->context != NULL, 0);

    context = source->context;

    LOCK_CONTEXT (context);

    if (!context->time_is_fresh)
    {
        context->time = x_mtime ();
        context->time_is_fresh = TRUE;
    }

    result = context->time;

    UNLOCK_CONTEXT (context);

    return result;
}

xuint
x_source_attach         (xSource        *source,
                         xMainContext   *context)
{
    xuint result = 0;

    x_return_val_if_fail (context != NULL, 0);
    x_return_val_if_fail (source->context == NULL, 0);
    x_return_val_if_fail (!SOURCE_DESTROYED (source), 0);

    LOCK_CONTEXT (context);

    result = source_attach_L (source, context);

    /* If another thread has acquired the context, wake it up since it
     * might be in poll() right now.
     */
    if (context->owner && context->owner != x_thread_self())
        x_wakeup_signal (context->wakeup);

    UNLOCK_CONTEXT (context);

    return result;
}
void
x_source_set_priority   (xSource        *source,
                         xint           priority)
{
    xMainContext *context;

    x_return_if_fail (source != NULL);

    context = source->context;

    if (context)
        LOCK_CONTEXT (context);
    source_set_priority_L (source, context, priority);
    if (context)
        UNLOCK_CONTEXT (context);
}
void
x_source_set_callback   (xSource        *source,
                         xptr           data,
                         xSourceCallback*callback)
{
    xMainContext *context;
    xptr old_cb_data;
    xSourceCallback *old_cb_funcs;

    x_return_if_fail (source != NULL);
    x_return_if_fail (callback != NULL || data == NULL);

    context = source->context;

    if (context)
        LOCK_CONTEXT (context);

    old_cb_data = source->callback_data;
    old_cb_funcs = source->callback_funcs;

    source->callback_data  = data;
    source->callback_funcs = callback;

    if (context)
        UNLOCK_CONTEXT (context);

    if (old_cb_funcs)
        old_cb_funcs->unref (old_cb_data);
}
static void
source_callback_ref     (xptr           cb_data)
{
    SourceCallback *callback = cb_data;
    callback->ref_count++;
}


static void
source_callback_unref   (xptr           cb_data)
{
    SourceCallback *callback = cb_data;

    callback->ref_count--;
    if (callback->ref_count == 0) {
        if (callback->notify)
            callback->notify (callback->data);
        x_free (callback);
    }
}

static void
source_callback_get     (xptr           cb_data,
                         xSource        *source, 
                         xSourceFunc    *func,
                         xptr           *data)
{
    SourceCallback *callback = cb_data;

    *func = callback->func;
    *data = callback->data;
}

static xSourceCallback _source_callback = {
    source_callback_ref,
    source_callback_unref,
    source_callback_get,
};

void
x_source_set_func       (xSource        *source,
                         xSourceFunc    func,
                         xptr           data,
                         xFreeFunc      notify)
{
    SourceCallback *new_callback;

    x_return_if_fail (source != NULL);

    new_callback = x_new (SourceCallback, 1);

    new_callback->ref_count = 1;
    new_callback->func = func;
    new_callback->data = data;
    new_callback->notify = notify;

    x_source_set_callback (source, new_callback, &_source_callback);
}

static xbool
idle_prepare (xSource *source, xint *timeout)
{
    *timeout = 0;
    return TRUE;
}
static xbool
idle_check (xSource *source)
{
    return TRUE;
}

static xbool
idle_dispatch (xSource    *source, 
               xSourceFunc callback,
               xptr    user_data)
{
    if (!callback)
    {
        x_warning ("Idle source dispatched without callback\n"
                   "You must call x_source_set_func().");
        return FALSE;
    }

    return callback (user_data);
}

static xSourceFuncs _idle_funcs =
{
    idle_prepare,
    idle_check,
    idle_dispatch,
    NULL
};

xuint
x_idle_add_full         (xuint          priority,
                         xSourceFunc    source_func,
                         xptr           user_data,
                         xFreeFunc      free_func)
{
    xSource *source;
    xuint id;

    x_return_val_if_fail (source_func != NULL, 0);

    if (!priority)
        priority = X_PRIORITY_DEFAULT_IDLE;

    source = x_source_new (&_idle_funcs, sizeof (xSource));
    x_source_set_priority (source, priority);
    x_source_set_func (source, source_func, user_data, free_func);
    id = x_source_attach (source, NULL);
    x_source_unref (source);

    return id;
}
typedef struct {
    xSource     source;
    xint64      expiration;
    xuint       interval;
    xbool       seconds;
} TimeoutSource;
static void
timeout_set_expiration (TimeoutSource *timeout_source,
                        xint64          current_time)
{
    timeout_source->expiration = current_time +
        (xuint64) timeout_source->interval * 1000;
}

static xbool
timeout_prepare     (xSource *source,
                     xint    *timeout)
{
    TimeoutSource *timeout_source = (TimeoutSource *) source;
    xint64 now = x_source_get_time (source);

    if (now < timeout_source->expiration) {
        /* Round up to ensure that we don't try again too early */
        *timeout = (xint)((timeout_source->expiration - now + 999) / 1000);
        return FALSE;
    }

    *timeout = 0;
    return TRUE;
}

static xbool 
timeout_check           (xSource        *source)
{
    TimeoutSource *timeout_source = (TimeoutSource *) source;
    xint64 now = x_source_get_time (source);

    return timeout_source->expiration <= now;
}

static xbool
timeout_dispatch        (xSource        *source,
                         xSourceFunc    callback,
                         xptr           user_data)
{
    TimeoutSource *timeout_source = (TimeoutSource *)source;
    xbool again;

    if (!callback) {
        x_warning ("Timeout source dispatched without callback\n"
                   "You must call g_source_set_callback().");
        return FALSE;
    }

    again = callback (user_data);

    if (again)
        timeout_set_expiration (timeout_source, x_source_get_time (source));

    return again;
}

xuint
x_timeout_add_full      (xuint          priority,
                         xuint          interval,
                         xSourceFunc    function,
                         xptr           data,
                         xFreeFunc      notify)
{
    xSource *source;
    xuint id;
    static xSourceFuncs _timeout_funcs = {
        timeout_prepare,
        timeout_check,
        timeout_dispatch,
        NULL
    };
    x_return_val_if_fail (function != NULL, 0);

    if (!priority)
        priority = X_PRIORITY_DEFAULT_IDLE;

    source = x_source_new (&_timeout_funcs, sizeof (TimeoutSource));
    ((TimeoutSource*)source)->interval = interval;
    timeout_set_expiration ((TimeoutSource*)source, x_mtime());


    x_source_set_priority (source, priority);
    x_source_set_func (source, function, data, notify);
    id = x_source_attach (source, x_main_context_default());
    x_source_unref (source);

    return id;
}
