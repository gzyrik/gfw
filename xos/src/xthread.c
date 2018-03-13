#include "config.h"
#include "xthread.h"
#include "xatomic.h"
#include "xmsg.h"
#include "xtest.h"
#include "xmem.h"
#include "xstr.h"
#include "xerror.h"
#include "xslice.h"
#include "xquark.h"
#include "xslist.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
typedef struct {
    /*< private >*/
    xThread     thread;
    xint        ref_count;
    xbool       ours;
    xstr        name;
    xptr        retval;
} RealThread;
static void
thread_abort            (xint           status,
                         xcstr          function)
{
    fprintf (stderr, "(%s): "
             "Unexpected error from C library during '%s': %s.  Aborting.\n",
             __FILE__, function, strerror (status));
    abort ();
}
/* system thread implementation (xthread-posix.c, xthread-win32.c) */
static void
system_thread_set_name  (xcstr          name);
static void
system_thread_free      (xThread        *thread);
static void
system_thread_wait      (xThread        *thread);
static xptr
thread_proxy            (xptr           data);
static void
system_thread_exit      (void);
static xThread*
system_thread_new       (xThreadFunc    func,
                         xsize          stack_size,
                         xError         **error);
static xThread*
thread_new_internal     (xcstr          name,
                         xThreadFunc    proxy,
                         xThreadFunc    func,
                         xptr           data,
                         xsize          stack_size,
                         xError         **error);

static void thread_cleanup (xptr data);

#ifdef X_OS_WINDOWS
#include "xthread-win.h"
#else
#include "xthread-posix.h"
#endif

static xSList*      _once_slist;
static xCond        _once_cond;
static xPrivate     _once_private;
static xPrivate     _thread_private = X_PRIVATE_INIT (thread_cleanup);

X_LOCK_DEFINE_STATIC (thread_new);
X_LOCK_DEFINE_STATIC (once);

xbool
x_once_init_enter       (volatile void  *location)
{
    xbool need_init;
    volatile xsize *val_location;
    xSList* tls_slist;

    tls_slist = x_private_get (&_once_private);
    if (x_slist_find (tls_slist, (xcptr)location))
        return FALSE;

    val_location = location;
    need_init = FALSE;
    X_LOCK (once);
    if (x_atomic_ptr_get (val_location) == NULL) {
        if (!x_slist_find (_once_slist, (xcptr)location)) {
            need_init = TRUE;
            _once_slist = x_slist_prepend (_once_slist, (xptr)location);
            tls_slist = x_slist_prepend(tls_slist, (xptr)location);
            x_private_set (&_once_private, tls_slist);
        }
        else {
            do {
                x_cond_wait (&_once_cond, &X_LOCK_NAME (once));
            } while (x_slist_find (_once_slist, (xcptr)location));
        }
    }
    X_UNLOCK (once);
    return need_init;
}

void
x_once_init_leave       (volatile void  *location)
{
    xSList* tls_slist;
    volatile xsize *val_location = location;

    x_return_if_fail (x_atomic_ptr_get(val_location) != NULL);
    tls_slist = x_private_get (&_once_private);
    tls_slist = x_slist_remove(tls_slist, (xptr)location);
    x_private_set (&_once_private, tls_slist);

    X_LOCK (once);
    _once_slist = x_slist_remove (_once_slist, (xcptr)location);
    x_cond_broadcast (&_once_cond);
    X_UNLOCK (once);
}

static void
thread_cleanup          (xptr           data)
{
    x_thread_unref (data);
}

static xptr
thread_proxy            (xptr           data)
{
    RealThread* real = data;
    x_assert (data);

    /* This has to happen before G_LOCK, as that might call g_thread_self */
    x_private_set (&_thread_private, data);
    /* The lock makes sure that g_thread_new_internal() has a chance to
     * setup 'func' and 'data' before we make the call.
     */
    X_LOCK (thread_new);
    X_UNLOCK (thread_new);
    if (real->name) {
        system_thread_set_name (real->name);
        x_free (real->name);
        real->name = NULL;
    }

    real->retval = real->thread.func (real->thread.data);

    return NULL;
}

xQuark
x_thread_domain         (void)
{
    static xQuark quark = 0;
    if (!x_atomic_ptr_get(&quark) && x_once_init_enter(&quark)) {
        quark = x_quark_from_static("xThread");
        x_once_init_leave (&quark);
    }
    return quark;
}

xThread*
x_thread_new            (xcstr          name,
                         xThreadFunc    func,
                         xptr           data)
{
    xError *error = NULL;
    xThread* thread;

    thread = thread_new_internal (name,
                                  X_PTR_FUNC(thread_proxy),
                                  func,
                                  data,
                                  0,
                                  &error);
    if X_UNLIKELY (thread == NULL)
        x_error ("creating thread '%s': %s", name ? name : "", error->message);

    return thread;
}

xThread*
x_thread_try_new        (xcstr          name,
                         xThreadFunc    func,
                         xptr           data,
                         xError         **error)
{
    return thread_new_internal (name, X_PTR_FUNC(thread_proxy), func, data, 0, error);
}

xThread*
x_thread_ref            (xThread        *thread)
{
    RealThread *real = (RealThread*)thread;
    x_return_val_if_fail (thread != NULL, 0);
    x_return_val_if_fail (x_atomic_int_get (&real->ref_count) > 0, NULL);

    x_atomic_int_inc (&real->ref_count);
    return thread;
}

xint
x_thread_unref          (xThread        *thread)
{
    xint ref_count;
    RealThread *real =(RealThread*)thread;
    x_return_val_if_fail (thread != NULL, -1);
    x_return_val_if_fail (x_atomic_int_get (&real->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&real->ref_count);
    if X_LIKELY(ref_count != 0)
        return ref_count;

    if (real->ours)
        system_thread_free (thread);
    else
        x_slice_free (RealThread, real);

    return ref_count;
}

static xThread*
thread_new_internal     (xcstr          name,
                         xThreadFunc    proxy,
                         xThreadFunc    func,
                         xptr           data,
                         xsize          stack_size,
                         xError         **error)
{
    RealThread *real;

    x_return_val_if_fail (func != NULL, NULL);

    X_LOCK (thread_new);
    real = (RealThread*)system_thread_new (proxy, stack_size, error);
    if (real) {
        real->ref_count = 2;
        real->ours = TRUE;
        real->thread.joinable = TRUE;
        real->thread.func = func;
        real->thread.data = data;
        real->name = x_strdup (name);
    }
    X_UNLOCK (thread_new);

    return (xThread*)real;
}

xThread*
x_thread_self           (void)
{
    RealThread* real;

    real = x_private_get (&_thread_private);

    if (!real) {
        real = x_slice_new0 (RealThread);
        real->ref_count = 1;

        x_private_set (&_thread_private, real);
    }

    return (xThread*)real;
}

void
x_thread_exit           (xptr           retval)
{
    RealThread* thread = (RealThread*)x_thread_self ();

    if X_UNLIKELY (!thread->ours)
        x_error ("attempt to x_thread_exit() a thread not created by xos");

    thread->retval = retval;

    system_thread_exit ();
}

xptr
x_thread_join           (xThread        *thread)
{
    xptr retval;
    RealThread *real;
    x_return_val_if_fail (thread, NULL);

    real = (RealThread*)thread;
    x_return_val_if_fail (real->ours, NULL);

    system_thread_wait (thread);

    retval = real->retval;

    /* Just to make sure, this isn't used any more */
    thread->joinable = 0;

    x_thread_unref (thread);

    return retval;
}
