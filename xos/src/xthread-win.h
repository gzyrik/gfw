#include "xwin.h"
#include <windows.h>
#include <process.h>

/* Starting with Vista and Windows 2008, we have access to the
 * CONDITION_VARIABLE and SRWLock primatives on Windows, which are
 * pretty reasonable approximations of the primatives specified in
 * POSIX 2001 (pthread_cond_t and pthread_mutex_t respectively).
 *
 * Both of these types are structs containing a single pointer.  That
 * pointer is used as an atomic bitfield to support user-space mutexes
 * that only get the kernel involved in cases of contention (similar
 * to how futex()-based mutexes work on Linux).  The biggest advantage
 * of these new types is that they can be statically initialised to
 * zero.  That means that they are completely ABI compatible with our
 * xMutex and xCond APIs.
 *
 * Unfortunately, Windows XP lacks these facilities and xos still
 * needs to support Windows XP.  Our approach here is as follows:
 *
 *   - avoid depending on structure declarations at compile-time by
 *     declaring our own xMutex and xCond strutures to be
 *     ABI-compatible with SRWLock and CONDITION_VARIABLE and using
 *     those instead
 *
 *   - avoid a hard dependency on the symbols used to manipulate these
 *     structures by doing a dynamic lookup of those symbols at
 *     runtime
 *
 *   - if the symbols are not available, emulate them using other
 *     primatives
 *
 * Using this approach also allows us to easily build a xos that lacks
 * support for Windows XP or to remove this code entirely when XP is no
 * longer supported (end of line is currently April 8, 2014).
 */
typedef struct
{
    void     (__stdcall * CallThisOnThreadExit)        (void);              /* fake */

    void     (__stdcall * InitializeSRWLock)           (xptr lock);
    void     (__stdcall * DeleteSRWLock)               (xptr lock);     /* fake */
    void     (__stdcall * AcquireSRWLockExclusive)     (xptr lock);
    BOOLEAN  (__stdcall * TryAcquireSRWLockExclusive)  (xptr lock);
    void     (__stdcall * ReleaseSRWLockExclusive)     (xptr lock);
    void     (__stdcall * AcquireSRWLockShared)        (xptr lock);
    BOOLEAN  (__stdcall * TryAcquireSRWLockShared)     (xptr lock);
    void     (__stdcall * ReleaseSRWLockShared)        (xptr lock);

    void     (__stdcall * InitializeConditionVariable) (xptr cond);
    void     (__stdcall * DeleteConditionVariable)     (xptr cond);     /* fake */
    BOOL     (__stdcall * SleepConditionVariableSRW)   (xptr cond,
                                                        xptr lock,
                                                        DWORD    timeout,
                                                        ULONG    flags);
    void     (__stdcall * WakeAllConditionVariable)    (xptr cond);
    void     (__stdcall * WakeConditionVariable)       (xptr cond);
} ThreadImplVtable;

static ThreadImplVtable _thread_impl_vtable;

void
x_mutex_init (xMutex *mutex)
{
    _thread_impl_vtable.InitializeSRWLock (mutex);
}

void
x_mutex_clear (xMutex *mutex)
{
    if (_thread_impl_vtable.DeleteSRWLock != NULL)
        _thread_impl_vtable.DeleteSRWLock (mutex);
}

void
x_mutex_lock (xMutex *mutex)
{
    _thread_impl_vtable.AcquireSRWLockExclusive (mutex);
}

xbool
x_mutex_try_lock        (xMutex         *mutex)
{
    return _thread_impl_vtable.TryAcquireSRWLockExclusive (mutex);
}

void
x_mutex_unlock          (xMutex         *mutex)
{
    _thread_impl_vtable.ReleaseSRWLockExclusive (mutex);
}

static CRITICAL_SECTION *
x_recmutex_impl_new     (void)
{
    CRITICAL_SECTION *cs;

    cs = x_slice_new (CRITICAL_SECTION);
    InitializeCriticalSection (cs);

    return cs;
}

