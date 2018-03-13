#ifndef __X_GC_H__
#define __X_GC_H__
#include "xtype.h"
X_BEGIN_DECLS

/**
 * Allocate a gc memory block
 *
 * @param[in] gc        The GC manager
 * @param[in] size      Byte number to request.
 * @param[in] parent    GC object of the block life time depend on
 * @param[in] finalize  Callback when the block free
 *
 * @return Memory block if successfully, or return NULL.
 *
 * @remark
 *  The #parent can be a gc memory block, a weak table, or NULL.
 *  If #parent is NULL or a weak table, the return memory block is temporary.
 *
 * @see x_gc_realloc
 */
xptr        x_gc_alloc              (xGC            *gc,
                                     xsize          size,
                                     xptr           parent,
                                     xFreeFunc      finalize);

/**
 * Adjust the size of gc memory allocated
 *
 * @param[in] gc        The GC manager
 * @param[in] p         Memory block return by x_gc_alloc
 * @param[in] size      New byte number to request
 * @param[in] parent    GC object of the block life time depend on
 *
 * @return Memory block if successfully, or return NULL
 *
 * @remark
 *  The #parent can be a gc memory block, a weak table, or NULL.
 *  If #parent is NULL or a weak table, the return memory block is temporary.
 *  If #p is NULL, it equals to x_gc_alloc
 *
 * @see x_gc_alloc
 */
xptr        x_gc_realloc            (xGC            *gc,
                                     xptr           p,
                                     xsize          size,
                                     xptr           parent);

/** 
 * Clone the memory block and its gc information.
 * 
 * @param[in] gc        The GC manager
 * @param[in] from      Memory block return by x_gc_alloc or x_gc_realloc
 * @param[in] size      Byte number to data copy
 *
 * @return  Memory block if successfully, or return NULL.
 *
 * @remark
 *  The gc information inclues callback, dependents and so on.
 */
xptr        x_gc_clone              (xGC            *gc,
                                     xptr           from,
                                     xsize          size);

/** 
 * Enter a new gc context
 *
 * @param[in] gc GC manager
 *
 * @see x_gc_leave
 */
void        x_gc_enter              (xGC            *gc);


/** Leave the current gc context.
 * The temporary memory in the context will be marked unused.
 * except the arguments #p and so.
 *
 * @param[in] gc        The GC manager
 * @param[in] p         gc object to be left on stack
 *
 * @see x_gc_enter
 */
void        x_gc_leave              (xGC            *gc,
                                     xptr           p,
                                     ...) X_NULL_TERMINATED;

/** 
 * Collect unused memory block.
 * This will collect all the memory block that can no longer be 
 * otherwise accessed. Before free it , collector will call the 
 * finalizer if you provided.
 *
 * @param[in] gc        GC manager
 */
void        x_gc_collect            (xGC            *gc);

/** 
 * Establish or remove the relationship of two gc objects.
 *
 * @param[in] gc        The GC manager
 * @param[in] parent    GC object of the life time depend on
 * @param[in] prev      To unlink gc object which life time depend on the #parent
 * @param[in] now       To link gc object which life time will depend on the #parent
 *
 * @remark
 *  For example, a->b = c;//a,b and c are both gc object
 *  You should call, x_gc_link (gc, a, a->b, c);
 *  You can also link a gc object to the global by pass #parent NULL,
 */
void        x_gc_link               (xGC            *gc,
                                     xptr           parent,
                                     xptr           prev,
                                     xptr           now);

/** 
 * Simulate collecting.
 * Only for debug, it will print some useful information instead of 
 * collecting the garbage
 * 
 * @param[in] gc        The GC manager
 *
 * @return debug information, need call x_free() after unused.
 */
xstr        x_gc_dryrun             (xGC            *gc,
                                     xcstr          format,
                                     ...) X_PRINTF (2, 3) X_MALLOC;

/**
 * Create weak table.
 *
 * @param[in] gc        The GC manager
 * @param[in] parent    GC object of the life time depend on
 *
 * @return Weak table if successfully, or return NULL.
 *
 * @remark
 *  The #parent can be a gc memory block, a weak table, or NULL.
 *  If #parent is NULL or a weak table, the return is temporary.
 */
xptr        x_gc_wtable_new         (xGC            *gc,
                                     xptr           parent);

/**
 * Iterate in the weak table.
 * 
 * @param[in] gc        The GC manager
 * @param[in] wtable    Weak table object
 * @param[in,out] iter  Index of the weak table
 *
 * @return weak pointer, or NULL
 *
 */
xptr        x_gc_wtable_next        (xGC            *gc,
                                     xptr           wtable,
                                     xsize          *iter);

/** Create gc manager
 *
 * @return GC manager if successfully, or return NULL
 */
xGC*        x_gc_new                (void);

/** 
 * Free all gc environment.
 * You may not call x_gc_delete at the end, becuase the OS will release
 * all the resouce used by your process.
 *
 * @param[in] gc        The GC manager
 */
void        x_gc_delete             (xGC            *gc);

X_END_DECLS
#endif /* __X_GC_H__ */
