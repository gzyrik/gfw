#include "config.h"
#include "xsignal.h"
#include "xclass-prv.h"
#include "xclosure-prv.h"
#include "xvalue.h"
#include "xboxed.h"
#include "xobject.h"
#include "xvalist.h"
#include <string.h>
/* indicates single_va_closure is valid but empty */
#define	SINGLE_VA_CLOSURE_EMPTY_MAGIC X_SIZE_TO_PTR(1)

typedef xClosureMarshal          xSignalCMarshaller;
typedef xVaClosureMarshal        xSignalCVaMarshaller;

typedef struct _Handler Handler;
typedef struct _HandlerMatch HandlerMatch;
typedef struct _Emission Emission;

typedef struct{
    xSignalAccumulator func;
    xptr           data;
} SignalAccumulator;
typedef struct {
    /* permanent portion */
    xuint               signal_id;
    xType               type;
    xcstr               name;
    xuint               destroyed : 1;
    /* reinitializable portion */
    xuint               flags : 9;
    xuint               n_params : 8;
    xuint               single_va_closure_is_valid : 1;
    xuint               single_va_closure_is_after : 1;
    /* mangled with X_SIGNAL_TYPE_STATIC_SCOPE flag */
    xType		        *param_types; 
    /* mangled with X_SIGNAL_TYPE_STATIC_SCOPE flag */
    xType		        return_type; 
    xHashTable            *class_closure_ht;
    SignalAccumulator   *accumulator;
    xSignalCMarshaller  c_marshaller;
    xSignalCVaMarshaller va_marshaller;
    xHookList           *emission_hooks;
    xClosure            *single_va_closure;
} SignalNode;

typedef struct {
    xType               type;
    xQuark              name;
    xuint               signal_id;
} SignalKey;
static xuint
signal_key_hash         (xcptr          key)
{
    const SignalKey *signal_key = key;
    return signal_key->type + signal_key->name;
}
static xint
signal_key_cmp          (xcptr          key1,
                         xcptr          key2,
                         ...)
{
    const SignalKey *signal_key1 = key1;
    const SignalKey *signal_key2 = key2;
    if (signal_key1->type < signal_key2->type)
        return -1;
    else
        return signal_key1->name > signal_key2->name; 
}
static void
signal_key_free         (xptr           key)
{
    x_slice_free (SignalKey, key);
}
struct _Handler {
    xulong          handler_id;
    Handler         *next;
    Handler         *prev;
    xQuark	        detail;
    xint            ref_count;
    xint            block_count : 16;
#define HANDLER_MAX_BLOCK_COUNT (1 << 16)
    xuint           after : 1;
    xClosure        *closure;
};

struct _HandlerMatch {
    Handler         *handler;
    HandlerMatch    *next;
    xuint           signal_id;
};
typedef struct {
    xuint           signal_id;
    Handler         *handlers;
    /* normal signal handlers are appended here  */
    Handler         *tail_before;
    /* CONNECT_AFTER handlers are appended here  */
    Handler         *tail_after;
} HandlerList;
typedef enum
{
    EMISSION_STOP,
    EMISSION_RUN,
    EMISSION_HOOK,
    EMISSION_RESTART
} EmissionState;
struct _Emission
{
    Emission             *next;
    xptr                    instance;
    xSignalInvocationHint ihint;
    EmissionState       state;
    xType			    chain_type;
};
typedef struct {
    xHook               hook;
    xQuark              detail;
} SignalHook;
#define	SIGNAL_HOOK(hook)	((SignalHook*) (hook))
X_LOCK_DEFINE_STATIC    (signal);
static xuint            _n_signal_nodes = 0;
static SignalNode       **_signal_nodes = NULL;
static xHashTable       *_signal_key_ht = NULL;
static xuint            _n_handlers     = 1;
static xHashTable       *_hlist_array_ht = NULL;
static Emission         *_recursive_emissions = NULL;
static Emission         *_restart_emissions = NULL;

static xcstr
type_debug_name_I       (xType          type)
{
    if (type)
    {
        xcstr  name = x_type_name (type & ~X_SIGNAL_TYPE_STATIC_SCOPE);
        return name ? name : "<unknown>";
    }
    else
        return "<invalid>";
}
static inline SignalNode*
lookup_signal_node_L    (xuint          signal_id)
{
    if (signal_id < _n_signal_nodes)
        return _signal_nodes[signal_id];
    else
        return NULL;
}
static inline xuint
lookup_signal_id_L      (xQuark         quark,
                         xType          type)
{
    SignalKey       key,*ret;
    xsize           n_ifaces;
    xType           *ifaces;

    key.type = type;
    key.name = quark;
    do {
        ret = x_hash_table_lookup (_signal_key_ht, &key);
        if (ret)
            return ret->signal_id;
        else
            key.type = x_type_parent (key.type);
    } while (key.type);

    ifaces = x_type_ifaces (type, &n_ifaces);
    while (n_ifaces--) {
        key.type = ifaces[n_ifaces];
        ret = x_hash_table_lookup (_signal_key_ht, &key);
        if (ret) {
            x_free (ifaces);
            return ret->signal_id;
        }
    }
    x_free (ifaces);
    return 0;
}
static inline xuint
parse_signal_name_L     (xcstr          name,
                         xType          itype,
                         xQuark         *detail_p,
                         xbool          force_quark)
{
    const xchar *colon = strchr (name, ':');
    xuint signal_id;

    if (!colon) {
        signal_id = lookup_signal_id_L (x_quark_try (name), itype);
        if (signal_id && detail_p)
            *detail_p = 0;
    }
    else if (colon[1] == ':') {
        xchar buffer[32];
        xuint l = colon - name;

        if (l < 32) {
            memcpy (buffer, name, l);
            buffer[l] = 0;
            signal_id = lookup_signal_id_L (x_quark_try (buffer), itype);
        }
        else {
            xchar *signal = x_new (xchar, l + 1);

            memcpy (signal, name, l);
            signal[l] = 0;
            signal_id = lookup_signal_id_L (x_quark_try (signal), itype);
            x_free (signal);
        }

        if (signal_id && detail_p)
            *detail_p = colon[2] ? (force_quark ? x_quark_from : x_quark_try) (colon + 2) : 0;
    }
    else
        signal_id = 0;
    return signal_id;
}
static void
node_check_deprecated   (const SignalNode   *node)
{
    static xcstr x_enable_diagnostic = NULL;

    if (X_UNLIKELY (!x_enable_diagnostic)) {
        x_enable_diagnostic = x_getenv ("X_ENABLE_DIAGNOSTIC");
        if (!x_enable_diagnostic)
            x_enable_diagnostic = "0";
    }

    if (x_enable_diagnostic[0] == '1') {
        if (node->flags & X_SIGNAL_DEPRECATED) {
            x_warning ("The signal %s::%s is deprecated and shouldn't be used "
                       "anymore. It will be removed in a future version.",
                       type_debug_name_I (node->type), node->name);
        }
    }
}

