#include <pthread.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

/* {{{1 xMutex */
static pthread_mutex_t *
mutex_impl_new          (void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_t *pattr = NULL;
    pthread_mutex_t *mutex;
    xint status;

    mutex = malloc (sizeof (pthread_mutex_t));
    if X_UNLIKELY (mutex == NULL)
        thread_abort (errno, "malloc");

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pattr = &attr;
#endif

    if X_UNLIKELY ((status = pthread_mutex_init (mutex, pattr)) != 0)
        thread_abort (status, "pthread_mutex_init");

#ifdef PTHREAD_ADAPTIVE_MUTEX_NP
    pthread_mutexattr_destroy (&attr);
#endif

    return mutex;
}
static void
mutex_impl_delete       (pthread_mutex_t *mutex)
{
    pthread_mutex_destroy (mutex);
    free (mutex);
}
static pthread_mutex_t*
mutex_get_impl          (xMutex         *mutex)
{
    pthread_mutex_t *impl = mutex->p;

    if X_UNLIKELY (impl == NULL) {
        impl = mutex_impl_new ();
        if (!x_atomic_ptr_cas (&mutex->p, NULL, impl))
            mutex_impl_delete (impl);
        impl = mutex->p;
    }

    return impl;
}
void
x_mutex_init            (xMutex         *mutex)
{
    mutex->p = mutex_impl_new ();
}
xbool
x_mutex_try_lock        (xMutex         *mutex)
{
    xint status;

    if X_LIKELY ((status = pthread_mutex_trylock (mutex_get_impl (mutex))) == 0)
        return TRUE;

    if X_UNLIKELY (status != EBUSY)
        thread_abort (status, "pthread_mutex_trylock");

    return FALSE;
}
void
x_mutex_lock            (xMutex         *mutex)
{
    xint status;

    if X_UNLIKELY ((status = pthread_mutex_lock (mutex_get_impl (mutex))) != 0)
        thread_abort (status, "pthread_mutex_lock");
}
void
x_mutex_unlock          (xMutex         *mutex)
{
    xint status;

    if X_UNLIKELY ((status = pthread_mutex_unlock (mutex_get_impl (mutex))) != 0)
        thread_abort (status, "pthread_mutex_unlock");
}
void
x_mutex_clear           (xMutex         *mutex)
{
    mutex_impl_delete (mutex->p);
}
/* {{{1 xRWLock */
static pthread_rwlock_t*
rwlock_impl_new         (void)
{
    pthread_rwlock_t *rwlock;
    xint status;

    rwlock = malloc (sizeof (pthread_rwlock_t));
    if X_UNLIKELY (rwlock == NULL)
        thread_abort (errno, "malloc");

    if X_UNLIKELY ((status = pthread_rwlock_init (rwlock, NULL)) != 0)
        thread_abort (status, "pthread_rwlock_init");

    return rwlock;
}
static void
rwlock_impl_delete      (pthread_rwlock_t *rwlock)
{
    pthread_rwlock_destroy (rwlock);
    free (rwlock);
}
static pthread_rwlock_t*
rwlock_get_impl         (xRWLock        *lock)
{
    pthread_rwlock_t *impl = lock->p;

    if X_UNLIKELY (impl == NULL) {
        impl = rwlock_impl_new ();
        if (!x_atomic_ptr_cas (&lock->p, NULL, impl))
            rwlock_impl_delete (impl);
        impl = lock->p;
    }

    return impl;
}
void
x_rwlock_init           (xRWLock        *rwlock)
{
    rwlock->p = rwlock_impl_new ();
}
xbool
x_rwlock_try_wlock      (xRWLock        *rwlock)
{
    if (pthread_rwlock_trywrlock (rwlock_get_impl (rwlock)) != 0)
        return FALSE;

    return TRUE;
}
void
x_rwlock_wlock          (xRWLock        *rwlock)
{
    pthread_rwlock_wrlock (rwlock_get_impl (rwlock));
}
void
x_rwlock_unwlock        (xRWLock        *rwlock)
{
    pthread_rwlock_unlock (rwlock_get_impl (rwlock));
}
xbool
x_rwlock_try_rlock      (xRWLock        *rwlock)
{
    if (pthread_rwlock_tryrdlock (rwlock_get_impl (rwlock)) != 0)
        return FALSE;

    return TRUE;
}
void
x_rwlock_rlock          (xRWLock        *rwlock)
{
    pthread_rwlock_rdlock (rwlock_get_impl (rwlock));
}
void
x_rwlock_unrlock        (xRWLock        *rwlock)
{
    pthread_rwlock_unlock (rwlock_get_impl (rwlock));
}
void
x_rwlock_clear          (xRWLock        *rwlock)
{
    rwlock_impl_delete (rwlock->p);
}
/* {{{1 xRecMutex */
static pthread_mutex_t*
recmutex_impl_new       (void)
{
    pthread_mutexattr_t attr;
    pthread_mutex_t *mutex;

    mutex = x_slice_new (pthread_mutex_t);
    pthread_mutexattr_init (&attr);
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init (mutex, &attr);
    pthread_mutexattr_destroy (&attr);

    return mutex;
}
static void
recmutex_impl_delete    (pthread_mutex_t *mutex)
{
    pthread_mutex_destroy (mutex);
    x_slice_free (pthread_mutex_t, mutex);
}
static pthread_mutex_t*
recmutex_get_impl       (xRecMutex      *rec_mutex)
{
    pthread_mutex_t *impl = rec_mutex->p;

    if X_UNLIKELY (impl == NULL) {
        impl = recmutex_impl_new ();
        if (!x_atomic_ptr_cas (&rec_mutex->p, NULL, impl))
            recmutex_impl_delete (impl);
        impl = rec_mutex->p;
    }

    return impl;
}
void
x_recmutex_init         (xRecMutex      *recmutex)
{
    recmutex->p = recmutex_impl_new ();
}
xbool
x_recmutex_try_lock     (xRecMutex      *recmutex)
{
    if (pthread_mutex_trylock (recmutex_get_impl (recmutex)) != 0)
        return FALSE;

    return TRUE;
}
void
x_recmutex_lock         (xRecMutex      *recmutex)
{
    pthread_mutex_lock (recmutex_get_impl (recmutex));
}
void
x_recmutex_unlock       (xRecMutex      *recmutex)
{
    pthread_mutex_unlock (recmutex->p);
}
void
x_recmutex_clear        (xRecMutex      *recmutex)
{
    recmutex_impl_delete (recmutex->p);
}
/* {{{1 xCond */
static pthread_cond_t*
cond_impl_new (void)
{
    pthread_condattr_t attr;
    pthread_cond_t *cond;
    xint status;

    pthread_condattr_init (&attr);
#if defined (HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined (CLOCK_MONOTONIC)
    pthread_condattr_setclock (&attr, CLOCK_MONOTONIC);
#endif

    cond = malloc (sizeof (pthread_cond_t));
    if X_UNLIKELY (cond == NULL)
        thread_abort (errno, "malloc");

    if X_UNLIKELY ((status = pthread_cond_init (cond, &attr)) != 0)
        thread_abort (status, "pthread_cond_init");

    pthread_condattr_destroy (&attr);

    return cond;
}

