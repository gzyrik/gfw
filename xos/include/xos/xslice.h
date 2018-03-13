#ifndef __X_SLICE_H__
#define __X_SLICE_H__
#include "xtype.h"
X_BEGIN_DECLS

xptr x_slice_alloc  (xsize block_size) X_MALLOC X_ALLOC_SIZE(1);
xptr x_slice_alloc0 (xsize block_size) X_MALLOC X_ALLOC_SIZE(1);
xptr x_slice_copy   (xsize block_size, 
                     xcptr mem_block) X_MALLOC X_ALLOC_SIZE(1);
void x_slice_free1  (xsize block_size, xptr  mem_block);
void x_slice_free_chain_with_offset (xsize block_size,
                                     xptr mem_chain,  xsize next_offset);
#define  x_slice_new(T)         ((T*) x_slice_alloc (sizeof (T)))
#define  x_slice_new0(T)        ((T*) x_slice_alloc0 (sizeof (T)))
#define  x_slice_dup(T, mem)                                    \
    (1 ? (T*) x_slice_copy (sizeof (T), (mem))                  \
     : ((void) ((T*) 0 == (mem)), (T*) 0))

#define  x_slice_free(T, mem)  X_STMT_START{                    \
    if (1) x_slice_free1 (sizeof (T), (mem));                   \
    else   (void) ((T*) 0 == (mem));                            \
} X_STMT_END

#define  x_slice_free_chain(T, mem_chain, next)  X_STMT_START{  \
    if (1) x_slice_free_chain_with_offset                       \
    (sizeof (T),(mem_chain), X_STRUCT_OFFSET (T, next));        \
    else   (void) ((T*) 0 == (mem_chain));                      \
} X_STMT_END

X_END_DECLS
#endif /* __X_SLICE_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