static inline xClosure*
lookup_closure          (SignalNode     *node,
                         xType          type)
{
    xClosure       *closure;
    while (type != 0) {
        closure = x_hash_table_lookup(node->class_closure_ht,
                                      X_SIZE_TO_PTR(type));
        if(closure) return closure;
        type = x_type_parent (type);
    }
    return x_hash_table_lookup(node->class_closure_ht, NULL);
}

static void
add_class_closure       (SignalNode     *node,
                         xType          itype,
                         xClosure       *closure)
{
    node->single_va_closure_is_valid = FALSE;

    if (!node->class_closure_ht)
        node->class_closure_ht = x_hash_table_new_full(x_direct_hash,
                                                       x_direct_cmp,
                                                       NULL,
                                                       (xFreeFunc)x_closure_unref);
    x_hash_table_insert (node->class_closure_ht,
                         X_SIZE_TO_PTR(itype),
                         x_closure_ref (closure));
    x_closure_sink (closure);
    if (node->c_marshaller && closure && X_CLOSURE_NEEDS_MARSHAL (closure)) {
        x_closure_set_marshal (closure, node->c_marshaller);
        if (node->va_marshaller)
            _x_closure_set_va_marshal (closure, node->va_marshaller);
    }
}
static void
node_update_single_va_closure   (SignalNode     *node)
{
    xClosure    *closure = NULL;
    xbool       is_after = FALSE;

    /* Fast path single-handler without boxing the arguments in GValues */
    if (X_TYPE_IS_OBJECT (node->type) &&
        (node->flags & (X_SIGNAL_MUST_COLLECT)) == 0 &&
        (node->emission_hooks == NULL || x_hook_first_valid(node->emission_hooks,TRUE) == NULL)) {
        xSignalFlags run_type;
        xsize n_class_closure = x_hash_table_size (node->class_closure_ht);

        if (n_class_closure == 0)
            closure = SINGLE_VA_CLOSURE_EMPTY_MAGIC;
        else if (n_class_closure == 1) {
            closure  = x_hash_table_lookup(node->class_closure_ht, NULL);
            if (closure) {
                run_type = node->flags & (X_SIGNAL_RUN_FIRST|X_SIGNAL_RUN_LAST|X_SIGNAL_RUN_CLEANUP);
                /* Only support *one* of run-first or run-last, not multiple or cleanup */
                if (run_type == X_SIGNAL_RUN_FIRST ||
                    run_type == X_SIGNAL_RUN_LAST) {
                    is_after = (run_type == X_SIGNAL_RUN_LAST);
                }
                else {
                    closure = NULL;
                }
            }
        }
    }

    node->single_va_closure_is_valid = TRUE;
    node->single_va_closure = closure;
    node->single_va_closure_is_after = is_after;
}
static inline Handler*
handler_new_L           (xbool          after)
{
    Handler *handler = x_slice_new (Handler);
#ifndef X_DISABLE_CHECKS
    if (_n_handlers < 1)
        x_error ("%s: handler id overflow", X_STRLOC);
#endif

    handler->handler_id = _n_handlers++;
    handler->prev = NULL;
    handler->next = NULL;
    handler->detail = 0;
    handler->ref_count = 1;
    handler->block_count = 0;
    handler->after = (after != FALSE);
    handler->closure = NULL;

    return handler;
}
static xint
handler_list_cmp        (xcptr          key1,
                         xcptr          key2,
                         ...)
{
    const HandlerList* hl1 = (const HandlerList*)key1;
    const HandlerList* hl2 = (const HandlerList*)key2;
    return hl1->signal_id - hl2->signal_id;
}
static HandlerList*
handler_list_ensure_L   (xuint          signal_id,
                         xptr           instance)
{
    xsize   i;
    xArray  *hlist_array;
    HandlerList *hlist;

    hlist_array = x_hash_table_lookup (_hlist_array_ht, instance);
    if (!hlist_array) {
        hlist_array = x_array_new_full (sizeof (HandlerList), 4, NULL);
        x_hash_table_insert (_hlist_array_ht, instance, hlist_array);
    }
    i = x_array_insert_sorted (hlist_array, &signal_id, NULL,
                               handler_list_cmp, FALSE);
    hlist = &x_array_index (hlist_array, HandlerList, i);
    if (signal_id != hlist->signal_id) {
        hlist->signal_id = signal_id;
    }
    return hlist;
}
static Handler*
handler_lookup          (xptr           instance,
                         xuint          handler_id,
                         xuint          *signal_id_p)
{
    xsize   i;
    xArray  *hlist_array;

    hlist_array = x_hash_table_lookup (_hlist_array_ht, instance);
    if (!hlist_array)
        return NULL;
    for (i=0; i< hlist_array->len; ++i) {
        Handler *handler;
        HandlerList *hlist = &x_array_index (hlist_array, HandlerList, i);
        for (handler = hlist->handlers; handler; handler = handler->next)
            if (handler->handler_id == handler_id) {
                if (signal_id_p)
                    *signal_id_p = hlist->signal_id;
                return handler;
            }
    }
    return NULL;
}
static void
handler_insert_L        (xuint          signal_id,
                         xptr           instance,
                         Handler        *handler)
{
    HandlerList *hlist;
    /* paranoid */
    x_assert (handler->prev == NULL && handler->next == NULL);

    hlist = handler_list_ensure_L (signal_id, instance);
    if (!hlist->handlers) {
        hlist->handlers = handler;
        if (!handler->after)
            hlist->tail_before = handler;
    }
    else if (handler->after) {
        handler->prev = hlist->tail_after;
        hlist->tail_after->next = handler;
    }
    else {
        if (hlist->tail_before)
        {
            handler->next = hlist->tail_before->next;
            if (handler->next)
                handler->next->prev = handler;
            handler->prev = hlist->tail_before;
            hlist->tail_before->next = handler;
        }
        else /* insert !after handler into a list of only after handlers */
        {
            handler->next = hlist->handlers;
            if (handler->next)
                handler->next->prev = handler;
            hlist->handlers = handler;
        }
        hlist->tail_before = handler;
    }

    if (!handler->next)
        hlist->tail_after = handler;
}
static inline HandlerList*
handler_list_lookup_L(xuint          signal_id,
                      xptr           instance)
{
    xArray  *hlist_array;

    hlist_array = x_hash_table_lookup (_hlist_array_ht, instance);
    if (hlist_array) {
        xint i = x_array_find_sorted (hlist_array, &signal_id,
                                      handler_list_cmp);
        if (i >= 0)
            return &x_array_index (hlist_array, HandlerList, i);
    }
    return NULL;
}
static inline void
handler_ref             (Handler        *handler)
{
    x_return_if_fail (handler->ref_count > 0);

    x_atomic_int_inc (&handler->ref_count);
}

