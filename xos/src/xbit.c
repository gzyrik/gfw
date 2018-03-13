#define X_IMPLEMENT_INLINES 1
#define __X_BIT_C__
#include "config.h"
#include "xbit.h"
void
x_bit_lock              (volatile xint  *address,
                         xint           lock_bit)
{
}

xbool
x_bit_try_lock          (volatile xint  *address,
                         xint           lock_bit)
{
    return FALSE;
}

void
x_bit_unlock            (volatile xint  *address,
                         xint           lock_bit)
{
}

void
x_ptr_bit_lock          (volatile void  *address,
                         xint           lock_bit)
{
}

xbool
x_ptr_bit_try_lock      (volatile void  *address,
                         xint           lock_bit)
{
    return FALSE;
}

void
x_ptr_bit_unlock        (volatile void  *address,
                         xint           lock_bit)
{
}