static void
cond_impl_delete        (pthread_cond_t *cond)
{
    pthread_cond_destroy (cond);
    free (cond);
}

static pthread_cond_t*
cond_get_impl           (xCond          *cond)
{
    pthread_cond_t *impl = cond->p;

    if X_UNLIKELY (impl == NULL) {
        impl = cond_impl_new ();
        if (!x_atomic_ptr_cas (&cond->p, NULL, impl))
            cond_impl_delete (impl);
        impl = cond->p;
    }

    return impl;
}
void
x_cond_init             (xCond          *cond)
{
    cond->p = cond_impl_new ();
}
xbool
x_cond_wait_timeout     (xCond          *cond,
                         xMutex         *mutex,
                         xuint32        timeout_ms)
{
    if (timeout_ms == X_MAXUINT32) {
        xint status;

        if X_UNLIKELY ((status = pthread_cond_wait (cond_get_impl (cond),
                                                    mutex_get_impl (mutex))) != 0)
            thread_abort (status, "pthread_cond_wait");
        return TRUE;
    }
    else {
        struct timeval now;
        struct timespec ts;
        xint status;

        gettimeofday(&now, NULL);

        ts.tv_sec = now.tv_sec + (timeout_ms / 1000);
        ts.tv_nsec = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;

        if ((status = pthread_cond_timedwait (cond_get_impl (cond),
                                              mutex_get_impl (mutex), &ts)) == 0)
            return TRUE;

        if X_UNLIKELY (status != ETIMEDOUT)
            thread_abort (status, "pthread_cond_timedwait");
    }

    return FALSE;
}
void
x_cond_signal           (xCond          *cond)
{
    xint status;

    if X_UNLIKELY ((status = pthread_cond_signal (cond_get_impl (cond))) != 0)
        thread_abort (status, "pthread_cond_signal");
}
void
x_cond_broadcast        (xCond          *cond)
{
    xint status;

    if X_UNLIKELY ((status = pthread_cond_broadcast (cond_get_impl (cond))) != 0)
        thread_abort (status, "pthread_cond_broadcast");
}
void
x_cond_clear            (xCond          *cond)
{
    cond_impl_delete (cond->p);
}

