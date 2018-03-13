#ifndef __X_ATOMIC_ARRAY_H__
#define __X_ATOMIC_ARRAY_H__
#include "xtype.h"
X_BEGIN_DECLS

#define X_ATOMIC_ARRAY_DATA_SIZE(mem) (*((xsize *) (mem) - 1))

#define X_ATOMIC_ARRAY_GET_LOCKED(_array, _type) ((_type *)((_array)->data))

#define X_ATOMIC_ARRAY_BEGIN_TRANSACTION(_array, _data) X_STMT_START{   \
    volatile xptr *__datap  = &(_array)->data;                          \
    volatile xptr __check   = x_atomic_ptr_get (__datap);               \
    do {                                                                \
        *(xptr*)&_data = __check;

#define X_ATOMIC_ARRAY_END_TRANSACTION(_data)                           \
        __check = x_atomic_ptr_get (__datap);                           \
    } while ((xptr)_data != __check);                                   \
} X_STMT_END

void        x_atomic_array_init     (xAtomicArray   *array);

xptr        x_atomic_array_copy     (xAtomicArray   *array,
                                     xsize          header_size,
                                     xsize          additional_element_size);

void        x_atomic_array_update   (xAtomicArray   *array,
                                     xptr           new_data);

struct _xAtomicArray
{
    volatile xptr     data;
};

X_END_DECLS

#endif  /* __X_ATOMIC_ARRAY_H__ */
