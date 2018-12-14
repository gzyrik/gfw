#include "rdp_config.hpp"
#include "rdp_shared.hpp"
#ifdef _WIN32
#include <windows.h>
#elif defined DARWIN
#include <libkern/OSAtomic.h>
#endif
namespace rdp{
namespace {
inline static int atomicIncrement(int *val)
{
#ifdef _WIN32
    return sizeof(int) == sizeof(LONG) 
        ?  InterlockedIncrement((LONG volatile*)val)
        :  (int)InterlockedIncrement64((LONGLONG volatile*)val);
#elif defined DARWIN
    return sizeof(int) == sizeof(uint32_t) 
        ? OSAtomicIncrement32((volatile int32_t*)val)
        : OSAtomicIncrement64((volatile int64_t*)val);
#endif
}
inline static int atomicDecrement(int *val)
{
    return
#ifdef _WIN32
        sizeof(int) == sizeof(LONG) 
        ? InterlockedDecrement((LONG volatile*)val)
        : (int)InterlockedDecrement64((LONGLONG volatile*)val);
#elif defined DARWIN
    sizeof(int) == sizeof(uint32_t) 
        ? OSAtomicDecrement32((volatile int32_t*)val)
        : (int)OSAtomicDecrement64((volatile int64_t*)val);
#endif
}
inline static bool atomicCompareAndSwap(int *atomic, int oldval, int newval)
{
#ifdef _WIN32
    return sizeof(int) == sizeof(LONG)
        ? InterlockedCompareExchange ((LONG volatile*)atomic, newval, oldval) == oldval
        : InterlockedCompareExchange64((LONGLONG volatile*)atomic, newval, oldval) == oldval;
#elif defined DARWIN
    return sizeof(int) == sizeof(uint32_t)
        ? OSAtomicCompareAndSwap32(oldval, newval, (volatile int32_t*)atomic)
        : OSAtomicCompareAndSwap64(oldval, newval, (volatile int64_t*)atomic);
#endif
}
}
void SharedCountBase::addRef()
{
    atomicIncrement(&refcount_);
}
void SharedCountBase::release()
{
    if(atomicDecrement(&refcount_)==0) {
        dispose();
        weakRelease();
    }
}
void SharedCountBase::weakAddRef()
{
    atomicIncrement(&weakcount_);
}
bool SharedCountBase::addLockRef()
{
    for(;;)
    {
        int tmp = static_cast<int const volatile&>(refcount_) ;
        if(tmp==0) return false;
        if(atomicCompareAndSwap(&refcount_, tmp, tmp+1))
            return true;
    }
}
void SharedCountBase::weakRelease()
{
    if(atomicDecrement(&weakcount_)==0)
        destroy();
}
}