static inline void
handler_unref_R         (xuint          signal_id,
                         xptr           instance,
                         Handler        *handler)
{
    x_return_if_fail (handler->ref_count > 0);

    if (X_UNLIKELY (x_atomic_int_dec (&handler->ref_count) == 0)) {
        HandlerList *hlist = NULL;

        if (handler->next)
            handler->next->prev = handler->prev;
        if (handler->prev)
            handler->prev->next = handler->next;
        else {
            hlist = handler_list_lookup_L (signal_id, instance);
            hlist->handlers = handler->next;
        }

        if (instance) {
            /*  check if we are removing the handler pointed to by tail_before  */
            if (!handler->after && (!handler->next || handler->next->after)) {
                if (!hlist)
                    hlist = handler_list_lookup_L (signal_id, instance);
                if (hlist)
                {
                    x_assert (hlist->tail_before == handler); /* paranoid */
                    hlist->tail_before = handler->prev;
                }
            }

            /*  check if we are removing the handler pointed to by tail_after  */
            if (!handler->next) {
                if (!hlist)
                    hlist = handler_list_lookup_L (signal_id, instance);
                if (hlist) {
                    x_assert (hlist->tail_after == handler); /* paranoid */
                    hlist->tail_after = handler->prev;
                }
            }
        }

        X_UNLOCK (signal);
        x_closure_unref (handler->closure);
        X_LOCK (signal);
        x_slice_free (Handler, handler);
    }
}
static inline HandlerMatch*
handler_match_prepend (HandlerMatch *mlist,
                       Handler      *handler,
                       xuint	     signal_id)
{
    HandlerMatch *node;
    node = x_slice_new (HandlerMatch);
    node->handler = handler;
    node->next = mlist;
    node->signal_id = signal_id;
    handler_ref (handler);
    return node;
}
static inline HandlerMatch*
handler_match_free1_R (HandlerMatch *node,
                       xptr      instance)
{
    HandlerMatch *next = node->next;

    handler_unref_R (node->signal_id, instance, node->handler);
    x_slice_free (HandlerMatch, node);

    return next;
}
static HandlerMatch*
handler_list_match_L    (HandlerList* hlist,
                         HandlerMatch *mlist,
                         xbool      only_one,
                         xuint      mask,
                         xQuark     detail,
                         xClosure   *closure,
                         xptr       func,
                         xptr       data)
{
    Handler *handler;
    SignalNode *node;

    if (mask & X_SIGNAL_MATCH_FUNC) {
        node = lookup_signal_node_L(hlist->signal_id);
        if (!node || !node->c_marshaller)
            return NULL;
    }
    mask = ~mask;
    for (handler = hlist->handlers; handler; handler = handler->next) {
        if (handler->handler_id &&
            ((mask & X_SIGNAL_MATCH_DETAIL) || handler->detail == detail) &&
            ((mask & X_SIGNAL_MATCH_CLOSURE) || handler->closure == closure) &&
            ((mask & X_SIGNAL_MATCH_DATA) || handler->closure->data == data) &&
            ((mask & X_SIGNAL_MATCH_UNBLOCKED) || handler->block_count == 0) &&
            ((mask & X_SIGNAL_MATCH_BLOCKED) || handler->block_count > 0) &&
            ((mask & X_SIGNAL_MATCH_FUNC)
             || (handler->closure->marshal == node->c_marshaller &&
                 X_REAL_CLOSURE (handler->closure)->meta_marshal == NULL &&
                 ((xCClosure*) handler->closure)->callback == func))) {
            mlist = handler_match_prepend (mlist, handler, hlist->signal_id);
            if (only_one)
                return mlist;
        }
    }
    return mlist;
}
static HandlerMatch*
handler_matched_collect_L  (xptr       instance,
                            xuint      mask,
                            xuint      signal_id,
                            xQuark     detail,
                            xClosure   *closure,
                            xptr       func,
                            xptr       data,
                            xbool      only_one)
{
    HandlerList *hlist;
    HandlerMatch *mlist = NULL;
    if (mask & X_SIGNAL_MATCH_ID) {
        hlist = handler_list_lookup_L (signal_id, instance);
        if (hlist)
            return handler_list_match_L (hlist, mlist, only_one,mask,
                                         detail, closure, func, data);
    }
    else {
        xsize i;
        xArray  *hlist_array;

        hlist_array = x_hash_table_lookup (_hlist_array_ht, instance);
        if (hlist_array) {
            for (i=0; i<hlist_array->len;++i) {
                hlist = &x_array_index (hlist_array, HandlerList, i);
                mlist = handler_list_match_L (hlist, mlist, only_one, mask,
                                              detail, closure, func, data);
                if (only_one && mlist)
                    return mlist;
            }
        }
    }
    return mlist;
}
static xsize
handler_foreach_matched_R (xptr         instance,
                           xuint        mask,
                           xuint        signal_id,
                           xQuark       detail,
                           xClosure     *closure,
                           xptr         func,
                           xptr         data,
                           void (*callback) (xptr, xulong))
{
    HandlerMatch *mlist;
    xsize n_handlers = 0;

    mlist = handler_matched_collect_L (instance, mask, signal_id,
                                       detail, closure, func, data, FALSE);
    if (mlist) {
        n_handlers++;
        if (mlist->handler->handler_id) {
            X_UNLOCK (signal);
            callback (instance, mlist->handler->handler_id);
            X_LOCK (signal);
        }
        mlist = handler_match_free1_R (mlist, instance);
    }
    return n_handlers;
}

static inline void
emission_push_L         (Emission       **emission_list_p,
                         Emission       *emission)
{
    emission->next = *emission_list_p;
    *emission_list_p = emission;
}

static inline void
emission_pop_L          (Emission       **emission_list_p,
                         Emission       *emission)
{
    Emission *node, *last = NULL;

    for (node = *emission_list_p; node;
         last = node, node = last->next) {
        if (node == emission) {
            if (last)
                last->next = node->next;
            else
                *emission_list_p = node->next;
            return;
        }
    }
    x_assert_not_reached ();
}
static inline Emission*
emission_find_L         (Emission       *emission_list,
                         xuint          signal_id,
                         xQuark         detail,
                         xptr           instance)
{
    Emission *emission;

    for (emission = emission_list; emission; emission = emission->next)
        if (emission->instance == instance &&
            emission->ihint.signal_id == signal_id &&
            emission->ihint.detail == detail)
            return emission;
    return NULL;
}

static inline xbool
accumulate              (xSignalInvocationHint  *ihint,
                         xValue                 *return_accu,
                         xValue	                *handler_return,
                         SignalAccumulator      *accumulator)
{
    xbool continue_emission;

    if (!accumulator)
        return TRUE;

    continue_emission = accumulator->func (ihint, return_accu, handler_return, accumulator->data);
    x_value_reset (handler_return);

    return continue_emission;
}
static void
signal_destroy_R        (SignalNode     *signal_node)
{
}

