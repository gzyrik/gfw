#include "gfw-octree.h"
X_DEFINE_DYNAMIC_TYPE (octNode, oct_node, G_TYPE_SCENE_NODE);
xType
_oct_node_register      (xTypeModule    *module)
{
    return oct_node_register (module);
}

static void
onode_bounding          (gSceneNode     *snode)
{
    G_SCENE_NODE_CLASS(oct_node_parent_class)->bounding(snode);
    if (g_scene_node_wbbox(snode)->extent> 0)
        oct_scene_refresh ((octScene *)(snode->scene), (octNode*)snode);
}
static void enqueue_movable (gMovable *movable, gMovableContext*context)
{
    g_movable_enqueue(movable, context);
}
static void
onode_collect           (gSceneNode     *snode,
                         gRenderQueue   *queue,
                         gVisibleBounds *bounds)
{
    gMovableContext context={bounds->camera,queue,(gNode*)snode};
    g_scene_node_foreach (snode, enqueue_movable, &context);
}
static void
oct_node_init           (octNode     *onode)
{

}
static void
oct_node_class_init     (octNodeClass *klass)
{
    gSceneNodeClass *sclass;

    sclass = G_SCENE_NODE_CLASS (klass);
    sclass->bounding = onode_bounding;
    sclass->collect  = onode_collect;
}
static void
oct_node_class_finalize (octNodeClass *klass)
{

}
