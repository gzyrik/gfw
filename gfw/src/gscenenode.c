#include "config.h"
#include "gscenenode.h"
#include "gmovable.h"
#include "gcamera.h"
#include "gscene.h"
#include "gsimple.h"
#include "gfw-prv.h"

X_DEFINE_TYPE (gSceneNode, g_scene_node, G_TYPE_NODE);
#define SCENE_NODE_PRIVATE(o) ((struct SceneNodePrivate*) (G_SCENE_NODE (o)->priv))

enum {
    PROP_0,
    PROP_WBBOX_VISIBLE,
    N_PROPERTIES
};
struct MovableStub

{
    gaabb           wbbox;
};
struct SceneNodePrivate
{
    gNode           *tracking_target;
    gvec            tracking_offset;
    gvec            tracking_updir;
    xHashTable      *movables;
    xbool           bounding_dirty;
    gaabb           world_aabb;
    gBoundingBox    *boundingbox;
};

static void
set_property            (xObject            *snode,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    struct SceneNodePrivate *priv;
    priv = SCENE_NODE_PRIVATE (snode);

    switch(property_id) {
    case PROP_WBBOX_VISIBLE:
        if (x_value_get_bool (value)) {
            if (!priv->boundingbox) {
                priv->boundingbox = x_object_new (G_TYPE_BOUNDINGBOX, NULL);
                priv->boundingbox->world_space = TRUE;
                g_boundingbox_set (priv->boundingbox, &priv->world_aabb);
            }
        }
        else if (priv->boundingbox) {
            x_object_unref (priv->boundingbox);
            priv->boundingbox = NULL;
        }
        break;
    }
}
static void
get_property            (xObject            *snode,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    struct SceneNodePrivate *priv;
    priv = SCENE_NODE_PRIVATE (snode);

    switch(property_id) {
    case PROP_WBBOX_VISIBLE:
        x_value_set_bool (value, priv->boundingbox != NULL);
        break;
    }
}
static void
g_scene_node_init (gSceneNode     *snode)
{
    struct SceneNodePrivate *priv;
    priv = x_instance_private(snode, G_TYPE_SCENE_NODE);
    snode->priv = priv;

    priv->movables = x_hash_table_new_full (x_direct_hash, x_direct_cmp,
                                            x_object_unref,x_free);
}
static void
snode_finalize          (xObject        *snode)
{
    struct SceneNodePrivate *priv;
    priv = SCENE_NODE_PRIVATE (snode);

    x_hash_table_ref (priv->movables);
}
static xbool
merge_child (gSceneNode *child, struct SceneNodePrivate *priv)
{
    g_aabb_merge (&priv->world_aabb, g_scene_node_wbbox (child));
    return TRUE;
}
static void
merge_movable (gMovable  *movable, struct MovableStub *stub, gSceneNode *snode)
{
    struct SceneNodePrivate *priv;
    priv = SCENE_NODE_PRIVATE (snode);

    stub->wbbox = *g_movable_lbbox (movable);
    g_aabb_affine (&stub->wbbox, g_node_xform (G_NODE (snode)));
    g_aabb_merge (&priv->world_aabb, &stub->wbbox); 
}
static void
snode_bounding          (gSceneNode     *snode)
{
    struct SceneNodePrivate *priv;
    priv = SCENE_NODE_PRIVATE (snode);

    priv->world_aabb.extent = 0;

    x_hash_table_foreach (priv->movables, merge_movable, snode);
    g_node_foreach (G_NODE (snode), merge_child, priv);

    if (priv->boundingbox)
        g_boundingbox_set(priv->boundingbox, &priv->world_aabb);
}
static void
snode_update            (gNode          *snode,
                         xbool          update_children,
                         xbool          parent_chagned)
{
    struct SceneNodePrivate *priv;
    priv = SCENE_NODE_PRIVATE (snode);

    G_NODE_CLASS(g_scene_node_parent_class)->update (snode, update_children, parent_chagned);
    if (priv->bounding_dirty) {
        priv->bounding_dirty = FALSE;
        G_SCENE_NODE_GET_CLASS (snode)->bounding (G_SCENE_NODE (snode));
    }
}
static xbool
node_collect(gSceneNode     *snode, xptr param[2])
{
    g_scene_node_collect(snode, param[0], param[1]);
    return TRUE;
}
static void
snode_collect           (gSceneNode     *snode,
                         gRenderQueue   *queue,
                         gVisibleBounds *bounds)
{
    struct SceneNodePrivate *priv;
    gMovableContext context={bounds->camera, queue, (gNode*)snode};

    if (!g_camera_see (bounds->camera, g_scene_node_wbbox(snode)))
        return;

    priv = SCENE_NODE_PRIVATE (snode);
    x_hash_table_foreach_key (priv->movables, g_movable_enqueue, &context);

    if (bounds->include_children) {
        xptr param[2] = { queue, bounds};
        g_node_foreach (G_NODE (snode), node_collect, param);
    }
}
static void
notify_bounding_diry    (gNode          *snode)
{
    struct SceneNodePrivate *priv;
    priv = SCENE_NODE_PRIVATE (snode);
    if (!priv->bounding_dirty) {
        priv->bounding_dirty = TRUE;
        if (snode->parent)
            notify_bounding_diry (snode->parent);
    }
}
static void
snode_attached          (gNode          *node,
                         gNode          *parent)
{
    gSceneNode *snode, *sparent;
    x_return_if_fail (G_IS_SCENE_NODE (parent));

    snode = (gSceneNode*)node;
    sparent = (gSceneNode*)parent;
    snode->scene = sparent->scene;
}
static void
g_scene_node_class_init (gSceneNodeClass*klass)
{
    xParam          *param;
    gNodeClass      *nclass;
    xObjectClass    *oclass;

    x_class_set_private(klass, sizeof(struct SceneNodePrivate));
    klass->bounding = snode_bounding;
    klass->collect = snode_collect;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize = snode_finalize;

    nclass = G_NODE_CLASS (klass);
    nclass->update = snode_update;
    nclass->changed = notify_bounding_diry;
    nclass->attached = snode_attached;

    param = x_param_bool_new ("wbbox-visible",
                              "World bounding box visible",
                              "Whether the world bounding box is visible",
                              FALSE,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_WBBOX_VISIBLE, param);
}
void
g_scene_node_track      (gSceneNode     *snode,
                         gNode          *target,
                         const gvec     offset,
                         const gvec     updir)
{
    struct SceneNodePrivate *priv;

    x_return_if_fail (G_IS_SCENE_NODE (snode));
    priv = SCENE_NODE_PRIVATE (snode);

    priv->tracking_target = target;
    priv->tracking_offset = offset;
    priv->tracking_updir  = updir;
}