static xbool
signal_emit_unlocked_R  (SignalNode     *node,
                         xQuark	        detail,
                         xptr           instance,
                         xValue	        *emission_return,
                         const xValue   *instance_and_params)
{
    SignalAccumulator   *accumulator;
    Emission            emission;
    xClosure            *class_closure;
    HandlerList         *hlist;
    Handler *handler_list = NULL;
    xValue *return_accu, accu = X_VALUE_INIT;
    xuint signal_id;
    xulong max_handler_id;
    xbool   return_value_altered = FALSE;

#ifdef	X_ENABLE_DEBUG
    IF_DEBUG (SIGNALS, x_trace_instance_signals == instance || x_trap_instance_signals == instance)
    {
        x_message ("%s::%s(%u) emitted (instance=%p, signal-node=%p)",
                   type_debug_name_I (X_INSTANCE_TYPE (instance)),
                   node->name, detail,
                   instance, node);
        if (x_trap_instance_signals == instance)
            BREAKPOINT ();
    }
#endif	/* ENABLE_DEBUG */

    TRACE(GOBJECT_SIGNAL_EMIT(node->signal_id,
                              detail,
                              instance,
                              X_INSTANCE_TYPE (instance)));

    X_LOCK (signal);
    signal_id = node->signal_id;
    if (node->flags & X_SIGNAL_NO_RECURSE) {
        Emission *node = emission_find_L (_restart_emissions,
                                          signal_id, detail, instance);

        if (node) {
            node->state = EMISSION_RESTART;
            X_UNLOCK (signal);
            return return_value_altered;
        }
    }
    accumulator = node->accumulator;
    if (accumulator) {
        X_UNLOCK (signal);
        x_value_init (&accu, node->return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE);
        return_accu = &accu;
        X_LOCK (signal);
    }
    else
        return_accu = emission_return;
    emission.instance = instance;
    emission.ihint.signal_id = node->signal_id;
    emission.ihint.detail = detail;
    emission.ihint.run_type = 0;
    emission.state = 0;
    emission.chain_type = X_TYPE_VOID;
    emission_push_L ((node->flags & X_SIGNAL_NO_RECURSE)
                     ? &_restart_emissions : &_recursive_emissions,
                     &emission);
    class_closure = lookup_closure (node, X_INSTANCE_TYPE(instance));

EMIT_RESTART:

    if (handler_list)
        handler_unref_R (signal_id, instance, handler_list);
    max_handler_id = _n_handlers;
    hlist = handler_list_lookup_L (signal_id, instance);
    handler_list = hlist ? hlist->handlers : NULL;
    if (handler_list)
        handler_ref (handler_list);

    emission.ihint.run_type = X_SIGNAL_RUN_FIRST;

    if ((node->flags & X_SIGNAL_RUN_FIRST) && class_closure) {
        emission.state = EMISSION_RUN;

        emission.chain_type = X_INSTANCE_TYPE (instance);
        X_UNLOCK (signal);
        x_closure_invoke (class_closure,
                          return_accu,
                          node->n_params + 1,
                          instance_and_params,
                          &emission.ihint);
        if (!accumulate (&emission.ihint, emission_return, &accu, accumulator) &&
            emission.state == EMISSION_RUN)
            emission.state = EMISSION_STOP;
        X_LOCK (signal);
        emission.chain_type = X_TYPE_VOID;
        return_value_altered = TRUE;

        if (emission.state == EMISSION_STOP)
            goto EMIT_CLEANUP;
        else if (emission.state == EMISSION_RESTART)
            goto EMIT_RESTART;
    }

    if (node->emission_hooks)
    {
        xbool need_destroy, was_in_call, may_recurse = TRUE;
        xHook *hook;

        emission.state = EMISSION_HOOK;
        hook = x_hook_first_valid (node->emission_hooks, may_recurse);
        while (hook)
        {
            SignalHook *signal_hook = SIGNAL_HOOK (hook);

            if (!signal_hook->detail || signal_hook->detail == detail)
            {
                xSignalEmissionHook hook_func = (xSignalEmissionHook) hook->func;

                was_in_call = X_HOOK_IN_CALL (hook);
                hook->flags |= X_HOOK_FLAG_IN_CALL;
                X_UNLOCK (signal);
                need_destroy = !hook_func (&emission.ihint, node->n_params + 1, instance_and_params, hook->data);
                X_LOCK (signal);
                if (!was_in_call)
                    hook->flags &= ~X_HOOK_FLAG_IN_CALL;
                if (need_destroy)
                    x_hook_destroy_link (node->emission_hooks, hook);
            }
            hook = x_hook_next_valid (node->emission_hooks, hook, may_recurse);
        }

        if (emission.state == EMISSION_RESTART)
            goto EMIT_RESTART;
    }

    if (handler_list)
    {
        Handler *handler = handler_list;

        emission.state = EMISSION_RUN;
        handler_ref (handler);
        do
        {
            Handler *tmp;

            if (handler->after)
            {
                handler_unref_R (signal_id, instance, handler_list);
                handler_list = handler;
                break;
            }
            else if (!handler->block_count && (!handler->detail || handler->detail == detail) &&
                     handler->handler_id < max_handler_id)
            {
                X_UNLOCK (signal);
                x_closure_invoke (handler->closure,
                                  return_accu,
                                  node->n_params + 1,
                                  instance_and_params,
                                  &emission.ihint);
                if (!accumulate (&emission.ihint, emission_return, &accu, accumulator) &&
                    emission.state == EMISSION_RUN)
                    emission.state = EMISSION_STOP;
                X_LOCK (signal);
                return_value_altered = TRUE;

                tmp = emission.state == EMISSION_RUN ? handler->next : NULL;
            }
            else
                tmp = handler->next;

            if (tmp)
                handler_ref (tmp);
            handler_unref_R (signal_id, instance, handler_list);
            handler_list = handler;
            handler = tmp;
        }
        while (handler);

        if (emission.state == EMISSION_STOP)
            goto EMIT_CLEANUP;
        else if (emission.state == EMISSION_RESTART)
            goto EMIT_RESTART;
    }

    emission.ihint.run_type = X_SIGNAL_RUN_LAST;

    if ((node->flags & X_SIGNAL_RUN_LAST) && class_closure)
    {
        emission.state = EMISSION_RUN;

        emission.chain_type = X_INSTANCE_TYPE (instance);
        X_UNLOCK (signal);
        x_closure_invoke (class_closure,
                          return_accu,
                          node->n_params + 1,
                          instance_and_params,
                          &emission.ihint);
        if (!accumulate (&emission.ihint, emission_return, &accu, accumulator) &&
            emission.state == EMISSION_RUN)
            emission.state = EMISSION_STOP;
        X_LOCK (signal);
        emission.chain_type = X_TYPE_VOID;
        return_value_altered = TRUE;

        if (emission.state == EMISSION_STOP)
            goto EMIT_CLEANUP;
        else if (emission.state == EMISSION_RESTART)
            goto EMIT_RESTART;
    }

    if (handler_list)
    {
        Handler *handler = handler_list;

        emission.state = EMISSION_RUN;
        handler_ref (handler);
        do
        {
            Handler *tmp;

            if (handler->after && !handler->block_count && (!handler->detail || handler->detail == detail) &&
                handler->handler_id < max_handler_id)
            {
                X_UNLOCK (signal);
                x_closure_invoke (handler->closure,
                                  return_accu,
                                  node->n_params + 1,
                                  instance_and_params,
                                  &emission.ihint);
                if (!accumulate (&emission.ihint, emission_return, &accu, accumulator) &&
                    emission.state == EMISSION_RUN)
                    emission.state = EMISSION_STOP;
                X_LOCK (signal);
                return_value_altered = TRUE;

                tmp = emission.state == EMISSION_RUN ? handler->next : NULL;
            }
            else
                tmp = handler->next;

            if (tmp)
                handler_ref (tmp);
            handler_unref_R (signal_id, instance, handler);
            handler = tmp;
        }
        while (handler);

        if (emission.state == EMISSION_STOP)
            goto EMIT_CLEANUP;
        else if (emission.state == EMISSION_RESTART)
            goto EMIT_RESTART;
    }

EMIT_CLEANUP:

    emission.ihint.run_type = X_SIGNAL_RUN_CLEANUP;

    if ((node->flags & X_SIGNAL_RUN_CLEANUP) && class_closure)
    {
        xbool need_unset = FALSE;

        emission.state = EMISSION_STOP;

        emission.chain_type = X_INSTANCE_TYPE (instance);
        X_UNLOCK (signal);
        if (node->return_type != X_TYPE_VOID && !accumulator)
        {
            x_value_init (&accu, node->return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE);
            need_unset = TRUE;
        }
        x_closure_invoke (class_closure,
                          node->return_type != X_TYPE_VOID ? &accu : NULL,
                          node->n_params + 1,
                          instance_and_params,
                          &emission.ihint);
        if (need_unset)
            x_value_clear (&accu);
        X_LOCK (signal);
        emission.chain_type = X_TYPE_VOID;

        if (emission.state == EMISSION_RESTART)
            goto EMIT_RESTART;
    }

    if (handler_list)
        handler_unref_R (signal_id, instance, handler_list);

    emission_pop_L ((node->flags & X_SIGNAL_NO_RECURSE)
                    ? &_restart_emissions : &_recursive_emissions,
                    &emission);
    X_UNLOCK (signal);
    if (accumulator)
        x_value_clear (&accu);

    TRACE(GOBJECT_SIGNAL_EMIT_END(node->signal_id,
                                  detail,
                                  instance, 
                                  X_INSTANCE_TYPE (instance)));

    return return_value_altered;
}
xuint
x_signal_new            (xcstr          signal_name,
                         xType          itype,
                         xSignalFlags   signal_flags,
                         xuint          class_offset,
                         xSignalAccumulator	 accumulator,
                         xptr		    accu_data,
                         xType          return_type,
                         xuint          n_params,
                         ...)
{
    va_list     args;
    xuint       signal_id;

    x_return_val_if_fail (signal_name != NULL, 0);

    va_start (args, n_params);

    signal_id = x_signal_new_valist (signal_name, itype, signal_flags,
                                     !class_offset ?  NULL:
                                     x_type_cclosure_new (itype,
                                                          class_offset),
                                     accumulator, accu_data,
                                     return_type, n_params, args);

    va_end (args);

    return signal_id;
}
xuint
x_signal_new_valist     (xcstr          signal_name,
                         xType          itype,
                         xSignalFlags   signal_flags,
                         xClosure       *class_closure,
                         xSignalAccumulator accumulator,
                         xptr		    accu_data,
                         xType          return_type,
                         xuint          n_params,
                         va_list        args)
{
    xType   *param_types;
    xuint   i;
    xuint   signal_id;

    if (n_params > 0) {
        param_types = x_new (xType, n_params);

        for (i = 0; i < n_params; i++)
            param_types[i] = va_arg (args, xType);
    }
    else
        param_types = NULL;

    signal_id = x_signal_newv (signal_name, itype, signal_flags,
                               class_closure, accumulator, accu_data,
                               return_type, n_params, param_types);
    x_free (param_types);

    return signal_id;
}