static void
x_recmutex_impl_free    (CRITICAL_SECTION   *cs)
{
    DeleteCriticalSection (cs);
    x_slice_free (CRITICAL_SECTION, cs);
}

static CRITICAL_SECTION *
x_recmutex_get_impl     (xRecMutex      *mutex)
{
    CRITICAL_SECTION *impl = mutex->p;

    if X_UNLIKELY (mutex->p == NULL)
    {
        impl = x_recmutex_impl_new ();
        if (InterlockedCompareExchangePointer (&mutex->p, impl, NULL) != NULL)
            x_recmutex_impl_free (impl);
        impl = mutex->p;
    }

    return impl;
}

void
x_recmutex_init         (xRecMutex      *mutex)
{
    mutex->p = x_recmutex_impl_new ();
}

void
x_recmutex_clear        (xRecMutex      *mutex)
{
    x_recmutex_impl_free (mutex->p);
}

void
x_recmutex_lock         (xRecMutex      *mutex)
{
    EnterCriticalSection (x_recmutex_get_impl (mutex));
}

void
x_recmutex_unlock       (xRecMutex      *mutex)
{
    LeaveCriticalSection (mutex->p);
}

xbool
x_recmutex_try_lock     (xRecMutex      *mutex)
{
    return TryEnterCriticalSection (x_recmutex_get_impl (mutex));
}


void
x_rwlock_init           (xRWLock        *lock)
{
    _thread_impl_vtable.InitializeSRWLock (lock);
}

void
x_rwlock_clear          (xRWLock        *lock)
{
    if (_thread_impl_vtable.DeleteSRWLock != NULL)
        _thread_impl_vtable.DeleteSRWLock (lock);
}

void
x_rwlock_wlock          (xRWLock        *lock)
{
    _thread_impl_vtable.AcquireSRWLockExclusive (lock);
}

xbool
x_rwlock_try_wlock      (xRWLock        *lock)
{
    return _thread_impl_vtable.TryAcquireSRWLockExclusive (lock);
}

void
x_rwlock_unwlock        (xRWLock        *lock)
{
    _thread_impl_vtable.ReleaseSRWLockExclusive (lock);
}

void
x_rwlock_rlock          (xRWLock        *lock)
{
    _thread_impl_vtable.AcquireSRWLockShared (lock);
}

xbool
x_rwlock_try_rlock      (xRWLock        *lock)
{
    return _thread_impl_vtable.TryAcquireSRWLockShared (lock);
}

void
x_rwlock_unrlock        (xRWLock        *lock)
{
    _thread_impl_vtable.ReleaseSRWLockShared (lock);
}

/* {{{1 xCond */
void
x_cond_init (xCond *cond)
{
    _thread_impl_vtable.InitializeConditionVariable (cond);
}

void
x_cond_clear (xCond *cond)
{
    if (_thread_impl_vtable.DeleteConditionVariable)
        _thread_impl_vtable.DeleteConditionVariable (cond);
}

void
x_cond_signal (xCond *cond)
{
    _thread_impl_vtable.WakeConditionVariable (cond);
}

void
x_cond_broadcast (xCond *cond)
{
    _thread_impl_vtable.WakeAllConditionVariable (cond);
}

xbool
x_cond_wait_timeout     (xCond          *cond,
                         xMutex         *entered_mutex,
                         xuint32        timeout)
{
    DWORD span;

    if (timeout == X_MAXUINT32)
        span = INFINITE;
    else
        span = timeout;

    return _thread_impl_vtable.SleepConditionVariableSRW
        (cond, entered_mutex, span, 0);
}

typedef struct _xPrivateDestructor xPrivateDestructor;

struct _xPrivateDestructor
{
    DWORD               index;
    xFreeFunc           notify;
    xPrivateDestructor  *next;
};

static xPrivateDestructor * volatile x_private_destructors;
static CRITICAL_SECTION x_private_lock;