gaabb*
g_scene_node_wbbox      (gSceneNode     *snode)
{
    struct SceneNodePrivate *priv;

    x_return_val_if_fail (G_IS_SCENE_NODE (snode), NULL);
    priv = SCENE_NODE_PRIVATE (snode);

    if (priv->bounding_dirty)
        G_SCENE_NODE_GET_CLASS (snode)->bounding (snode);

    return &priv->world_aabb;
}
gaabb*
g_movable_wbbox         (gMovable       *movable,
                         gSceneNode     *snode)
{
    struct SceneNodePrivate *priv;
    struct MovableStub *stub;

    x_return_val_if_fail (G_IS_MOVABLE (movable), NULL);
    x_return_val_if_fail (G_IS_SCENE_NODE (snode), NULL);
    priv = SCENE_NODE_PRIVATE (snode);

    stub = x_hash_table_lookup(priv->movables, movable);
    x_return_val_if_fail (stub != NULL, NULL);

    if (priv->bounding_dirty)
        G_SCENE_NODE_GET_CLASS (snode)->bounding (snode);

    return &stub->wbbox;
}

void
g_scene_node_attach     (gSceneNode     *snode,
                         xptr           movable)
{
    struct SceneNodePrivate *priv;

    x_return_if_fail (G_IS_SCENE_NODE (snode));
    x_return_if_fail (G_IS_MOVABLE (movable));
    priv = SCENE_NODE_PRIVATE (snode);

    if (!x_hash_table_lookup (priv->movables, movable)) {
        x_object_sink (movable);
        x_hash_table_insert (priv->movables, movable, x_slice_new(struct MovableStub));
        x_signal_emit_by_name(movable, "attached", snode);
        priv->bounding_dirty = TRUE;
        x_signal_connect_swapped (movable, "resize",
                                  notify_bounding_diry, snode);
    }
}
void
g_scene_node_detach     (gSceneNode     *snode,
                         xptr           movable)
{
    struct SceneNodePrivate *priv;

    x_return_if_fail (G_IS_SCENE_NODE (snode));
    x_return_if_fail (G_IS_MOVABLE (movable));
    priv = SCENE_NODE_PRIVATE (snode);

    if (x_hash_table_steal (priv->movables, movable)) {
        x_signal_disconnect_func (movable, notify_bounding_diry, snode);
        x_signal_emit_by_name(movable, "detach", snode);
        x_object_unref (movable);
        priv->bounding_dirty = TRUE;
        g_node_dirty (G_NODE (snode), FALSE, FALSE);
    }
}

void
g_scene_node_collect    (gSceneNode     *snode,
                         gRenderQueue   *queue,
                         gVisibleBounds *bounds)
{
    gSceneNodeClass *klass;
    struct SceneNodePrivate *priv;

    x_return_if_fail (G_IS_SCENE_NODE (snode));
    priv = SCENE_NODE_PRIVATE (snode);

    klass = G_SCENE_NODE_GET_CLASS (snode);
    klass->collect (snode, queue, bounds);
    if (priv->boundingbox) {
        gMovableContext context={bounds->camera,queue, NULL};
        g_movable_enqueue ((gMovable*)priv->boundingbox, &context);
    }
}

void
g_scene_node_foreach    (gSceneNode     *snode,
                         xCallback      func,
                         xptr           user_data)
{
    struct SceneNodePrivate *priv;

    x_return_if_fail (G_IS_SCENE_NODE (snode));
    priv = SCENE_NODE_PRIVATE (snode);

    x_hash_table_foreach_key (priv->movables, func, user_data);
}