xuint
x_signal_newv           (xcstr          signal_name,
                         xType          type,
                         xSignalFlags   signal_flags,
                         xClosure       *class_closure,
                         xSignalAccumulator accumulator,
                         xptr		    accu_data,
                         xType		    return_type,
                         xuint          n_params,
                         xType		    *param_types)
{
    xchar *name;
    xuint signal_id, i;
    SignalNode *node;

    x_return_val_if_fail (signal_name != NULL, 0);
    x_return_val_if_fail (X_TYPE_IS_INSTANTIATABLE (type)
                          || X_TYPE_IS_IFACE (type), 0);
    if (n_params)
        x_return_val_if_fail (param_types != NULL, 0);
    x_return_val_if_fail ((return_type & X_SIGNAL_TYPE_STATIC_SCOPE) == 0, 0);

    if (return_type == (X_TYPE_VOID & ~X_SIGNAL_TYPE_STATIC_SCOPE))
        x_return_val_if_fail (accumulator == NULL, 0);
    if (!accumulator)
        x_return_val_if_fail (accu_data == NULL, 0);

    name = x_strdup (signal_name);
    x_strdelimit (name, "_-|> <.:^", '_');

    X_LOCK(signal);
    signal_id = lookup_signal_id_L (x_quark_try (name), type);
    node = lookup_signal_node_L (signal_id);
    if (node && !node->destroyed) {
        x_warning ("%s: signal \"%s\" already exists in the `%s' %s",
                   X_STRLOC,
                   name,
                   type_debug_name_I (node->type),
                   X_TYPE_IS_IFACE (node->type) ? "iface" : "class");
        X_UNLOCK(signal);
        x_free (name);
        return 0;
    }
    if (node && node->type != type) {
        x_warning ("%s: signal \"%s\" for type `%s' was previously "
                   "created for type `%s'",
                   X_STRLOC,
                   name, type_debug_name_I (type),
                   type_debug_name_I (node->type));
        X_UNLOCK(signal);
        x_free (name);
        return 0;
    }
    for (i = 0; i < n_params; i++)
        if (!X_TYPE_IS_VALUE (param_types[i] & ~X_SIGNAL_TYPE_STATIC_SCOPE)) {
            x_warning ("%s: parameter %d of type `%s' for signal "
                       "\"%s::%s\" is not a value type",
                       X_STRLOC,
                       i + 1, type_debug_name_I (param_types[i]),
                       type_debug_name_I (type), name);
            X_UNLOCK(signal);
            x_free (name);
            return 0;
        }
    if (return_type != X_TYPE_VOID &&
        !X_TYPE_IS_VALUE (return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE)) {
        x_warning ("%s: return value of type `%s' for signal "
                   "\"%s::%s\" is not a value type",
                   X_STRLOC,
                   type_debug_name_I (return_type),
                   type_debug_name_I (type), name);
        X_UNLOCK(signal);
        x_free (name);
        return 0;
    }
    if (return_type != X_TYPE_VOID &&
        (signal_flags & (X_SIGNAL_RUN_FIRST |
                         X_SIGNAL_RUN_LAST | X_SIGNAL_RUN_CLEANUP))
        == X_SIGNAL_RUN_FIRST) {
        x_warning ("%s: signal \"%s::%s\" has return type `%s' "
                   "and is only X_SIGNAL_RUN_FIRST",
                   X_STRLOC,
                   type_debug_name_I (type),
                   name,
                   type_debug_name_I (return_type));
        X_UNLOCK(signal);
        x_free (name);
        return 0;
    }

    if (!node) {
        SignalKey *key;

        signal_id = _n_signal_nodes++;
        node = x_new (SignalNode, 1);
        node->signal_id = signal_id;
        _signal_nodes = x_renew (SignalNode*, _signal_nodes, _n_signal_nodes);
        _signal_nodes[signal_id] = node;
        node->type = type;
        node->name = name;
        key = x_slice_new (SignalKey);
        key->type = type;
        key->name = x_quark_from (node->name);
        key->signal_id = signal_id;
        x_hash_table_add (_signal_key_ht, key);
        x_strdelimit (name, "_", '-');
        node->name = x_intern_str (name);
        key = x_slice_new (SignalKey);
        key->type = type;
        key->name = x_quark_from (node->name);
        key->signal_id = signal_id;
        x_hash_table_add (_signal_key_ht, key);
        TRACE(GOBJECT_SIGNAL_NEW(signal_id, name, type));
    }
    node->destroyed = FALSE;

    node->single_va_closure_is_valid = FALSE;
    node->flags = signal_flags & X_SIGNAL_FLAGS_MASK;
    node->n_params = n_params;
    node->param_types = x_memdup (param_types, sizeof (xType) * n_params);
    node->return_type = return_type;
    node->class_closure_ht = NULL;
    if (accumulator) {
        node->accumulator = x_new (SignalAccumulator, 1);
        node->accumulator->func = accumulator;
        node->accumulator->data = accu_data;
    }
    else
        node->accumulator = NULL;

    node->c_marshaller = _x_cclosure_marshal;
    node->va_marshaller = _x_closure_va_marshal;
    node->emission_hooks = NULL;
    if (class_closure)
        add_class_closure (node, 0, class_closure);

    X_UNLOCK(signal);
    x_free (name);

    return signal_id;
}
xuint
x_signal_connect_data   (xptr           instance,
                         xcstr          detailed_signal,
                         xCallback      c_handler,
                         xptr           data,
                         xNotify        destroy_data,
                         xuint          connect_flags)
{
    xuint signal_id;
    xuint handler_id = 0;
    xQuark detail = 0;
    xType itype;
    xbool swapped, after;

    x_return_val_if_fail (X_INSTANCE_CHECK (instance), 0);
    x_return_val_if_fail (detailed_signal != NULL, 0);
    x_return_val_if_fail (c_handler != NULL, 0);

    swapped = (connect_flags & X_SIGNAL_CONNECT_SWAPPED) != FALSE;
    after = (connect_flags & X_SIGNAL_CONNECT_AFTER) != FALSE;

    X_LOCK (signal);
    itype = X_INSTANCE_TYPE (instance);
    signal_id = parse_signal_name_L (detailed_signal, itype, &detail, TRUE);
    if (signal_id) {
        SignalNode *node = lookup_signal_node_L (signal_id);

        node_check_deprecated (node);

        if (detail && !(node->flags & X_SIGNAL_DETAILED))
            x_warning ("%s: signal `%s' does not support details",
                       X_STRLOC,
                       detailed_signal);
        else if (!x_type_is (itype, node->type))
            x_warning ("%s: signal `%s' is invalid for instance `%p'",
                       X_STRLOC,
                       detailed_signal, instance);
        else {
            Handler *handler = handler_new_L (after);

            handler_id = handler->handler_id;
            handler->detail = detail;
            handler->closure = x_closure_ref ((swapped ? x_cclosure_new_swap : x_cclosure_new)
                                              (c_handler, data, destroy_data));
            x_closure_sink (handler->closure);
            handler_insert_L (signal_id, instance, handler);
            if (node->c_marshaller && X_CLOSURE_NEEDS_MARSHAL (handler->closure)) {
                x_closure_set_marshal (handler->closure, node->c_marshaller);
                if (node->va_marshaller)
                    _x_closure_set_va_marshal (handler->closure, node->va_marshaller);
            }
        }
    }
    else
        x_warning ("%s: signal `%s' is invalid for instance `%p'",
                   X_STRLOC,
                   detailed_signal, instance);
    X_UNLOCK (signal);

    return handler_id;
}