static DWORD
x_private_get_impl (xPrivate *key)
{
    DWORD impl = (DWORD) key->p;

    if X_UNLIKELY (impl == 0)
    {
        EnterCriticalSection (&x_private_lock);
        impl = (DWORD) key->p;
        if (impl == 0)
        {
            xPrivateDestructor *destructor;

            impl = TlsAlloc ();

            if (impl == TLS_OUT_OF_INDEXES)
                thread_abort (0, "TlsAlloc");

            if (key->notify != NULL)
            {
                destructor = malloc (sizeof (xPrivateDestructor));
                if X_UNLIKELY (destructor == NULL)
                    thread_abort (errno, "malloc");
                destructor->index = impl;
                destructor->notify = key->notify;
                destructor->next = x_private_destructors;

                /* We need to do an atomic store due to the unlocked
                 * access to the destructor list from the thread exit
                 * function.
                 *
                 * It can double as a sanity check...
                 */
                if (InterlockedCompareExchangePointer (&x_private_destructors, destructor,
                                                       destructor->next) != destructor->next)
                    thread_abort (0, "x_private_get_impl(1)");
            }

            /* Ditto, due to the unlocked access on the fast path */
            if (InterlockedCompareExchangePointer (&key->p, impl, NULL) != NULL)
                thread_abort (0, "x_private_get_impl(2)");
        }
        LeaveCriticalSection (&x_private_lock);
    }

    return impl;
}

xptr
x_private_get (xPrivate *key)
{
    return TlsGetValue (x_private_get_impl (key));
}

void
x_private_set (xPrivate *key,
               xptr  value)
{
    TlsSetValue (x_private_get_impl (key), value);
}

void
x_private_replace (xPrivate *key,
                   xptr  value)
{
    DWORD impl = x_private_get_impl (key);
    xptr old;

    old = TlsGetValue (impl);
    if (old && key->notify)
        key->notify (old);
    TlsSetValue (impl, value);
}

/* {{{1 xThread */

#define win32_check_for_error(what) X_STMT_START{			\
    if (!(what))								            \
    x_error ("file %s: line %d (%s): error %s during %s",	\
             __FILE__, __LINE__, X_STRFUNC,				    \
             x_win_error_msg (GetLastError ()), #what);	    \
}X_STMT_END

#define X_MUTEX_SIZE (sizeof (xptr))

typedef BOOL (__stdcall *GTryEnterCriticalSectionFunc) (CRITICAL_SECTION *);

typedef struct {
    RealThread  thread;
    xThreadFunc proxy;
    HANDLE      handle;
} ThreadWin32;

static void
system_thread_free      (xThread        *thread)
{
    ThreadWin32 *wt = (ThreadWin32 *) thread;

    win32_check_for_error (CloseHandle (wt->handle));
    x_slice_free (ThreadWin32, wt);
}

static void
system_thread_exit      (void)
{
    _endthreadex (0);
}

static xuint __stdcall
x_thread_win32_proxy (xptr data)
{
    ThreadWin32 *self = data;

    self->proxy (self);

    system_thread_exit ();

    x_assert_not_reached ();

    return 0;
}

static xThread *
system_thread_new       (xThreadFunc    func,
                         xsize          stack_size,
                         xError         **error)
{
    ThreadWin32 *thread;
    xuint ignore;

    thread = x_slice_new0 (ThreadWin32);
    thread->proxy = func;

    thread->handle = (HANDLE) _beginthreadex (NULL, (unsigned)stack_size, x_thread_win32_proxy, thread, 0, &ignore);

    if (thread->handle == NULL && error)
    {
        xchar *win_error = x_win_error_msg (GetLastError ());
        *error = x_error_new (x_thread_domain(), -1,
                              "Error creating thread: %s", win_error);
        x_free (win_error);
        x_slice_free (ThreadWin32, thread);
        return NULL;
    }

    return (xThread *) thread;
}

void
x_thread_yield (void)
{
    Sleep(0);
}

static void
system_thread_wait      (xThread        *thread)
{
    ThreadWin32 *wt = (ThreadWin32 *) thread;

    win32_check_for_error (WAIT_FAILED != WaitForSingleObject (wt->handle, INFINITE));
}

