#include "gfw-octree.h"
X_DEFINE_DYNAMIC_TYPE (octScene, oct_scene, G_TYPE_SCENE);
xType
_oct_scene_register     (xTypeModule    *module)
{
    return oct_scene_register (module);
}

enum {
    PROP_0,
    PROP_TREE_DEPTH,
    PROP_WORLD_SIZE,
    PROP_OCTREE_VISIBLE,
    N_PROPERTIES
};
static void
oscene_set_property     (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    octScene *oscene = (octScene*)object;

    switch(property_id) {
    case PROP_TREE_DEPTH:
        octree_set_depth (oscene->octree,
                          x_value_get_uint (value));
        break;
    case PROP_WORLD_SIZE:
        octree_set_size (oscene->octree,
                         g_value_get_vec (value));
        break;
    case PROP_OCTREE_VISIBLE:
        oscene->octree_visible = x_value_get_bool (value);
        break;
    }
}
static void
oscene_get_property     (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    octScene *oscene = (octScene*)object;

    switch(property_id) {
    case PROP_TREE_DEPTH:
        x_value_set_uint (value, oscene->octree->depth);
        break;
    case PROP_WORLD_SIZE:
        g_value_set_vec (value, oscene->octree->aabb.maximum);
        break;
    case PROP_OCTREE_VISIBLE:
        x_value_set_bool (value, oscene->octree_visible);
    }
}
void
oct_scene_refresh       (octScene    *scene,
                         octNode     *node)
{
    gaabb *box;

    x_return_if_fail (scene->octree != NULL);

    box = g_scene_node_wbbox (G_SCENE_NODE (node));
    x_return_if_fail (box->extent == 1);

    if (!node->octree) {
        octree_attach (scene->octree, node);
    }
    else if (!g_aabb_inside (box, &node->octree->aabb)) {
        octree_detach (node->octree, node);
        octree_attach (scene->octree, node);
    }
    else if (node->octree->depth > 0 &&
             octree_twice_size (node->octree, box)) {
        octree_detach (node->octree, node);
        octree_attach (node->octree, node);
    }
}

static void
oscene_walk             (octScene       *oscene,
                         gVisibleBounds *bounds,
                         gRenderQueue   *queue,
                         struct octree  *octree,
                         xbool          inside) 
{
    xsize i;
    xbool bbox_show;
    gSceneNode *snode;
    gBoxSide on_side;

    if (octree->n_nodes == 0)
        return;

    if(inside)
        on_side = G_BOX_INSIDE;
    else if(octree == oscene->octree)
        on_side = G_BOX_PARTIAL;
    else
        on_side = g_frustum_side_box ((gFrustum*)bounds->camera,
                                      &octree->aabb);

    if ( on_side < G_BOX_PARTIAL )
        return;

    bbox_show = FALSE;
    inside = (on_side == G_BOX_INSIDE);
    for (i=0;i<octree->nodes->len;++i) {
        snode = x_ptr_array_index(octree->nodes, gSceneNode, i);
        if (!inside)
            on_side = g_frustum_side_box ((gFrustum*)bounds->camera,
                                            g_scene_node_wbbox (snode));
        else
            on_side = G_BOX_INSIDE;
        if(on_side >= G_BOX_PARTIAL) {
            bbox_show = TRUE;
            g_scene_node_collect (snode, queue, bounds);
        }
    }
    if (bbox_show && oscene->octree_visible) {
        gMovableContext context={bounds->camera,queue,NULL};
        g_movable_enqueue ((gMovable*)octree_boundingbox(octree), &context);
    }
    /* ตÝน้ */
    for (i=0;i<8;++i) {
        if (octree->children[i])
            oscene_walk (oscene, bounds, queue, octree->children[i], inside);
    }
}
static void
oscene_collect          (gScene         *scene,
                         gRenderQueue   *queue,
                         gVisibleBounds *bounds)
{
    octScene *oscene = (octScene*)scene;
    oscene_walk (oscene, bounds, queue, oscene->octree, TRUE);
}
static void
oct_scene_init          (octScene *oscene)
{
    oscene->octree = octree_new ();
}
static void
oscene_finalize         (xObject        *object)
{
    octScene *oscene = (octScene*)object;
    octree_delete (oscene->octree);
}
static void
oct_scene_class_finalize(octSceneClass *klass)
{

}
static void 
oct_scene_class_init    (octSceneClass *klass)
{
    xParam          *param;
    gSceneClass     *sclass;
    xObjectClass    *oclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = oscene_get_property;
    oclass->set_property = oscene_set_property;
    oclass->finalize = oscene_finalize;

    sclass = G_SCENE_CLASS (klass);
    sclass->node_type   = x_type_from_mime (G_TYPE_SCENE_NODE, "octNode");
    sclass->collect     = oscene_collect;

    param = x_param_uint_new ("tree-depth",
                              "Tree depth",
                              "The octree depth of the scene",
                              0, X_MAXUINT32, 9,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_TREE_DEPTH, param);

    param = g_param_vec3_new ("world-size",
                              "World size",
                              "The world half size of scene",
                              G_VEC_0, G_VEC_MAX, g_vec(1000,1000,1000,0),
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_WORLD_SIZE, param);

    param = x_param_bool_new ("octree-visible",
                              "Octree visible",
                              "Whether the octree bounding box is visible",
                              FALSE,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_OCTREE_VISIBLE, param);
}
