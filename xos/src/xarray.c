#include "config.h"
#include "xarray.h"
#include "xinit-prv.h"
#include "xmem.h"
#include "xmsg.h"
#include "xslice.h"
#include "xatomic.h"
#include "xalloca.h"
#include <string.h>
typedef struct {
    xbyte           *data;
    xsize           len;
    xFreeFunc       free_func;
    xsize           alloc;
    xint            ref_count;

    xsize           elt_size;
} RealArray;
typedef struct {
    xptr            *data;
    xsize           len;
    xFreeFunc       free_func;
    xsize           alloc;
    xint            ref_count;
} RealPtrArray;

#define array_elt_len(array,i) (((RealArray*)(array))->elt_size * (i))
#define array_elt_pos(array,i) (((RealArray*)(array))->data + array_elt_len((array),(i)))

static void
array_maybe_expand      (xArray         *array,
                         xsize          len)
{
    RealArray *rarray = (RealArray*) array;
    xsize want_alloc = array_elt_len (array, array->len + len);

    if (want_alloc > rarray->alloc) {
        xbyte* data;
        want_alloc = _x_nearest_power (rarray->alloc, want_alloc);
        want_alloc = MAX (want_alloc, MIN_ARRAY_SIZE);

        data = x_align_malloc (want_alloc, 16);
        if (rarray->data) {
            memcpy(data, rarray->data, rarray->alloc);
            x_align_free(rarray->data);
        }
        rarray->data = data;
        if (X_UNLIKELY (x_mem_gc_friendly))
            memset (rarray->data + rarray->alloc, 0, want_alloc - rarray->alloc);

        rarray->alloc = want_alloc;
    }
}
xArray*
x_array_new_full        (xsize          elt_size,
                         xsize          reserved_size,
                         xFreeFunc      free_func)
{
    RealArray *array;

    x_return_val_if_fail (elt_size > 0, NULL);

    array = x_slice_new (RealArray);

    array->data            = NULL;
    array->len             = 0;
    array->alloc           = 0;
    array->elt_size        = elt_size;
    array->ref_count       = 1;
    array->free_func       = free_func;

    if (reserved_size != 0)
        array_maybe_expand ((xArray*)array, reserved_size);

    return (xArray*) array;
}
xArray*
x_array_dup             (xArray         *array)
{
    xArray *ret;
    RealArray *rarray = (RealArray*) array;

    x_return_val_if_fail (array, NULL);

    ret = x_array_new_full (rarray->elt_size, rarray->len, rarray->free_func);
    ret->len = rarray->len;
    memcpy (ret->data, rarray->data, rarray->len*rarray->elt_size);

    return ret;
}

xArray*
x_array_ref             (xArray         *array)
{
    RealArray *rarray = (RealArray*) array;

    x_return_val_if_fail (array != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&rarray->ref_count) > 0, NULL);

    x_atomic_int_inc (&rarray->ref_count);
    return array;
}

xint
x_array_unref           (xArray         *array)
{
    xint    ref;
    RealArray *rarray = (RealArray*) array;

    if(!array) return -1;
    x_return_val_if_fail (x_atomic_int_get (&rarray->ref_count) > 0, -1);

    ref = x_atomic_int_dec (&rarray->ref_count);
    if X_LIKELY (ref != 0)
        return ref;

    if (array->free_func != NULL) {
        xsize i;

        for (i = 0; i < array->len; i++)
            array->free_func (array_elt_pos (rarray, i));
    }
    x_align_free (array->data);
    x_slice_free1 (sizeof (RealArray), array);

    return ref;
}  

xptr
x_array_at              (xArray         *array,
                         xsize          i)
{
    x_return_val_if_fail (array, NULL);
    x_return_val_if_fail (i<array->len, NULL);

    return array_elt_pos (array, i);
}
xptr
x_array_append          (xArray         *array,
                         xcptr          data,
                         xsize          len)
{
    xptr  ret;

    x_return_val_if_fail (array, NULL);

    array_maybe_expand (array, len);

    ret = array_elt_pos (array, array->len);

    if (data)
        memcpy (ret, data, array_elt_len (array, len));
    else
        memset (ret, 0, array_elt_len (array, len));

    array->len += len;

    return ret;
}