/* {{{1 xPrivate */
static pthread_key_t *
private_impl_new (xFreeFunc notify)
{
    pthread_key_t *key;
    xint status;

    key = malloc (sizeof (pthread_key_t));
    if X_UNLIKELY (key == NULL)
        thread_abort (errno, "malloc");
    status = pthread_key_create (key, (void (*)(void *))notify);
    if X_UNLIKELY (status != 0)
        thread_abort (status, "pthread_key_create");

    return key;
}

static void
private_impl_free (pthread_key_t *key)
{
    xint status;

    status = pthread_key_delete (*key);
    if X_UNLIKELY (status != 0)
        thread_abort (status, "pthread_key_delete");
    free (key);
}

static pthread_key_t *
private_get_impl (xPrivate *key)
{
    pthread_key_t *impl = key->p;

    if X_UNLIKELY (impl == NULL)
    {
        impl = private_impl_new (key->notify);
        if (!x_atomic_ptr_cas (&key->p, NULL, impl))
        {
            private_impl_free (impl);
            impl = key->p;
        }
    }

    return impl;
}
xptr
x_private_get           (xPrivate       *key)
{
    return pthread_getspecific (*private_get_impl (key));
}
void
x_private_set           (xPrivate       *key,
                         xptr           value)
{
    xint status;

    if X_UNLIKELY ((status = pthread_setspecific (*private_get_impl (key), value)) != 0)
        thread_abort (status, "pthread_setspecific");
}
void
x_private_replace       (xPrivate       *key,
                         xptr           value)
{
    pthread_key_t *impl = private_get_impl (key);
    xptr old;
    xint status;

    old = pthread_getspecific (*impl);
    if (old && key->notify)
        key->notify (old);

    if X_UNLIKELY ((status = pthread_setspecific (*impl, value)) != 0)
        thread_abort (status, "pthread_setspecific");
}
/* {{{1 xThread */
typedef struct {
    RealThread  thread;

    pthread_t   system_thread;
    xbool       joined;
    xMutex      lock;
} ThreadPosix;
#define posix_check_err(err, name) X_STMT_START{                \
    int error = (err); 							                \
    if (error)	 		 		 			                    \
    x_error ("file %s: line %d (%s): error '%s' during '%s'",   \
             __FILE__, __LINE__, X_STRFUNC,				        \
             strerror (error), name);					        \
} X_STMT_END

#define posix_check_cmd(cmd) posix_check_err (cmd, #cmd)

static void
system_thread_set_name  (xcstr          name)
{
#ifdef HAVE_SYS_PRCTL_H
#ifdef PR_SET_NAME
    prctl (PR_SET_NAME, name, 0, 0, 0, 0);
#endif
#endif
}
static void
system_thread_free      (xThread        *thread)
{
    ThreadPosix *pt = (ThreadPosix *) thread;

    if (!pt->joined)
        pthread_detach (pt->system_thread);

    x_mutex_clear (&pt->lock);

    x_slice_free (ThreadPosix, pt);
}
static void
system_thread_wait      (xThread        *thread)
{
    ThreadPosix *pt = (ThreadPosix *) thread;

    x_mutex_lock (&pt->lock);

    if (!pt->joined) {
        posix_check_cmd (pthread_join (pt->system_thread, NULL));
        pt->joined = TRUE;
    }

    x_mutex_unlock (&pt->lock);
}
static xThread*
system_thread_new       (xThreadFunc    thread_func,
                         xulong         stack_size,
                         xError         **error)
{
    ThreadPosix *thread;
    pthread_attr_t attr;
    xint ret;

    thread = x_slice_new0 (ThreadPosix);

    posix_check_cmd (pthread_attr_init (&attr));

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
    if (stack_size)
    {
#ifdef _SC_THREAD_STACK_MIN
        stack_size = MAX (sysconf (_SC_THREAD_STACK_MIN), stack_size);
#endif /* _SC_THREAD_STACK_MIN */
        /* No error check here, because some systems can't do it and
         * we simply don't want threads to fail because of that. */
        pthread_attr_setstacksize (&attr, stack_size);
    }
#endif /* HAVE_PTHREAD_ATTR_SETSTACKSIZE */

    ret = pthread_create (&thread->system_thread, &attr,
                          (void* (*)(void*))thread_func, thread);

    posix_check_cmd (pthread_attr_destroy (&attr));

    if (ret == EAGAIN)
    {
        x_set_error (error, x_thread_domain(), -1, 
                     "Error creating thread: %s", strerror (ret));
        x_slice_free (ThreadPosix, thread);
        return NULL;
    }

    posix_check_err (ret, "pthread_create");

    x_mutex_init (&thread->lock);

    return (xThread*) thread;
}
static void
system_thread_exit      (void)
{
    pthread_exit (NULL);
}
/* vim:set foldmethod=marker: */
