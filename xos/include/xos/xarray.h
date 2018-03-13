#ifndef __X_ARRAY_H__
#define __X_ARRAY_H__
#include "xtype.h"
X_BEGIN_DECLS

#define     x_array_index(array, type, i)                               \
    (((type*) (void *) (array)->data) [(i)])

#define     x_array_size(array) array->len

#define     x_ptr_array_index(array, type, i)                           \
    ((type*)((array)->data[(i)]))

#define     x_array_new(elem_size)  x_array_new_full(elem_size, 0, NULL)

xArray*     x_array_new_full        (xsize          element_size,
                                     xsize          reserved_size,
                                     xFreeFunc      free_func);

xArray*     x_array_dup             (xArray         *array);

xArray*     x_array_ref             (xArray         *array);

xint        x_array_unref           (xArray         *array);

xptr        x_array_at              (xArray         *array,
                                     xsize          i);

xptr        x_array_append          (xArray         *array,
                                     xcptr          data,
                                     xsize          len);

#define     x_array_append1(array, value)                               \
    x_array_append (array, value, 1)

xptr        x_array_prepend         (xArray         *array,
                                     xcptr          data,
                                     xsize          len);

#define     x_array_prepend1(array, value)                              \
    x_array_prepend (array, &(value), 1)

xptr        x_array_insert          (xArray         *array,
                                     xsize          index,
                                     xcptr          data,
                                     xsize          len);

#define     x_array_insert1(array, index, value)                        \
    x_array_insert (array, index, &(value), 1)

xsize       x_array_insert_sorted   (xArray         *array,
                                     xcptr          key,
                                     xptr           value,
                                     xCmpFunc       cmp_func,
                                     xbool          replace);

xssize      x_array_find_sorted     (xArray         *array,
                                     xcptr          key,
                                     xCmpFunc       cmp_func);

void        x_array_resize          (xArray         *array,
                                     xsize          size);

void        x_array_swap            (xArray         *array,
                                     xsize          i,
                                     xsize          j);

void        x_array_remove_range    (xArray         *array,
                                     xsize          i,
                                     xsize          n);
#define     x_array_remove(array, i)                                    \
    x_array_remove_range (array, i, 1)

void        x_array_erase_range     (xArray         *array,
                                     xsize          i,
                                     xsize          n);
#define     x_array_erase(array, i)                                    \
    x_array_erase_range (array, i, 1)

void        x_array_foreach         (xArray         *array,
                                     xVisitFunc      visit,
                                     xptr           user_data);

void        x_array_walk            (xArray         *array,
                                     xWalkFunc      func,
                                     xptr           user_data);

#define     x_ptr_array_new()       x_ptr_array_new_full(0, NULL)

xPtrArray*  x_ptr_array_new_full    (xsize          reserved_size,
                                     xFreeFunc      free_func);

xPtrArray*  x_ptr_array_dup         (xPtrArray      *array);

xPtrArray*  x_ptr_array_ref         (xPtrArray      *array);

xint        x_ptr_array_unref       (xPtrArray      *array);

xptr        x_ptr_array_append      (xPtrArray      *array,
                                     xcptr          *data,
                                     xsize          len);

#define     x_ptr_array_append1(array, value)                           \
    x_ptr_array_append (array, (xcptr*)&(value), 1)

#define     x_ptr_array_pop(array)                                      \
    (array->len > 0 ? array->data[--(array->len)] : NULL)

#define     x_ptr_array_push(array, value)                              \
    x_ptr_array_append (array, &(value), 1)

xptr        x_ptr_array_prepend     (xPtrArray      *array,
                                     xcptr          *data,
                                     xsize          len);

#define     x_ptr_array_prepend1(array, value)                          \
    x_ptr_array_prepend (array, &(value), 1)

xptr        x_ptr_array_insert      (xPtrArray      *array,
                                     xsize          index,
                                     xcptr          *data,
                                     xsize          len);

#define     x_ptr_array_insert1(array, index, value)                    \
    x_ptr_array_insert (array, index, &(value), 1)

xsize      x_ptr_array_insert_sorted(xPtrArray      *array,
                                     xcptr          key,
                                     xptr           value,
                                     xCmpFunc       cmp_func,
                                     xbool          replace);

xssize      x_ptr_array_find_sorted (xPtrArray      *array,
                                     xcptr          key,
                                     xCmpFunc       cmp_func);

xPtrArray*  x_ptr_array_resize      (xPtrArray      *array,
                                     xsize          size);

void        x_ptr_array_swap        (xPtrArray      *array,
                                     xsize          i,
                                     xsize          j);

void        x_ptr_array_foreach     (xPtrArray      *array,
                                     xVisitFunc     func,
                                     xptr           user_data);

void        x_ptr_array_walk        (xPtrArray      *array,
                                     xWalkFunc      func,
                                     xptr           user_data);

/**
 * Remove data from the array
 * It keep the order.
 *
 * @param [in] array
 * @param [in] data
 * @return TRUE if successfully, or return FALSE
 */
xbool       x_ptr_array_remove_data (xPtrArray      *array,
                                     xptr           data);

/**
 * Erase data from the array
 * It maybe disrupt the order.
 *
 * @param [in] array
 * @param [in] data
 * @return TRUE if successfully, or return FALSE
 */
xbool       x_ptr_array_erase_data  (xPtrArray      *array,
                                     xptr           data);

/**
 * Remove same continuous elements from the array
 * It keep the order.
 *
 * @param [in] array
 * @param [in] i The start index of elements
 * @param [in] n The number of elements, if < 0, will erase to end
 */
void        x_ptr_array_remove_range(xPtrArray      *array,
                                     xsize          i,
                                     xssize         n);
/**
 * Remove a element from the array
 * It keep the order.
 *
 * @param [in] array
 * @param [in] i The index of element
 */
#define     x_ptr_array_remove(array, i)                                \
    x_ptr_array_remove_range (array, i, 1)

/**
 * Erase same continuous elements from the array
 * It maybe disrupt the order.
 *
 * @param [in] array
 * @param [in] i The start index of elements
 * @param [in] n The number of elements, if < 0, will erase to end
 */
void        x_ptr_array_erase_range (xPtrArray      *array,
                                     xsize          i,
                                     xssize         n);
/**
 * Erase a element from the array
 * It maybe disrupt the order.
 *
 * @param [in] array
 * @param [in] i The index of element
 */
#define     x_ptr_array_erase(array, i)                                \
    x_ptr_array_erase_range (array, i, 1)

/**
 * Find the index of data in the array.
 * If the array hasn't the data, it return a negative
 *
 * @param [in] array 
 * @param [in] data
 * @return >=0 if successfully, or return < 0
 */
xint        x_ptr_array_find        (xPtrArray      *array,
                                     xcptr          data);


struct _xPtrArray
{
    /*< private >*/
    xptr            *data;
    xsize           len;
    xFreeFunc       free_func;
};

struct _xArray
{
    /*< private >*/
    xptr            data;
    xsize           len;
    xFreeFunc       free_func;
};

X_END_DECLS
#endif /* __X_ARRAY_H__ */

