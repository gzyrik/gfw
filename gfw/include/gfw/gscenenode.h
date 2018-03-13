#ifndef __G_SCENE_NODE_H__
#define __G_SCENE_NODE_H__
#include "gnode.h"
X_BEGIN_DECLS

#define G_TYPE_SCENE_NODE               (g_scene_node_type())
#define G_SCENE_NODE(object)            (X_INSTANCE_CAST((object), G_TYPE_SCENE_NODE, gSceneNode))
#define G_SCENE_NODE_CLASS(klass)       (X_CLASS_CAST((klass), G_TYPE_SCENE_NODE, gSceneNodeClass))
#define G_IS_SCENE_NODE(object)         (X_INSTANCE_IS_TYPE((object), G_TYPE_SCENE_NODE))
#define G_SCENE_NODE_GET_CLASS(object)  (X_INSTANCE_GET_CLASS((object), G_TYPE_SCENE_NODE, gSceneNodeClass))

/** Return the entire scene node bounding box */
gaabb*      g_scene_node_wbbox      (gSceneNode     *snode);

/** Setting the scene node automatic tracking */
void        g_scene_node_track      (gSceneNode     *snode,
                                     gNode          *target,
                                     const gvec     offset,
                                     const gvec     updir);
/** Mount a movable objects to a scene node.
 * Allows the movable object to mount a number of different scene nodes
 */
void        g_scene_node_attach     (gSceneNode     *snode,
                                     xptr           movable);

void        g_scene_node_detach     (gSceneNode     *snode,
                                     xptr           movable);

void        g_scene_node_collect    (gSceneNode     *snode,
                                     gRenderQueue   *queue,
                                     gVisibleBounds *bounds);

/** Traverse all movable objects on the scene node
 *  for each movable call func(movable, user_data)
 */
void        g_scene_node_foreach    (gSceneNode     *snode,
                                     xCallback      func,
                                     xptr           user_data);

struct _gSceneNode
{
    gNode           parent;
    gScene          *scene;
    xptr            priv;
};
struct _gSceneNodeClass
{
    gNodeClass      parent;
    void (*collect)  (gSceneNode *snode, gRenderQueue *queue, gVisibleBounds *bounds);
    void (*bounding) (gSceneNode *snode);
};

xType       g_scene_node_type       (void);

X_END_DECLS
#endif /* __G_SCENE_NODE_H__ */