static void
system_thread_set_name  (const xchar    *name)
{
    /* FIXME: implement */
}

/* {{{1 SRWLock and CONDITION_VARIABLE emulation (for Windows XP) */

static CRITICAL_SECTION     _thread_xp_lock;
static DWORD                _thread_xp_waiter_tls;

/* {{{2 GThreadWaiter utility class for CONDITION_VARIABLE emulation */
typedef struct _xThreadXpWaiter xThreadXpWaiter;
struct _xThreadXpWaiter
{
    HANDLE                     event;
    volatile xThreadXpWaiter  *next;
    volatile xThreadXpWaiter **my_owner;
};

static xThreadXpWaiter *
x_thread_xp_waiter_get (void)
{
    xThreadXpWaiter *waiter;

    waiter = TlsGetValue (_thread_xp_waiter_tls);

    if X_UNLIKELY (waiter == NULL)
    {
        waiter = malloc (sizeof (xThreadXpWaiter));
        if (waiter == NULL)
            thread_abort (GetLastError (), "malloc");
        waiter->event = CreateEvent (0, FALSE, FALSE, NULL);
        if (waiter->event == NULL)
            thread_abort (GetLastError (), "CreateEvent");
        waiter->my_owner = NULL;

        TlsSetValue (_thread_xp_waiter_tls, waiter);
    }

    return waiter;
}

static void __stdcall
x_thread_xp_CallThisOnThreadExit (void)
{
    xThreadXpWaiter *waiter;

    waiter = TlsGetValue (_thread_xp_waiter_tls);

    if (waiter != NULL)
    {
        TlsSetValue (_thread_xp_waiter_tls, NULL);
        CloseHandle (waiter->event);
        free (waiter);
    }
}

/* {{{2 SRWLock emulation */
typedef struct
{
    CRITICAL_SECTION  writer_lock;
    xbool          ever_shared;    /* protected by writer_lock */
    xbool          writer_locked;  /* protected by writer_lock */

    /* below is only ever touched if ever_shared becomes true */
    CRITICAL_SECTION  atomicity;
    xThreadXpWaiter  *queued_writer; /* protected by atomicity lock */
    xint              num_readers;   /* protected by atomicity lock */
} xThreadSRWLock;

static void __stdcall
x_thread_xp_InitializeSRWLock (xptr mutex)
{
    *(xThreadSRWLock * volatile *) mutex = NULL;
}

static void __stdcall
x_thread_xp_DeleteSRWLock (xptr mutex)
{
    xThreadSRWLock *lock = *(xThreadSRWLock * volatile *) mutex;

    if (lock)
    {
        if (lock->ever_shared)
            DeleteCriticalSection (&lock->atomicity);

        DeleteCriticalSection (&lock->writer_lock);
        free (lock);
    }
}

static xThreadSRWLock * __stdcall
x_thread_xp_get_srwlock (xThreadSRWLock * volatile *lock)
{
    xThreadSRWLock *result;

    /* It looks like we're missing some barriers here, but this code only
     * ever runs on Windows XP, which in turn only ever runs on hardware
     * with a relatively rigid memory model.  The 'volatile' will take
     * care of the compiler.
     */
    result = *lock;

    if X_UNLIKELY (result == NULL)
    {
        EnterCriticalSection (&_thread_xp_lock);

        /* Check again */
        result = *lock;
        if (result == NULL)
        {
            result = malloc (sizeof (xThreadSRWLock));

            if (result == NULL)
                thread_abort (errno, "malloc");

            InitializeCriticalSection (&result->writer_lock);
            result->writer_locked = FALSE;
            result->ever_shared = FALSE;
            *lock = result;
        }

        LeaveCriticalSection (&_thread_xp_lock);
    }

    return result;
}