void
x_signal_emit           (xptr           instance,
                         xuint          signal_id,
                         xQuark         detail,
                         ...)
{
    va_list args;

    va_start (args, detail);
    x_signal_emit_valist (instance, signal_id, detail, args);
    va_end (args);
}

void
x_signal_emit_by_name   (xptr           instance,
                         xcstr          detailed_signal,
                         ...)
{
    xQuark detail = 0;
    xuint signal_id;

    x_return_if_fail (X_INSTANCE_CHECK (instance));
    x_return_if_fail (detailed_signal != NULL);

    X_LOCK(signal);
    signal_id = parse_signal_name_L (detailed_signal,
                                     X_INSTANCE_TYPE (instance),
                                     &detail, TRUE);
    X_UNLOCK(signal);

    if (signal_id) {
        va_list var_args;

        va_start (var_args, detailed_signal);
        x_signal_emit_valist (instance, signal_id, detail, var_args);
        va_end (var_args);
    }
    else
        x_warning ("%s: signal name `%s' is invalid for instance `%p'",
                   X_STRLOC, detailed_signal, instance);
}
void
x_signal_emit_valist    (xptr           instance,
                         xuint          signal_id,
                         xQuark         detail,
                         va_list        var_args)
{
    xValue *instance_and_params;
    xType signal_return_type;
    xValue *param_values;
    SignalNode *node;
    xuint i, n_params;

    x_return_if_fail (X_INSTANCE_CHECK (instance));
    x_return_if_fail (signal_id > 0);

    X_LOCK (signal);
    node = lookup_signal_node_L (signal_id);
    if (!node || !x_type_is (X_INSTANCE_TYPE (instance), node->type)) {
        x_warning ("%s: signal id `%u' is invalid for instance `%p'",
                   X_STRLOC, signal_id, instance);
        X_UNLOCK (signal);
        return;
    }
#ifndef X_DISABLE_CHECKS
    if (detail && !(node->flags & X_SIGNAL_DETAILED)) {
        x_warning ("%s: signal id `%u' does not support detail (%s)",
                   X_STRLOC, signal_id, x_quark_str(detail));
        X_UNLOCK (signal);
        return;
    }
#endif  /* !X_DISABLE_CHECKS */

    if (!node->single_va_closure_is_valid)
        node_update_single_va_closure (node);

    if (node->single_va_closure != NULL
#ifdef	X_ENABLE_DEBUG
        && !COND_DEBUG (SIGNALS, x_trace_instance_signals != instance &&
                        x_trap_instance_signals == instance)
#endif	/* X_ENABLE_DEBUG */
       ) {
        HandlerList* hlist = handler_list_lookup_L (node->signal_id, instance);
        Handler *l;
        xClosure *closure = NULL;
        xbool   fastpath = TRUE;
        xSignalFlags run_type = X_SIGNAL_RUN_FIRST;

        if (node->single_va_closure != SINGLE_VA_CLOSURE_EMPTY_MAGIC &&
            !_x_closure_is_void (node->single_va_closure, instance)) {
            if (_x_closure_supports_invoke_va (node->single_va_closure)) {
                closure = node->single_va_closure;
                if (node->single_va_closure_is_after)
                    run_type = X_SIGNAL_RUN_LAST;
                else
                    run_type = X_SIGNAL_RUN_FIRST;
            }
            else
                fastpath = FALSE;
        }

        for (l = hlist ? hlist->handlers : NULL; fastpath && l != NULL; l = l->next) {
            if (!l->block_count &&
                (!l->detail || l->detail == detail)) {
                if (closure != NULL || !_x_closure_supports_invoke_va (l->closure)) {
                    fastpath = FALSE;
                    break;
                }
                else {
                    closure = l->closure;
                    if (l->after)
                        run_type = X_SIGNAL_RUN_LAST;
                    else
                        run_type = X_SIGNAL_RUN_FIRST;
                }
            }
        }

        if (fastpath && closure == NULL && node->return_type == X_TYPE_VOID) {
            X_UNLOCK (signal);
            return;
        }

        /* Don't allow no-recurse emission as we might have to restart, which means
           we will run multiple handlers and thus must ref all arguments */
        if (closure != NULL && (node->flags & (X_SIGNAL_NO_RECURSE)) != 0)
            fastpath = FALSE;

        if (fastpath) {
            SignalAccumulator *accumulator;
            Emission emission;
            xValue *return_accu, accu = X_VALUE_INIT;
            xuint signal_id;
            xType instance_type = X_INSTANCE_TYPE (instance);
            xValue emission_return = X_VALUE_INIT;
            xType rtype = node->return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE;
            xbool static_scope = node->return_type & X_SIGNAL_TYPE_STATIC_SCOPE;

            signal_id = node->signal_id;
            accumulator = node->accumulator;
            if (rtype == X_TYPE_VOID)
                return_accu = NULL;
            else if (accumulator)
                return_accu = &accu;
            else
                return_accu = &emission_return;

            emission.instance = instance;
            emission.ihint.signal_id = signal_id;
            emission.ihint.detail = detail;
            emission.ihint.run_type = run_type;
            emission.state = EMISSION_RUN;
            emission.chain_type = instance_type;
            emission_push_L (&_recursive_emissions, &emission);

            X_UNLOCK (signal);

            TRACE(GOBJECT_SIGNAL_EMIT(signal_id, detail, instance, instance_type));

            if (rtype != X_TYPE_VOID)
                x_value_init (&emission_return, rtype);

            if (accumulator)
                x_value_init (&accu, rtype);

            if (closure != NULL)
            {
                x_object_ref (instance);
                _x_closure_invoke_va (closure,
                                      return_accu,
                                      instance,
                                      var_args,
                                      node->n_params,
                                      node->param_types);
                accumulate (&emission.ihint, &emission_return, &accu, accumulator);
                x_object_unref (instance);
            }

            X_LOCK (signal);

            emission.chain_type = X_TYPE_VOID;
            emission_pop_L (&_recursive_emissions, &emission);

            X_UNLOCK (signal);

            if (accumulator)
                x_value_clear (&accu);

            if (rtype != X_TYPE_VOID)
            {
                xchar *error = NULL;
                for (i = 0; i < node->n_params; i++)
                {
                    xType ptype = node->param_types[i] & ~X_SIGNAL_TYPE_STATIC_SCOPE;
                    X_VALUE_COLLECT_SKIP(ptype, var_args);
                }

                X_VALUE_LCOPY(&emission_return,
                              var_args,
                              static_scope ? X_VALUE_NOCOPY_CONTENTS : 0,
                              error);
                if (!error)
                    x_value_clear (&emission_return);
                else
                {
                    x_warning ("%s: %s", X_STRLOC, error);
                    x_free (error);
                    /* we purposely leak the value here, it might not be
                     * in a sane state if an error condition occurred
                     */
                }
            }

            TRACE(GOBJECT_SIGNAL_EMIT_END(signal_id, detail, instance, instance_type));

            return;
        }
    }

    n_params = node->n_params;
    signal_return_type = node->return_type;
    instance_and_params = x_alloca (sizeof (xValue) * (n_params + 1));
    memset (instance_and_params, 0, sizeof (xValue) * (n_params + 1));
    param_values = instance_and_params + 1;

    for (i = 0; i < node->n_params; i++)
    {
        xstr error;
        xType ptype = node->param_types[i] & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        xbool static_scope = node->param_types[i] & X_SIGNAL_TYPE_STATIC_SCOPE;

        X_UNLOCK (signal);
        X_VALUE_COLLECT_INIT (param_values + i,ptype,
                              var_args,
                              static_scope ? X_VALUE_NOCOPY_CONTENTS : 0,
                              error);
        if (error)
        {
            x_warning ("%s: %s", X_STRLOC, error);
            x_free (error);

            /* we purposely leak the value here, it might not be
             * in a sane state if an error condition occoured
             */
            while (i--)
                x_value_clear (param_values + i);

            return;
        }
        X_LOCK (signal);
    }
    X_UNLOCK (signal);

    instance_and_params->type = 0;
    x_value_init (instance_and_params, X_INSTANCE_TYPE (instance));
    x_value_set_instance (instance_and_params, instance);
    if (signal_return_type == X_TYPE_VOID)
        signal_emit_unlocked_R (node, detail, instance, NULL,
                                instance_and_params);
    else
    {
        xValue return_value;
        xchar *error = NULL;
        xType rtype = signal_return_type & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        xbool static_scope = signal_return_type & X_SIGNAL_TYPE_STATIC_SCOPE;

        x_value_init (&return_value, rtype);

        signal_emit_unlocked_R (node, detail, instance, &return_value,
                                instance_and_params);

        X_VALUE_LCOPY(&return_value,
                      var_args,
                      static_scope ? X_VALUE_NOCOPY_CONTENTS : 0,
                      error);
        if (!error)
            x_value_clear (&return_value);
        else
        {
            x_warning ("%s: %s", X_STRLOC, error);
            x_free (error);

            /* we purposely leak the value here, it might not be
             * in a sane state if an error condition occurred
             */
        }
    }
    for (i = 0; i < n_params; i++)
        x_value_clear (param_values + i);
    x_value_clear (instance_and_params);
}
void
x_signal_clean          (xptr           instance)
{
}
void
_x_signals_destroy      (xType itype)
{
    xuint i;

    X_LOCK (signal);
    for (i = 1; i < _n_signal_nodes; i++) {
        SignalNode *node = _signal_nodes[i];

        if (node->type == itype) {
            if (node->destroyed)
                x_warning ("%s: signal \"%s\" of type `%s' already destroyed",
                           X_STRLOC,
                           node->name,
                           type_debug_name_I (node->type));
            else
                signal_destroy_R (node);
        }
    }
    X_UNLOCK (signal);
}
void
_x_signal_init          (void)
{
    X_LOCK (signal);
    if (!_n_signal_nodes) {
        /* setup handler list binary searchable array hash table (in german, that'd be one word ;) */
        _hlist_array_ht = x_hash_table_new_full (x_direct_hash, x_direct_cmp,
                                                 NULL, (xFreeFunc)x_array_unref);
        _signal_key_ht = x_hash_table_new_full (signal_key_hash, signal_key_cmp,
                                                signal_key_free, NULL);

        /* invalid (0) signal_id */
        _n_signal_nodes = 1;
        _signal_nodes = x_renew (SignalNode*, _signal_nodes, _n_signal_nodes);
        _signal_nodes[0] = NULL;
    }
    X_UNLOCK (signal);
}
xbool
x_singal_accumulator_or (xSignalInvocationHint *ihint,
                         xValue                *return_accu,
                         const xValue          *handler_return,
                         xptr                  dummy)
{
    xbool   continue_emission;
    xbool   signal_handled;

    signal_handled = x_value_get_bool (handler_return);
    x_value_set_bool (return_accu, signal_handled);
    continue_emission = !signal_handled;

    return continue_emission;
}
void
x_signal_disconnect     (xptr           instance,
                         xulong         handler_id)
{
    Handler *handler;
    xuint signal_id;

    x_return_if_fail (X_INSTANCE_CHECK (instance));
    x_return_if_fail (handler_id > 0);

    X_LOCK (signal);
    handler = handler_lookup (instance, handler_id, &signal_id);
    if (handler) {
        handler->handler_id = 0;
        handler->block_count = 1;
        handler_unref_R (signal_id, instance, handler);
    }
    else
        x_warning ("%s: instance `%p' has no handler with id `%lu'",
                   X_STRLOC, instance, handler_id);
    X_UNLOCK (signal);
}

