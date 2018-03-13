#include "config.h"
#include "xatomicarray.h"
#include <string.h>
X_LOCK_DEFINE_STATIC (array);
static xTrash   *_freelist;
static xptr
freelist_alloc_L        (xsize          size,
                         xbool          reuse)
{
    xptr mem;
    xTrash *free, **prev;
    xsize real_size;

    if (reuse) {
        for (free = _freelist, prev = &_freelist;
             free != NULL; prev = &free->next, free = free->next) {
            if (X_ATOMIC_ARRAY_DATA_SIZE (free) == size) {
                *prev = free->next;
                return (xptr)free;
            }
        }
    }

    real_size = sizeof (xsize) + MAX (size, sizeof (xTrash));
    mem = x_slice_alloc (real_size);
    mem = ((char *) mem) + sizeof (xsize);
    X_ATOMIC_ARRAY_DATA_SIZE (mem) = size;
    return mem;
}
static inline void
freelist_free_L         (xptr           mem)
{
    x_trash_push (&_freelist, mem);
}
void
x_atomic_array_init     (xAtomicArray   *array)
{
    array->data = NULL;
}

xptr
x_atomic_array_copy     (xAtomicArray   *array,
                         xsize          header_size,
                         xsize          additional_element_size)
{
    xuint8 *new, *old;
    xsize old_size, new_size;

    X_LOCK (array);
    old = x_atomic_ptr_get (&array->data);
    if (old) {
        old_size = X_ATOMIC_ARRAY_DATA_SIZE (old);
        new_size = old_size + additional_element_size;
        /* Don't reuse if copying to same size, as this may end
         * up reusing the same pointer for the same array thus
         * confusing the transaction check
         */
        new = freelist_alloc_L (new_size, additional_element_size != 0);
        memcpy (new, old, old_size);
    }
    else if (additional_element_size != 0) {
        new_size = header_size + additional_element_size;
        new = freelist_alloc_L (new_size, TRUE);
    }
    else
        new = NULL;
    X_UNLOCK (array);
    return new;
}

void
x_atomic_array_update   (xAtomicArray   *array,
                         xptr           new_data)
{
    xuint8 *old;

    X_LOCK (array);
    old = x_atomic_ptr_get (&array->data);

    x_assert (old == NULL
              || X_ATOMIC_ARRAY_DATA_SIZE (old)
              <= X_ATOMIC_ARRAY_DATA_SIZE (new_data));

    x_atomic_ptr_set (&array->data, new_data);
    if (old)
        freelist_free_L (old);
    X_UNLOCK (array);
}