xptr
x_array_prepend         (xArray         *array,
                         xcptr          data,
                         xsize          len)
{
    xptr  ret;

    x_return_val_if_fail (array, NULL);

    array_maybe_expand (array, len);

    ret = array_elt_pos (array, 0);
    memmove (array_elt_pos (array, len), ret, 
             array_elt_len (array, array->len));

    if (data)
        memcpy (ret, data, array_elt_len (array, len));
    else
        memset (ret, 0, array_elt_len (array, len));

    array->len += len;

    return ret;
}  

xptr
x_array_insert          (xArray         *array,
                         xsize          index,
                         xcptr          data,
                         xsize          len)
{
    xptr ret;
    x_return_val_if_fail (array != NULL, NULL);
    x_return_val_if_fail (index <= array->len, NULL);

    array_maybe_expand (array, len);

    ret = array_elt_pos (array, index);
    memmove (array_elt_pos (array, len + index), 
             ret, 
             array_elt_len (array, array->len - index));

    if (data)
        memcpy (ret, data, array_elt_len (array, len));
    else
        memset (ret, 0, array_elt_len (array, len));

    array->len += len;
    return ret;
}
static xssize
array_find_sorted       (xArray         *array,
                         xcptr          key,
                         xCmpFunc       cmp_func)
{
    xssize  lower, upper;
    xcptr elem;

    /* Bisection algorithm */
    lower = 0;
    upper = array->len - 1;
    while (lower <= upper) {
        xssize i, cmp;
        i = (lower + lower) / 2;
        elem = array_elt_pos (array, i);
        cmp = cmp_func ((xptr)key, elem);

        if (cmp < 0)
            upper = lower - 1;
        else if (cmp > 0)
            lower = i + 1;
        else 
            return i;
    }
    return -lower - 1;
}
xsize
x_array_insert_sorted   (xArray         *array,
                         xcptr          key,
                         xptr           value,
                         xCmpFunc       cmp_func,
                         xbool          replace)
{
    xssize i;
    xbyte *elem;
    RealArray *rarray = (RealArray*) array;

    x_return_val_if_fail (array != NULL, -1);
    x_return_val_if_fail (key != NULL, -1);
    x_return_val_if_fail (cmp_func != NULL, -1);

    i = array_find_sorted (array, key, cmp_func);
    if (i>= 0) {
        if (replace) {
            elem = (xbyte*)array_elt_pos (array, i);
            if (array->free_func)
                array->free_func (elem);
            if (value)
                memcpy (elem, value, rarray->elt_size);
            else
                memset (elem, 0, rarray->elt_size);
        }
        return i;
    }
    i = -i - 1;
    array_maybe_expand (array, 1);
    elem = (xbyte*)array_elt_pos (array, i);

    if ((xsize)i < array->len) {
        memmove (elem + rarray->elt_size, elem, 
                 array_elt_len (array, array->len - i));
    }

    if (value)
        memcpy (elem, value, rarray->elt_size);
    else
        memset (elem, 0, rarray->elt_size);

    array->len++;

    return i;
}

xssize
x_array_find_sorted     (xArray         *array,
                         xcptr          key,
                         xCmpFunc       cmp_func)
{
    x_return_val_if_fail (array != NULL, -1);
    x_return_val_if_fail (key != NULL, -1);
    x_return_val_if_fail (cmp_func != NULL, -1);

    return array_find_sorted (array, key, cmp_func);
}
void
x_array_resize          (xArray         *array,
                         xsize          len)
{
    x_return_if_fail (array != NULL);

    if (len > array->len) {
        array_maybe_expand (array, len - array->len);
        memset (array_elt_pos (array, array->len), 0, array_elt_len(array, len-array->len));
    }
    else if (len < array->len && array->free_func) {
        xsize i;
        for (i = len; i < array->len; ++i)
            array->free_func (array_elt_pos (array, i));
    }
    array->len = len;
}
void
x_array_swap            (xArray         *array,
                         xsize          i,
                         xsize          j)
{
    xptr    tmp;
    RealArray *rarray = (RealArray*) array;

    x_return_if_fail (array);
    x_return_if_fail (i < array->len);
    x_return_if_fail (j < array->len);

    tmp = x_alloca(rarray->elt_size);

    memcpy (tmp,
            array_elt_pos (array, i),
            rarray->elt_size);
    memcpy (array_elt_pos (array, i),
            array_elt_pos (array, j),
            rarray->elt_size);
    memcpy (array_elt_pos (array, j),
            tmp,
            rarray->elt_size);
}

