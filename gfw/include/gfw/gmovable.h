#ifndef __G_MOVABLE_H__
#define __G_MOVABLE_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_MOVABLE              (g_movable_type())
#define G_MOVABLE(object)           (X_INSTANCE_CAST((object), G_TYPE_MOVABLE, gMovable))
#define G_MOVABLE_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_MOVABLE, gMovableClass))
#define G_IS_MOVABLE(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_MOVABLE))
#define G_MOVABLE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_MOVABLE, gMovableClass))

typedef enum  {
    G_QUERY_ENTITY      =1<<31,
    G_QUERY_FRUSTUM     =1<<30,
    G_QUERY_LIGHT       =1<<29,
    G_QUERY_STATIC      =1<<28,
    G_QUERY_BBOARD      =1<<27,
    G_QUERY_PARTICLE    =1<<26,
    G_QUERY_SKY         =1<<25,
} gQueryFlags;

/** Get named gMovable instance
 * @param[in] name Unique name in all gMovable instances
 * @return The only object that owns the name
 */
gMovable*   g_movable_get           (xcstr          name);

gMovable*   g_movable_new           (xcstr          type,
                                     xcstr          first_property,
                                     ...);

xbool       g_movable_match         (gMovable       *movable,
                                     xuint          type_mask,
                                     xuint          query_mask);
/** 将可移动物体放入渲染队列 */
void        g_movable_enqueue       (gMovable       *movable,
                                     gMovableContext*context);

/** 设置可移动物体的本地边界盒 */
void        g_movable_resize        (gMovable       *movable,
                                     const gaabb    *aabb);

/** 返回可移动物体的本地边界盒 */
gaabb*      g_movable_lbbox         (gMovable       *movable);

/** 返回可移动物体在场景结点下的绝对边界盒
 *  绝对边界盒等于本地边界盒经过场景结点变换的结果
 */
gaabb*      g_movable_wbbox         (gMovable       *movable,
                                     gSceneNode     *snode);
struct _gMovableContext
{
    gCamera         *camera;
    gRenderQueue    *queue;
    gNode           *node;
};
struct _gMovable
{
    xObject         parent;
    xptr            priv;
};

struct _gMovableClass
{
    xObjectClass    parent;
    xuint           type_flags;

    void    (*enqueue) (gMovable *movable, gMovableContext* context);
    void    (*attached) (gMovable *movable, gNode *node);
    void    (*detached) (gMovable *movable, gNode *node);
    void    (*resized)  (gMovable *movable);
};

xType       g_movable_type          (void);

X_END_DECLS
#endif /* __G_MOVABLE_H__ */
