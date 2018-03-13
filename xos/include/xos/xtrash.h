#ifndef __X_TRASH_H__
#define __X_TRASH_H__
#include "xtype.h"
X_BEGIN_DECLS

X_INLINE_FUNC
void        x_trash_push            (xTrash         **trash,
                                     xptr           data);
X_INLINE_FUNC
xptr        x_trash_pop             (xTrash         **trash);
X_INLINE_FUNC
xptr        x_trash_peek            (xTrash         **trash);
X_INLINE_FUNC
xuint       x_trash_depth           (xTrash         **trash);

struct _xTrash
{
    xTrash *next;
};

#if defined (X_CAN_INLINE) || defined (__X_TRASH_C__)
X_INLINE_FUNC void
x_trash_push            (xTrash         **trash,
                         xptr           data_p)
{
    xTrash* data = (xTrash*)data_p;
    data->next = *trash;
    *trash = data;
}

X_INLINE_FUNC xptr
x_trash_pop    (xTrash** trash)
{
    xTrash* data = *trash;
    if(data) {
        *trash = data->next;
        data->next = NULL;
    }
    return data;
}

X_INLINE_FUNC xptr
x_trash_peek            (xTrash         **trash)
{
    xTrash *data = *trash;
    return data;
}

X_INLINE_FUNC xuint
x_trash_depth           (xTrash         **trash)
{
    xTrash *data;
    xuint i = 0;

    for (data = *trash; data; data = data->next)
        ++i;
    return i;
}
#endif  /* X_CAN_INLINE || __X_TRASH_C__ */

X_END_DECLS
#endif /* __X_TRASH_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