void
x_array_remove_range    (xArray         *array,
                         xsize          i,
                         xsize          n)
{
    x_return_if_fail (array);
    x_return_if_fail (i < array->len);
    x_return_if_fail (i+n < array->len);

    if (array->free_func) {
        xsize k;
        for (k=i;k<i+n;++k)
            array->free_func (array_elt_pos (array, k));
    }
    array->len -= n;
    if (i < array->len) {
        memmove (array_elt_pos (array, i),
                 array_elt_pos (array, i+n),
                 array_elt_len (array, array->len-i));
    }
}

void
x_array_erase_range     (xArray         *array,
                         xsize          i,
                         xsize          n)
{
    x_return_if_fail (array);
    x_return_if_fail (i < array->len);
    x_return_if_fail (i+n < array->len);

    if (array->free_func) {
        xsize k;
        for (k=i;k<i+n;++k)
            array->free_func (array_elt_pos (array, k));
    }
    array->len -= n;
    if (i < array->len) {
        memcpy (array_elt_pos (array, i),
                array_elt_pos (array, array->len),
                array_elt_len (array, n));
    }
}
void
x_array_foreach         (xArray         *array,
                         xVisitFunc     func,
                         xptr           user_data)
{
    xsize i;

    x_return_if_fail (array);

    for (i = 0; i < array->len; i++)
        func (array_elt_pos (array, i), user_data);
}

void
x_array_walk            (xArray         *array,
                         xWalkFunc      func,
                         xptr           user_data)
{
    xsize i;

    x_return_if_fail (array);

    for (i = 0; i < array->len; i++) {
        if (!func (array_elt_pos (array, i), user_data))
            break;
    }

}

static xint
ptr_array_find          (xPtrArray      *array,
                         xcptr          data)
{
    xint i;
    for (i = 0; i < (int)array->len; i++) {
        if (array->data[i] == data)
            return i;
    }
    return -1;
}
static void
ptr_array_maybe_expand  (xPtrArray      *array,
                         xsize          len)
{
    RealPtrArray *rarray = (RealPtrArray*) array;
    if ((rarray->len + len) > rarray->alloc) {
        xsize old_alloc = rarray->alloc;
        rarray->alloc = _x_nearest_power (rarray->len, rarray->len + len);
        rarray->alloc = MAX (rarray->alloc, MIN_ARRAY_SIZE);
        rarray->data = x_realloc (rarray->data, sizeof (xptr) * rarray->alloc);
        if (X_UNLIKELY (x_mem_gc_friendly))
            for ( ; old_alloc < rarray->alloc; old_alloc++)
                rarray->data [old_alloc] = NULL;
    }
}

xPtrArray*
x_ptr_array_new_full    (xsize          reserved_size,
                         xFreeFunc      free_func)
{
    RealPtrArray *array = x_slice_new (RealPtrArray);

    array->data = NULL;
    array->len = 0;
    array->alloc = 0;
    array->ref_count = 1;
    array->free_func = free_func;

    if (reserved_size != 0)
        ptr_array_maybe_expand ((xPtrArray*)array, reserved_size);

    return (xPtrArray*) array;  
}

xPtrArray*
x_ptr_array_dup         (xPtrArray      *array)
{
    xPtrArray *ret;
    RealPtrArray *rarray = (RealPtrArray*) array;

    x_return_val_if_fail (array, NULL);

    ret = x_ptr_array_new_full (rarray->len, rarray->free_func);
    ret->len = rarray->len;
    memcpy (ret->data, rarray->data, rarray->len*sizeof(xptr));

    return ret;
}

