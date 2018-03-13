#include "config.h"
#include "xatomic.h"
#ifdef X_OS_WINDOWS

#include <windows.h>
#if !defined(_M_AMD64) && !defined (_M_IA64) && !defined(_M_X64)
#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor
#endif
/*
 * http://msdn.microsoft.com/en-us/library/ms684122(v=vs.85).aspx
 */
xint
x_atomic_int_get (volatile xint *atomic)
{
    MemoryBarrier ();
    return *atomic;
}

void
x_atomic_int_set (volatile xint *atomic, xint newval)
{
    *atomic = newval;
    MemoryBarrier ();
}

xint
x_atomic_int_inc (volatile xint *atomic)
{
    return InterlockedIncrement (atomic);
}

xint
x_atomic_int_dec (volatile xint *atomic)
{
    return InterlockedDecrement (atomic);
}

xbool
x_atomic_int_cas (volatile xint *atomic, xint oldval, xint newval)
{
    return InterlockedCompareExchange (atomic, newval, oldval) == oldval;
}

xint
x_atomic_int_add (volatile xint *atomic, xint val)
{
    return InterlockedExchangeAdd (atomic, val);
}

xuint
x_atomic_int_and (volatile xuint *atomic, xuint val)
{
    return InterlockedAnd (atomic, val);
}

xuint
x_atomic_int_or (volatile xuint *atomic, xuint val)
{
    return InterlockedOr (atomic, val);
}

xuint
x_atomic_int_xor (volatile xuint *atomic, xuint val)
{
    return InterlockedXor (atomic, val);
}

xptr
x_atomic_ptr_get (volatile void *atomic)
{
    volatile xptr *ptr = (volatile xptr *)atomic;
    MemoryBarrier ();
    return *ptr;
}

void
x_atomic_ptr_set (volatile void *atomic, xptr newval)
{
    volatile xptr *ptr = (volatile xptr *)atomic;
    *ptr = newval;
    MemoryBarrier ();
}

xbool
x_atomic_ptr_cas (volatile void *atomic, xptr oldval, xptr newval)
{
    volatile xptr *ptr = (volatile xptr *)atomic;

    return InterlockedCompareExchangePointer (ptr, newval, oldval) == oldval;
}

xssize
x_atomic_ptr_add (volatile void *atomic, xssize val)
{
#ifdef X_OS_WIN64
    return InterlockedExchangeAdd64 (atomic, val);
#else
    return InterlockedExchangeAdd (atomic, val);
#endif
}

xsize
x_atomic_ptr_and (volatile void *atomic, xsize val)
{
#ifdef X_OS_WIN64
    return InterlockedAnd64 (atomic, val);
#else
    return InterlockedAnd (atomic, val);
#endif
}

xsize
x_atomic_ptr_or (volatile void *atomic, xsize val)
{
#ifdef X_OS_WIN64
    return InterlockedOr64 (atomic, val);
#else
    return InterlockedOr (atomic, val);
#endif
}

xsize
x_atomic_ptr_xor (volatile void *atomic, xsize val)
{
#ifdef X_OS_WIN64
    return InterlockedXor64 (atomic, val);
#else
    return InterlockedXor (atomic, val);
#endif
}
#endif /* X_WIN32 */