xsize
x_signal_disconnect_matched (xptr       instance,
                             xuint      mask,
                             xuint      signal_id,
                             xQuark     detail,
                             xClosure   *closure,
                             xptr       func,
                             xptr       data)
{
    xsize n_handlers = 0;

    x_return_val_if_fail (X_INSTANCE_CHECK (instance), 0);
    x_return_val_if_fail ((mask & ~X_SIGNAL_MATCH_MASK) == 0, 0);

    X_LOCK (signal);

    n_handlers = handler_foreach_matched_R (instance, mask, signal_id, detail,
                                            closure, func, data,
                                            x_signal_disconnect);
    X_UNLOCK (signal);

    return n_handlers;
}
void
x_signal_block          (xptr           instance,
                         xulong         handler_id)
{
    Handler *handler;

    x_return_if_fail (X_INSTANCE_CHECK (instance));
    x_return_if_fail (handler_id > 0);

    X_LOCK (signal);
    handler = handler_lookup (instance, handler_id, NULL);
    if (handler)
    {
        if (handler->block_count >= HANDLER_MAX_BLOCK_COUNT - 1)
            x_error ("%s: handler block_count overflow", X_STRLOC);
        handler->block_count ++;
    }
    else
        x_warning ("%s: instance `%p' has no handler with id `%lu'",
                   X_STRLOC, instance, handler_id);
    X_UNLOCK (signal);
}