static void __stdcall
x_thread_xp_AcquireSRWLockExclusive (xptr mutex)
{
    xThreadSRWLock *lock = x_thread_xp_get_srwlock (mutex);

    EnterCriticalSection (&lock->writer_lock);

    /* CRITICAL_SECTION is reentrant, but SRWLock is not.
     * Detect the deadlock that would occur on later Windows version.
     */
    x_assert (!lock->writer_locked);
    lock->writer_locked = TRUE;

    if (lock->ever_shared)
    {
        xThreadXpWaiter *waiter = NULL;

        EnterCriticalSection (&lock->atomicity);
        if (lock->num_readers > 0)
            lock->queued_writer = waiter = x_thread_xp_waiter_get ();
        LeaveCriticalSection (&lock->atomicity);

        if (waiter != NULL)
            WaitForSingleObject (waiter->event, INFINITE);

        lock->queued_writer = NULL;
    }
}

static BOOLEAN __stdcall
x_thread_xp_TryAcquireSRWLockExclusive (xptr mutex)
{
    xThreadSRWLock *lock = x_thread_xp_get_srwlock (mutex);

    if (!TryEnterCriticalSection (&lock->writer_lock))
        return FALSE;

    /* CRITICAL_SECTION is reentrant, but SRWLock is not.
     * Ensure that this properly returns FALSE (as SRWLock would).
     */
    if X_UNLIKELY (lock->writer_locked)
    {
        LeaveCriticalSection (&lock->writer_lock);
        return FALSE;
    }

    lock->writer_locked = TRUE;

    if (lock->ever_shared)
    {
        xbool available;

        EnterCriticalSection (&lock->atomicity);
        available = lock->num_readers == 0;
        LeaveCriticalSection (&lock->atomicity);

        if (!available)
        {
            LeaveCriticalSection (&lock->writer_lock);
            return FALSE;
        }
    }

    return TRUE;
}

static void __stdcall
x_thread_xp_ReleaseSRWLockExclusive (xptr mutex)
{
    xThreadSRWLock *lock = *(xThreadSRWLock * volatile *) mutex;

    lock->writer_locked = FALSE;

    /* We need this until we fix some weird parts of xos that try to
     * unlock freshly-allocated mutexes.
     */
    if (lock != NULL)
        LeaveCriticalSection (&lock->writer_lock);
}

static void
x_thread_xp_srwlock_become_reader (xThreadSRWLock *lock)
{
    if X_UNLIKELY (!lock->ever_shared)
    {
        InitializeCriticalSection (&lock->atomicity);
        lock->queued_writer = NULL;
        lock->num_readers = 0;

        lock->ever_shared = TRUE;
    }

    EnterCriticalSection (&lock->atomicity);
    lock->num_readers++;
    LeaveCriticalSection (&lock->atomicity);
}

static void __stdcall
x_thread_xp_AcquireSRWLockShared (xptr mutex)
{
    xThreadSRWLock *lock = x_thread_xp_get_srwlock (mutex);

    EnterCriticalSection (&lock->writer_lock);

    /* See x_thread_xp_AcquireSRWLockExclusive */
    x_assert (!lock->writer_locked);

    x_thread_xp_srwlock_become_reader (lock);

    LeaveCriticalSection (&lock->writer_lock);
}

static BOOLEAN __stdcall
x_thread_xp_TryAcquireSRWLockShared (xptr mutex)
{
    xThreadSRWLock *lock = x_thread_xp_get_srwlock (mutex);

    if (!TryEnterCriticalSection (&lock->writer_lock))
        return FALSE;

    /* See x_thread_xp_AcquireSRWLockExclusive */
    if X_UNLIKELY (lock->writer_locked)
    {
        LeaveCriticalSection (&lock->writer_lock);
        return FALSE;
    }

    x_thread_xp_srwlock_become_reader (lock);

    LeaveCriticalSection (&lock->writer_lock);

    return TRUE;
}

static void __stdcall
x_thread_xp_ReleaseSRWLockShared (xptr mutex)
{
    xThreadSRWLock *lock = x_thread_xp_get_srwlock (mutex);

    EnterCriticalSection (&lock->atomicity);

    lock->num_readers--;

    if (lock->num_readers == 0 && lock->queued_writer)
        SetEvent (lock->queued_writer->event);

    LeaveCriticalSection (&lock->atomicity);
}