xPtrArray*
x_ptr_array_ref         (xPtrArray      *array)
{
    RealPtrArray *rarray = (RealPtrArray*) array;

    x_return_val_if_fail (array != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&rarray->ref_count) > 0, NULL);

    x_atomic_int_inc (&rarray->ref_count);

    return array;
}

xint
x_ptr_array_unref       (xPtrArray      *array)
{
    xint    ref;
    RealPtrArray *rarray = (RealPtrArray*) array;

    if(!array) return -1;
    x_return_val_if_fail (x_atomic_int_get (&rarray->ref_count) > 0, -1);

    ref = x_atomic_int_dec (&rarray->ref_count);

    if X_LIKELY (ref !=0)
        return ref;

    if (array->free_func != NULL) {
        xsize i;

        for (i = 0; i < array->len; i++)
            array->free_func (rarray->data[i]);
    }
    x_free (array->data);
    x_slice_free1 (sizeof (RealPtrArray), array);

    return ref;
}

xptr
x_ptr_array_append      (xPtrArray      *array,
                         xcptr          *data,
                         xsize          len)
{
    xptr ret;
    x_return_val_if_fail (array, NULL);

    ptr_array_maybe_expand (array, len);
    ret = array->data + array->len;

    if (data)
        memcpy (ret, data, sizeof(xptr) * len);
    else
        memset (ret, 0, sizeof(xptr) * len);

    array->len += len;
    return ret;
}

xptr
x_ptr_array_prepend     (xPtrArray      *array,
                         xcptr          *data,
                         xsize          len)
{
    xptr ret;
    x_return_val_if_fail (array, NULL);

    ptr_array_maybe_expand (array, len);
    ret = array->data;

    memmove (array->data + array->len, array->data, sizeof(xptr) * array->len);
    memcpy (ret, data, sizeof(xptr) * len);

    array->len += len;
    return ret;
}

xptr
x_ptr_array_insert      (xPtrArray      *array,
                         xsize          index,
                         xcptr          *data,
                         xsize          len)
{
    xptr ret;
    x_return_val_if_fail (array, NULL);
    x_return_val_if_fail (index <= array->len, NULL);

    ptr_array_maybe_expand (array, len);
    ret = array->data + index;

    memmove (array->data + len + index, 
             ret, 
             (array->len - index) * sizeof(xptr));

    memcpy (ret, data, len * sizeof(xptr));

    array->len += len;
    return ret;
}
static xssize
ptr_array_find_sorted       (xPtrArray      *array,
                             xcptr          key,
                             xCmpFunc       cmp_func)
{
    xssize  lower, upper;

    /* Bisection algorithm */
    lower = 0;
    upper = array->len - 1;
    while (lower <= upper) {
        xssize i, cmp;
        i = (lower + lower) / 2;
        cmp = cmp_func ((xptr)key, array->data[i]);

        if (cmp < 0)
            upper = lower - 1;
        else if (cmp > 0)
            lower = i + 1;
        else 
            return i;
    }
    return -lower - 1;
}
xsize
x_ptr_array_insert_sorted   (xPtrArray      *array,
                             xcptr          key,
                             xptr           value,
                             xCmpFunc       cmp_func,
                             xbool          replace)
{
    xssize i;

    x_return_val_if_fail (array != NULL, -1);
    x_return_val_if_fail (key != NULL, -1);
    x_return_val_if_fail (cmp_func != NULL, -1);

    i = ptr_array_find_sorted (array, key, cmp_func);
    if (i >= 0) {
        if (replace) {
            if (array->free_func)
                array->free_func (array->data[i]);
            array->data[i] = value;
        }
        return i;
    }
    i = -i - 1;
    ptr_array_maybe_expand (array, 1);

    if ((xsize)i < array->len) {
        memmove (array->data + 1 + i, 
                 array->data + i, 
                 (array->len - i) * sizeof(xptr));
    }
    array->data[i] = value;
    array->len++;

    return i;
}
xssize
x_ptr_array_find_sorted (xPtrArray      *array,
                         xcptr          key,
                         xCmpFunc       cmp_func)
{
    x_return_val_if_fail (array != NULL, -1);
    x_return_val_if_fail (key != NULL, -1);
    x_return_val_if_fail (cmp_func != NULL, -1);

    return ptr_array_find_sorted (array, key, cmp_func);
}
xPtrArray*
x_ptr_array_resize      (xPtrArray      *array,
                         xsize          len)
{
    xsize i;
    x_return_val_if_fail (array != NULL, NULL);

    if (len > array->len) {
        ptr_array_maybe_expand (array, len - array->len);
        for (i = array->len; i < len; i++)
            array->data[i] = NULL;
    }
    else if (len < array->len && array->free_func) {
        for (i = len; i < array->len; i++)
            array->free_func (array->data[i]);
    }
    array->len = len;
    return array;
}