void
x_signal_unblock        (xptr           instance,
                         xulong         handler_id)
{
    Handler *handler;

    x_return_if_fail (X_INSTANCE_CHECK (instance));
    x_return_if_fail (handler_id > 0);

    X_LOCK (signal);
    handler = handler_lookup (instance, handler_id, NULL);
    if (handler) {
        if (handler->block_count)
            handler->block_count --;
        else
            x_warning ("%s: handler `%lu' of instance `%p' is not blocked",
                       X_STRLOC, handler_id, instance);
    }
    else
        x_warning ("%s: instance `%p' has no handler with id `%lu'",
                   X_STRLOC, instance, handler_id);
    X_UNLOCK (signal);
}

xsize
x_signal_block_matched  (xptr           instance,
                         xuint          mask,
                         xuint          signal_id,
                         xQuark         detail,
                         xClosure       *closure,
                         xptr           func,
                         xptr           data)
{
    xsize n_handlers = 0;

    x_return_val_if_fail (X_INSTANCE_CHECK (instance), 0);
    x_return_val_if_fail ((mask & ~X_SIGNAL_MATCH_MASK) == 0, 0);

    mask |= X_SIGNAL_MATCH_UNBLOCKED;

    X_LOCK (signal);
    n_handlers = handler_foreach_matched_R (instance, mask, signal_id, detail,
                                            closure, func, data,
                                            x_signal_block);
    X_UNLOCK (signal);

    return n_handlers;
}

xsize
x_signal_unblock_matched(xptr           instance,
                         xuint          mask,
                         xuint          signal_id,
                         xQuark         detail,
                         xClosure       *closure,
                         xptr           func,
                         xptr           data)
{
    xsize n_handlers = 0;

    x_return_val_if_fail (X_INSTANCE_CHECK (instance), 0);
    x_return_val_if_fail ((mask & ~X_SIGNAL_MATCH_MASK) == 0, 0);

    mask |= X_SIGNAL_MATCH_BLOCKED;

    X_LOCK (signal);
    n_handlers = handler_foreach_matched_R (instance, mask, signal_id, detail,
                                            closure, func, data,
                                            x_signal_unblock);
    X_UNLOCK (signal);

    return n_handlers;
}