/* {{{2 CONDITION_VARIABLE emulation */
typedef struct
{
    volatile xThreadXpWaiter  *first;
    volatile xThreadXpWaiter **last_ptr;
} xThreadXpCONDITION_VARIABLE;

static void __stdcall
x_thread_xp_InitializeConditionVariable (xptr cond)
{
    *(xThreadXpCONDITION_VARIABLE * volatile *) cond = NULL;
}

static void __stdcall
x_thread_xp_DeleteConditionVariable (xptr cond)
{
    xThreadXpCONDITION_VARIABLE *cv = *(xThreadXpCONDITION_VARIABLE * volatile *) cond;

    if (cv)
        free (cv);
}

static xThreadXpCONDITION_VARIABLE * __stdcall
x_thread_xp_get_condition_variable (xThreadXpCONDITION_VARIABLE * volatile *cond)
{
    xThreadXpCONDITION_VARIABLE *result;

    /* It looks like we're missing some barriers here, but this code only
     * ever runs on Windows XP, which in turn only ever runs on hardware
     * with a relatively rigid memory model.  The 'volatile' will take
     * care of the compiler.
     */
    result = *cond;

    if X_UNLIKELY (result == NULL)
    {
        result = malloc (sizeof (xThreadXpCONDITION_VARIABLE));

        if (result == NULL)
            thread_abort (errno, "malloc");

        result->first = NULL;
        result->last_ptr = &result->first;

        if (InterlockedCompareExchangePointer (cond, result, NULL) != NULL)
        {
            free (result);
            result = *cond;
        }
    }

    return result;
}

static BOOL __stdcall
x_thread_xp_SleepConditionVariableSRW (xptr cond,
                                       xptr mutex,
                                       DWORD    timeout,
                                       ULONG    flags)
{
    xThreadXpCONDITION_VARIABLE *cv = x_thread_xp_get_condition_variable (cond);
    xThreadXpWaiter *waiter = x_thread_xp_waiter_get ();
    DWORD status;

    waiter->next = NULL;

    EnterCriticalSection (&_thread_xp_lock);
    waiter->my_owner = cv->last_ptr;
    *cv->last_ptr = waiter;
    cv->last_ptr = &waiter->next;
    LeaveCriticalSection (&_thread_xp_lock);

    x_mutex_unlock (mutex);
    status = WaitForSingleObject (waiter->event, timeout);

    if (status != WAIT_TIMEOUT && status != WAIT_OBJECT_0)
        thread_abort (GetLastError (), "WaitForSingleObject");
    x_mutex_lock (mutex);

    if (status == WAIT_TIMEOUT)
    {
        EnterCriticalSection (&_thread_xp_lock);
        if (waiter->my_owner)
        {
            if (waiter->next)
                waiter->next->my_owner = waiter->my_owner;
            else
                cv->last_ptr = waiter->my_owner;
            *waiter->my_owner = waiter->next;
            waiter->my_owner = NULL;
        }
        LeaveCriticalSection (&_thread_xp_lock);
    }

    return status == WAIT_OBJECT_0;
}

static void __stdcall
x_thread_xp_WakeConditionVariable (xptr cond)
{
    xThreadXpCONDITION_VARIABLE *cv = x_thread_xp_get_condition_variable (cond);
    volatile xThreadXpWaiter *waiter;

    EnterCriticalSection (&_thread_xp_lock);

    waiter = cv->first;
    if (waiter != NULL)
    {
        waiter->my_owner = NULL;
        cv->first = waiter->next;
        if (cv->first != NULL)
            cv->first->my_owner = &cv->first;
        else
            cv->last_ptr = &cv->first;
    }

    if (waiter != NULL)
        SetEvent (waiter->event);

    LeaveCriticalSection (&_thread_xp_lock);
}

