#ifndef __X_BIT_H__
#define __X_BIT_H__
#include "xtype.h"
X_BEGIN_DECLS

void        x_bit_lock              (volatile xint  *address,
                                     xint           lock_bit);

xbool       x_bit_try_lock          (volatile xint  *address,
                                     xint           lock_bit);

void        x_bit_unlock            (volatile xint  *address,
                                     xint           lock_bit);

void        x_ptr_bit_lock          (volatile void  *address,
                                     xint           lock_bit);

xbool       x_ptr_bit_try_lock      (volatile void  *address,
                                     xint           lock_bit);

void        x_ptr_bit_unlock        (volatile void  *address,
                                     xint           lock_bit);


X_INLINE_FUNC
xint        x_bit_nth_lsf           (xulong         mask,
                                     xint           nth);
X_INLINE_FUNC
xint        x_bit_nth_msf           (xulong         mask,
                                     xint           nth);
X_INLINE_FUNC
xsize       x_bit_storage           (xulong         number);

#if defined (X_CAN_INLINE) || defined (__X_BIT_C__)

X_INLINE_FUNC xint
x_bit_nth_lsf           (xulong         mask,
                         xint           nth)
{
    if (X_UNLIKELY (nth < -1))
        nth = -1;
    while (nth < ((sizeof(xulong) * 8) - 1)) {
        nth++;
        if (mask & (1UL << nth))
            return nth;
    }
    return -1;
}

X_INLINE_FUNC xint
x_bit_nth_msf           (xulong         mask,
                         xint           nth)
{
    if (nth < 0 || X_UNLIKELY (nth > sizeof(xulong) * 8))
        nth = sizeof(xulong) * 8;
    while (nth > 0) {
        nth--;
        if (mask & (1UL << nth))
            return nth;
    }
    return -1;
}

X_INLINE_FUNC xsize
x_bit_storage           (xulong         number)
{
    register xsize n_bits = 0;
    do {
        n_bits++;
        number >>= 1;
    } while (number);
    return n_bits;
}
#endif /* X_CAN_INLINE || __X_BIT_C__ */

X_END_DECLS
#endif /* __X_BITS_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