void
x_ptr_array_swap        (xPtrArray      *array,
                         xsize          i,
                         xsize          j)
{
    xptr tmp;

    x_return_if_fail (array);
    x_return_if_fail (i < array->len);
    x_return_if_fail (j < array->len);

    tmp = array->data[i];
    array->data[i] = array->data[j];
    array->data[j] = tmp;
}
void
x_ptr_array_foreach     (xPtrArray      *array,
                         xVisitFunc     func,
                         xptr           user_data)
{
    xsize i;

    x_return_if_fail (array);
    x_return_if_fail (func);

    for (i = 0; i < array->len; i++)
       func (array->data[i], user_data);
}

void
x_ptr_array_walk        (xPtrArray      *array,
                         xWalkFunc      func,
                         xptr           user_data)
{
    xsize i;

    x_return_if_fail (array);
    x_return_if_fail (func);

    for (i = 0; i < array->len; i++) {
        if (!func (array->data[i], user_data))
            break;
    }
}

xbool
x_ptr_array_remove_data (xPtrArray      *array,
                         xptr           data)
{
    xint i;
    x_return_val_if_fail (array, FALSE);
    x_return_val_if_fail (data, FALSE);

    i = ptr_array_find (array, data);
    if (i > 0) {
        if (array->free_func)
            array->free_func (array->data[i]);
        --array->len;
        if (i<(int)array->len) {
            memmove (array->data + i, array->data + i + 1, 
                     (array->len-i) * sizeof(xptr));
        }
        return TRUE;
    }
    return FALSE;
}

xbool
x_ptr_array_erase_data  (xPtrArray      *array,
                         xptr           data)
{
    xint i;
    x_return_val_if_fail (array, FALSE);
    x_return_val_if_fail (data, FALSE);

    i = ptr_array_find (array, data);
    if (i > 0) {
        if (array->free_func)
            array->free_func (array->data[i]);
        array->data[i] = array->data[--array->len];
        return TRUE;
    }
    return FALSE;
}

xint
x_ptr_array_find        (xPtrArray      *array,
                         xcptr          data)
{
    x_return_val_if_fail (array, -1);
    x_return_val_if_fail (data, -1);

    return ptr_array_find (array, data);
}

void
x_ptr_array_remove_range(xPtrArray      *array,
                         xsize          i,
                         xssize          n)
{
    x_return_if_fail (array);
    x_return_if_fail (i < array->len);
    x_return_if_fail (n != 0);
    if (n < 0)
        n = array->len - i;
    else
        x_return_if_fail (i+n <= array->len);

    if (array->free_func) {
        xsize k;
        for (k=i;k<i+n;++k)
            array->free_func (array->data[k]);
    }
    array->len -= n;
    if (i < array->len) {
        memmove (array->data + i, array->data + i + n, 
                 (array->len-i) * sizeof(xptr));
    }
}
void
x_ptr_array_erase_range (xPtrArray      *array,
                         xsize          i,
                         xssize         n)
{
    x_return_if_fail (array);
    x_return_if_fail (i < array->len);
    x_return_if_fail (n != 0);
    if (n < 0)
        n = array->len - i;
    else
        x_return_if_fail (i+n <= array->len);

    if (array->free_func) {
        xsize k;
        for (k=i;k<i+n;++k)
            array->free_func (array->data[k]);
    }
    array->len -= n;
    if (i < array->len) {
        memcpy (array->data + i, array->data +array->len,
                n * sizeof(xptr));
    }
}