static void __stdcall
x_thread_xp_WakeAllConditionVariable (xptr cond)
{
    xThreadXpCONDITION_VARIABLE *cv = x_thread_xp_get_condition_variable (cond);
    volatile xThreadXpWaiter *waiter;

    EnterCriticalSection (&_thread_xp_lock);

    waiter = cv->first;
    cv->first = NULL;
    cv->last_ptr = &cv->first;

    while (waiter != NULL)
    {
        volatile xThreadXpWaiter *next;

        next = waiter->next;
        SetEvent (waiter->event);
        waiter->my_owner = NULL;
        waiter = next;
    }

    LeaveCriticalSection (&_thread_xp_lock);
}

static void
x_thread_xp_init (void)
{
    static const ThreadImplVtable thread_xp_impl_vtable = {
        x_thread_xp_CallThisOnThreadExit,
        x_thread_xp_InitializeSRWLock,
        x_thread_xp_DeleteSRWLock,
        x_thread_xp_AcquireSRWLockExclusive,
        x_thread_xp_TryAcquireSRWLockExclusive,
        x_thread_xp_ReleaseSRWLockExclusive,
        x_thread_xp_AcquireSRWLockShared,
        x_thread_xp_TryAcquireSRWLockShared,
        x_thread_xp_ReleaseSRWLockShared,
        x_thread_xp_InitializeConditionVariable,
        x_thread_xp_DeleteConditionVariable,
        x_thread_xp_SleepConditionVariableSRW,
        x_thread_xp_WakeAllConditionVariable,
        x_thread_xp_WakeConditionVariable
    };

    InitializeCriticalSection (&_thread_xp_lock);
    _thread_xp_waiter_tls = TlsAlloc ();
    _thread_impl_vtable = thread_xp_impl_vtable;
}

/* {{{1 Epilogue */

static xbool
x_thread_lookup_native_funcs (void)
{
    ThreadImplVtable native_vtable = { 0, };
    HMODULE kernel32;

    kernel32 = GetModuleHandleA ("KERNEL32.DLL");

    if (kernel32 == NULL)
        return FALSE;

#define GET_FUNC(name) if ((native_vtable.name = (void *) GetProcAddress (kernel32, #name)) == NULL) return FALSE
    GET_FUNC(InitializeSRWLock);
    GET_FUNC(AcquireSRWLockExclusive);
    GET_FUNC(TryAcquireSRWLockExclusive);
    GET_FUNC(ReleaseSRWLockExclusive);
    GET_FUNC(AcquireSRWLockShared);
    GET_FUNC(TryAcquireSRWLockShared);
    GET_FUNC(ReleaseSRWLockShared);

    GET_FUNC(InitializeConditionVariable);
    GET_FUNC(SleepConditionVariableSRW);
    GET_FUNC(WakeAllConditionVariable);
    GET_FUNC(WakeConditionVariable);
#undef GET_FUNC

    _thread_impl_vtable = native_vtable;

    return TRUE;
}

void
_x_thread_win32_init    (void)
{
    if (!x_thread_lookup_native_funcs ())
        x_thread_xp_init ();

    InitializeCriticalSection (&x_private_lock);
}

void
_x_thread_win32_detach  (void)
{
    xbool dtors_called;

    do {
        xPrivateDestructor *dtor;

        /* We go by the POSIX book on this one.
         *
         * If we call a destructor then there is a chance that some new
         * TLS variables got set by code called in that destructor.
         *
         * Loop until nothing is left.
         */
        dtors_called = FALSE;

        for (dtor = x_private_destructors; dtor; dtor = dtor->next)
        {
            xptr value;

            value = TlsGetValue (dtor->index);
            if (value != NULL && dtor->notify != NULL)
            {
                /* POSIX says to clear this before the call */
                TlsSetValue (dtor->index, NULL);
                dtor->notify (value);
                dtors_called = TRUE;
            }
        }
    }
    while (dtors_called);

    if (_thread_impl_vtable.CallThisOnThreadExit)
        _thread_impl_vtable.CallThisOnThreadExit ();
}
