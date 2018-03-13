#ifndef __X_MALLOC_H__
#define __X_MALLOC_H__
#include "xtype.h"
X_BEGIN_DECLS

xptr            x_malloc                (xsize          bytes);

xptr            x_align_malloc          (xsize          bytes,
                                         xsize          align);

xptr            x_align_malloc0         (xsize          bytes,
                                         xsize          align);

void            x_align_free            (xptr           ptr);

xptr            x_malloc0               (xsize          bytes);

xptr            x_realloc               (xptr           ptr,
                                         xsize          bytes);

xptr            x_memdup                (xcptr          ptr,
                                         xsize          bytes);

void            x_free                  (xptr           ptr);

void            x_set_gc_friendly       (xbool          clean);

#define         x_new(T, num)           (T*) x_malloc(sizeof(T)*(num))

#define         x_new0(T,num)           (T*) x_malloc0(sizeof(T)*(num))

#define         x_renew(T, ptr, num)    x_realloc(ptr, sizeof(T)*(num))

X_END_DECLS
#endif /* __X_MALLOC_H__ */

