#ifndef __X_ATOMIC_H__
#define __X_ATOMIC_H__
#include "xtype.h"
X_BEGIN_DECLS
#if defined(X_HAVE_GCC_SYNC)
#define x_atomic_int_get(atomic)                                        \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (void) (0 ? *(atomic) ^ *(atomic) : 0);               \
                  __sync_synchronize ();                                \
                  (xint) *(atomic);                                     \
                  }))
#define x_atomic_int_set(atomic, newval)                                \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (void) (0 ? *(atomic) ^ (newval) : 0);                \
                  *(atomic) = (newval);                                 \
                  __sync_synchronize ();                                \
                  }))
#define x_atomic_int_inc(atomic)                                        \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (xint) (0 ? *(atomic) ^ *(atomic) : 0);               \
                  (xint) __sync_add_and_fetch ((atomic), 1);            \
                  }))
#define x_atomic_int_dec(atomic)                                        \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (xint) (0 ? *(atomic) ^ *(atomic) : 0);               \
                  (xint)__sync_sub_and_fetch ((atomic), 1);             \
                  }))
#define x_atomic_int_cas(atomic, oldval, newval)                        \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (void) (0 ? *(atomic) ^ (newval) ^ (oldval) : 0);     \
                  (xbool) __sync_bool_compare_and_swap ((atomic),       \
                                                        (oldval),       \
                                                        (newval));      \
                  }))
#define x_atomic_int_add(atomic, val)                                   \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (void) (0 ? *(atomic) ^ (val) : 0);                   \
                  (xint) __sync_fetch_and_add ((atomic), (val));        \
                  }))
#define x_atomic_int_and(atomic, val)                                   \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (void) (0 ? *(atomic) ^ (val) : 0);                   \
                  (guint) __sync_fetch_and_and ((atomic), (val));       \
                  }))
#define x_atomic_int_or(atomic, val)                                    \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (void) (0 ? *(atomic) ^ (val) : 0);                   \
                  (guint) __sync_fetch_and_or ((atomic), (val));        \
                  }))
#define x_atomic_int_xor(atomic, val)                                   \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xint));  \
                  (void) (0 ? *(atomic) ^ (val) : 0);                   \
                  (guint) __sync_fetch_and_xor ((atomic), (val));       \
                  }))

#define x_atomic_ptr_get(atomic)                                        \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xptr));  \
                  __sync_synchronize ();                                \
                  (xptr) *(atomic);                                     \
                  }))
#define x_atomic_ptr_set(atomic, newval)                                \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xptr));  \
                  (void) (0 ? (xptr) *(atomic) : 0);                    \
                  *(atomic) = (__typeof__ (*(atomic))) (xsize) (newval);\
                  __sync_synchronize ();                                \
                  }))
#define x_atomic_ptr_cas(atomic, oldval, newval)                        \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xptr));  \
                  (void) (0 ? (xptr) *(atomic) : 0);                    \
                  (xbool) __sync_bool_compare_and_swap ((atomic),       \
                                                        (oldval),       \
                                                        (newval));      \
                  }))
#define x_atomic_ptr_add(atomic, val)                                   \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xptr));  \
                  (void) (0 ? (xptr) *(atomic) : 0);                    \
                  (void) (0 ? (val) ^ (val) : 0);                       \
                  (gssize) __sync_fetch_and_add ((atomic), (val));      \
                  }))
#define x_atomic_ptr_and(atomic, val)                                   \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xptr));  \
                  (void) (0 ? (xptr) *(atomic) : 0);                    \
                  (void) (0 ? (val) ^ (val) : 0);                       \
                  (xsize) __sync_fetch_and_and ((atomic), (xptr)(val)); \
                  }))
#define x_atomic_ptr_or(atomic, val)                                    \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xptr));  \
                  (void) (0 ? (xptr) *(atomic) : 0);                    \
                  (void) (0 ? (val) ^ (val) : 0);                       \
                  (xsize) __sync_fetch_and_or ((atomic), (xptr)(val));  \
                  }))
#define x_atomic_ptr_xor(atomic, val)                                   \
    (X_EXTENSION ({                                                     \
                  X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xptr));  \
                  (void) (0 ? (xptr) *(atomic) : 0);                    \
                  (void) (0 ? (val) ^ (val) : 0);                       \
                  (xsize) __sync_fetch_and_xor ((atomic), (val));       \
                  }))

#else /* defined(G_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4) */
xint        x_atomic_int_get        (volatile xint  *atomic);

void        x_atomic_int_set        (volatile xint  *atomic,
                                     xint           value);
/** return result value */
xint        x_atomic_int_inc        (volatile xint  *atomic);
/** return result value */
xint        x_atomic_int_dec        (volatile xint  *atomic);
/** return origin value */
xint        x_atomic_int_add        (volatile xint  *atomic,
                                     xint           value);

xbool       x_atomic_int_cas        (volatile xint  *atomic,
                                     xint           oldval,
                                     xint           newval);
/** return origin value */
xuint       x_atomic_int_and        (volatile xuint *atomic,
                                     xuint          val);
/** return origin value */
xuint       x_atomic_int_or         (volatile xuint *atomic,
                                     xuint          val);
/** return origin value */
xuint       x_atomic_int_xor        (volatile xuint *atomic,
                                     xuint          val);

xptr        x_atomic_ptr_get        (volatile void  *atomic);

void        x_atomic_ptr_set        (volatile void  *atomic,
                                     xptr           val);
/** return origin value */
xssize      x_atomic_ptr_add        (volatile void  *atomic,
                                     xssize         val);

xbool       x_atomic_ptr_cas        (volatile void  *atomic,
                                     xptr           oldval,
                                     xptr           newval);
/** return origin value */
xsize       x_atomic_ptr_and        (volatile void  *atomic,
                                     xsize          val);
/** return origin value */
xsize       x_atomic_ptr_or         (volatile void  *atomic,
                                     xsize          val);
/** return origin value */
xsize       x_atomic_ptr_xor        (volatile void  *atomic,
                                     xsize          val);
#endif 
xbool       x_once_init_enter       (volatile void  *location);

void        x_once_init_leave       (volatile void  *location);

X_END_DECLS
#endif /* __X_ATOMIC_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
