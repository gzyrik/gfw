#include "rdp_config.hpp"
#include "rdp_thread.hpp"
#include <cstdlib>
#if defined HAVE_WINDOWS_THREAD
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <signal.h>
#define HAVE_PTHREAD
#endif
namespace rdp {
namespace {
#if defined HAVE_PTHREAD
inline static void mutex_impl_delete (void *& p)
{
    if (p != 0) {
        pthread_mutex_destroy ((pthread_mutex_t*)p);
        free (p);
        p = 0;
    }
}
inline static pthread_mutex_t* mutex_get_impl (void* & p)
{
    if (p == 0) {
        void *mutex = malloc (sizeof (pthread_mutex_t));
        if (!mutex)
            LOG_E("malloc %d", errno);
        pthread_mutexattr_t attr;
        pthread_mutexattr_init (&attr);
#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
        pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
#endif
        int status = pthread_mutex_init ((pthread_mutex_t*)mutex, &attr);
        if (status != 0)
            LOG_E("%s", strerror (status));
        pthread_mutexattr_destroy (&attr);
        if (!atomicCompareAndSwap (&p, 0, mutex))
            mutex_impl_delete (mutex);
    }
    return (pthread_mutex_t*)p;
}
inline static void rwlock_impl_delete (void *& p)
{
    if (p != 0) {
        pthread_rwlock_destroy ((pthread_rwlock_t*)p);
        free (p);
        p = 0;
    }
}

inline static pthread_rwlock_t* rwlock_get_impl (void* & p)
{
    if (p == 0) {
        void *rwlock = malloc (sizeof (pthread_rwlock_t));
        if (!rwlock)
            LOG_E("malloc %d", errno);

        int status = pthread_rwlock_init ((pthread_rwlock_t*)rwlock, 0);
        if (status != 0)
            LOG_E("pthread_rwlock_init %s", strerror (status));

        if (!x_atomic_ptr_cas (&p, 0, rwlock))
            rwlock_impl_delete (rwlock);
    }

    return (pthread_rwlock_t*)p;
}
static void *thread_routine (void *arg)
{
#if !defined HAVE_OPENVMS && !defined HAVE_ANDROID
    //  Following code will guarantee more predictable latencies as it'll
    //  disallow any signal handling in the I/O thread.
    sigset_t signal_set;
    int rc = sigfillset (&signal_set);
    ASSERT (rc == 0);
    rc = pthread_sigmask (SIG_BLOCK, &signal_set, NULL);
    ASSERT (rc == 0);
#endif
    SharedPtr<Runnable> *runnable = (SharedPtr<Runnable>*)arg;
    Thread::_routine(runnable->get());
    delete runnable;
    return NULL;
}
#elif defined HAVE_WINDOWS_THREAD
inline static void mutex_impl_delete (void* p)
{
    if (p != 0) {
        DeleteCriticalSection ((CRITICAL_SECTION*)p);
        free (p);
        p = 0;
    }
}
inline static CRITICAL_SECTION * mutex_get_impl (void* &p)
{
    if (p == 0) {
        void *cs = malloc (sizeof(CRITICAL_SECTION));
        if (!cs)
            LOG_E("malloc %d", errno);
        InitializeCriticalSection ((CRITICAL_SECTION*)cs);
        if (InterlockedCompareExchangePointer (&p, cs, 0) != 0)
            mutex_impl_delete (cs);
    }
    return (CRITICAL_SECTION*)p;
}
inline static void rwlock_impl_delete (void *& p)
{
    if (p != 0) {
        free (p);
        p = 0;
    }
}

inline static PSRWLOCK rwlock_get_impl (void* & p)
{
    if (p == 0) {
        void* rwlock = malloc (sizeof (SRWLOCK));
        if (!rwlock)
            LOG_E("malloc %d", errno);

        InitializeSRWLock ((PSRWLOCK)rwlock);
        if (!InterlockedCompareExchangePointer (&p, rwlock, 0) != 0)
            rwlock_impl_delete (rwlock);
    }

    return (PSRWLOCK)p;
}

#if defined _WIN32_WCE
static DWORD thread_routine (LPVOID arg)
#else
static unsigned int __stdcall thread_routine (void *arg)
#endif
{
    SharedPtr<Runnable> *runnable = (SharedPtr<Runnable>*)arg;
    Thread::_routine(runnable->get());
    delete runnable;
    return 0;
}
#endif
}//namespace
#if __cplusplus < 201103L
void Mutex::lock() const
{
#if defined HAVE_PTHREAD
    int status = pthread_mutex_lock (mutex_get_impl (impl_));
    if (status!= 0)
        LOG_E("%s", strerror (status));
#elif defined HAVE_WINDOWS_THREAD
    LeaveCriticalSection(mutex_get_impl(impl_));
#endif
}
void Mutex::unlock() const
{
#if defined HAVE_PTHREAD
    int status = pthread_mutex_unlock (mutex_get_impl (impl_));
    if (status!= 0)
        LOG_E("%s", strerror (status));
#elif defined HAVE_WINDOWS_THREAD
    EnterCriticalSection(mutex_get_impl(impl_));
#endif
}
Mutex::~Mutex()
{
    mutex_impl_delete(impl_);
}
#endif
#if __cplusplus < 201402L
void RWMutex::lock() const
{
#if defined HAVE_PTHREAD
    int status = pthread_rwlock_wrlock (rwlock_get_impl (impl_));
    if (status!= 0)
        LOG_E("%s", strerror (status));
#elif defined HAVE_WINDOWS_THREAD
    AcquireSRWLockExclusive(rwlock_get_impl(impl_));
#endif
}
void RWMutex::unlock() const
{
#if defined HAVE_PTHREAD
    int status = pthread_rwlock_unlock (rwlock_get_impl (impl_));
    if (status!= 0)
        LOG_E("%s", strerror (status));
#elif defined HAVE_WINDOWS_THREAD
    ReleaseSRWLockExclusive(rwlock_get_impl(impl_));
#endif
}
void RWMutex::lock_shared() const
{
#if defined HAVE_PTHREAD
    int status = pthread_rwlock_rdlock (rwlock_get_impl (impl_));
    if (status!= 0)
        LOG_E("%s", strerror (status));
#elif defined HAVE_WINDOWS_THREAD
    AcquireSRWLockShared(rwlock_get_impl(impl_));
#endif

}
void RWMutex::unlock_shared() const
{
#if defined HAVE_PTHREAD
    int status = pthread_rwlock_unlock (rwlock_get_impl (impl_));
    if (status!= 0)
        LOG_E("%s", strerror (status));
#elif defined HAVE_WINDOWS_THREAD
    ReleaseSRWLockShared(rwlock_get_impl(impl_));
#endif
}
RWMutex::~RWMutex()
{
    rwlock_impl_delete(impl_);
}
#endif
Runnable::~Runnable()
{
	while (state_ != kTerminated)
		Time::sleep(Time::MS(1));
}
void Thread::start(const SharedPtr<Runnable>& runnable)
{
    join();
    if (!runnable)
        runnable_ = new SharedPtr<Runnable>(SharedPtr<Runnable>::ever(this));
    else {
        runnable_ = new SharedPtr<Runnable>(runnable);
        runnable->state_ = kWaiting;
        state_ = kRunning;
    }
#if defined HAVE_PTHREAD
    handle_ = malloc(sizeof(pthread_t));
    int rc = pthread_create ((pthread_t*)handle_, NULL, thread_routine, arg);
    ASSERT (rc == 0);
#elif defined HAVE_WINDOWS_THREAD
    HANDLE *handle = (HANDLE*)malloc(sizeof(HANDLE));
    handle_ = handle;
#if defined _WIN32_WCE
    *handle = (HANDLE) CreateThread (NULL, 0, &thread_routine, runnable_, 0 , NULL);
#else
    *handle = (HANDLE) _beginthreadex (NULL, 0, &thread_routine, runnable_, 0 , NULL);
#endif
    ASSERT (*handle != NULL); 
#endif
}
void Thread::join(void)
{
    if (!handle_)
        return;
    Runnable* runnable = runnable_->get();
    if (runnable != this) state_ = kJoining;
    runnable->state_ = kJoining;
#if defined HAVE_PTHREAD
    int rc = pthread_join (*(pthread_t*)handle_, NULL);
    ASSERT (rc == 0);
#elif defined HAVE_WINDOWS_THREAD
    HANDLE descriptor = *(HANDLE*)handle_;
    DWORD rc = WaitForSingleObject (descriptor, INFINITE);
    ASSERT (rc != WAIT_FAILED);
    BOOL rc2 = CloseHandle (descriptor);
    ASSERT (rc2);
#endif
    free(handle_);
    handle_ = 0;
    runnable_ = nullptr;
    if (runnable != this) state_ = kTerminated;
}
void Thread::detach(void)
{
    if (handle_)
        return;
    ASSERT(runnable_->get() != this);

#if defined HAVE_PTHREAD
    int rc = pthread_detach(*(pthread_t*)handle_);
    ASSERT(rc == 0);
#endif
    free(handle_);
    handle_ = 0;
    runnable_ = nullptr;
}
void Thread::_routine(Runnable* runnable)
{
    if (runnable) {
        runnable->state_ = kRunning;
        runnable->run();
        runnable->state_ = kTerminated;
    }
}
void Thread::run(const SharedPtr<Runnable>& runnable)
{
    Thread thread;
    thread.start(runnable);
    thread.detach();
}

}
