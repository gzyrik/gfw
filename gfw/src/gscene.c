#include "config.h"
#include "gscene.h"
#include "grender.h"
#include "grenderqueue.h"
#include "gscenenode.h"
#include "gmovable.h"
#include "gcamera.h"
#include "gtarget.h"
#include "gfw-prv.h"
#include "gmaterial.h"
#include "grenderable.h"
#include "gscenerender.h"
#include "gshader.h"
#include "gpass.h"
#include "gquery.h"
struct ScenePrivate
{
    xstr                name;

    xPtrArray           *cameras;
    gSceneNode          *root_node;
    xPtrArray           *tracking_nodes;
    gRenderQueue        *render_queue;
    gSceneRender        *scene_render;
    gVisibleBounds      *visible_bounds;

    xint                find_visible : 1;
    xint                transparent_shadow_caster:1;
    xsize               frame_count;
};
X_DEFINE_TYPE (gScene, g_scene, X_TYPE_OBJECT);
#define SCENE_PRIVATE(o)  ((struct ScenePrivate*) (G_SCENE (o)->priv))
static xSList   *_scene_stack;

static void
init_shadow_materials   (gScene         *scene)
{

}
static void
apply_scene_animations    ()
{
}
static void
scene_update            (gScene         *scene,
                         gCamera        *camera)
{
    struct ScenePrivate *priv;
    priv = SCENE_PRIVATE (scene);

    g_node_update ((gNode*)priv->root_node, TRUE, FALSE);
}

static void
scene_collect           (gScene         *scene,
                         gRenderQueue   *queue,
                         gVisibleBounds *bounds)
{
    struct ScenePrivate *priv;
    priv = SCENE_PRIVATE (scene);

    g_scene_node_collect (priv->root_node, queue, bounds);
}

void
g_scene_render          (gScene         *scene,
                         gViewport      *viewport,
                         gCamera        *camera,
                         xbool          overlay)
{
    gSceneClass *klass;
    struct ScenePrivate *priv;

    x_return_if_fail (G_IS_SCENE (scene));
    priv = SCENE_PRIVATE (scene);
    x_return_if_fail (priv->root_node != NULL);

    klass = G_SCENE_GET_CLASS (scene);
    if (!priv->render_queue) {
        priv->render_queue = x_object_new (klass->queue_type, NULL);
        x_object_sink (priv->render_queue);
    }
    if (!priv->scene_render) {
        priv->scene_render = x_object_new (klass->render_type, NULL);
        x_object_sink (priv->scene_render);
    }
    if (!camera) camera = viewport->camera;

    _scene_stack = x_slist_prepend (_scene_stack, scene);

    priv->visible_bounds->camera = camera;

    _g_render_fhook_update ();
    if (priv->frame_count != g_render_ftimer()->frame_count) {
        /* 更新所有场景动画,保证只更新一帧 */
        apply_scene_animations (scene);
        priv->frame_count = g_render_ftimer()->frame_count;
    }
    if (priv->find_visible &&
        priv->scene_render->illumination_stage != IRS_RENDER_TO_TEXTURE ) {

    }
    _g_process_queued_nodes();

    klass->update (scene, camera);

    g_camera_track (camera);

    g_render_queue_clear(priv->render_queue);
    klass->collect (scene, priv->render_queue,priv->visible_bounds);

    g_scene_render_set_viewport(priv->scene_render, viewport);
    g_scene_render_set_camera(priv->scene_render, camera, FALSE);
    g_render_begin ();
    g_render_clear (viewport);
    g_render_queue_foreach (priv->render_queue, priv->scene_render);
    g_render_end ();

    priv->visible_bounds->camera = NULL;
    _scene_stack = x_slist_remove (_scene_stack, scene);
}

static void
g_scene_init            (gScene         *scene)
{
    struct ScenePrivate *priv;
    priv = x_instance_private(scene, G_TYPE_SCENE);
    scene->priv = priv;

    priv->visible_bounds = x_new0 (gVisibleBounds, 1);
    priv->visible_bounds->include_children = TRUE;
    priv->visible_bounds->only_shadowcaster = FALSE;

    priv->scene_render = x_object_new(G_TYPE_SCENE_RENDER, NULL);
    priv->cameras = x_ptr_array_new_full (4, x_object_unref);
}
static void
g_scene_class_init      (gSceneClass    *klass)
{
    x_class_set_private(klass, sizeof(struct ScenePrivate));

    klass->node_type   = G_TYPE_SCENE_NODE;
    klass->queue_type  = G_TYPE_RENDER_QUEUE;
    klass->camera_type = G_TYPE_CAMERA;
    klass->render_type = G_TYPE_SCENE_RENDER;

    klass->update          = scene_update;
    klass->collect         = scene_collect;
}

gScene*
g_scene_new             (xcstr          type,
                         xcstr          first_property,
                         ...)
{
    gScene  *scene;
    va_list argv;
    xType xtype;

    x_return_val_if_fail (type != NULL, NULL);
    xtype = x_type_from_mime (G_TYPE_SCENE, type);
    x_return_val_if_fail (xtype != X_TYPE_INVALID, NULL);

    va_start(argv, first_property);
    scene = x_object_new_valist (xtype, first_property, argv);
    va_end(argv);

    return scene;
}
xptr
g_scene_root_node       (gScene         *scene)
{
    struct ScenePrivate *priv;

    x_return_val_if_fail (G_IS_SCENE (scene), NULL);
    priv = SCENE_PRIVATE (scene);

    if (!priv->root_node) {
        gSceneClass *klass;
        klass = G_SCENE_GET_CLASS (scene);
        priv->root_node = x_object_new (klass->node_type, NULL);
        priv->root_node->scene = scene;
        x_object_sink (priv->root_node);
    }
    return priv->root_node;
}

gCamera*
g_scene_add_camera      (gScene         *scene,
                         xcstr          first_property,
                         ...)
{
    gSceneClass *klass;
    gCamera* camera;
    va_list argv;
    struct ScenePrivate *priv;

    x_return_val_if_fail (G_IS_SCENE (scene), NULL);
    priv = SCENE_PRIVATE (scene);


    klass = G_SCENE_GET_CLASS (scene);
    va_start(argv, first_property);
    camera = x_object_new_valist (klass->camera_type, first_property, argv);
    va_end(argv);
    camera->scene = scene;
    x_ptr_array_append1 (priv->cameras, camera);

    return camera;
}
gQuery*
g_scene_new_query       (gScene         *scene,
                         xcstr          type,
                         xcstr          first_property,
                         ...)
{
    xType xtype;
    gQuery* query;
    va_list argv;

    x_return_val_if_fail (G_IS_SCENE (scene), NULL);

    xtype = x_type_from_mime (G_TYPE_QUERY, type);
    x_return_val_if_fail (xtype != X_TYPE_INVALID, NULL);

    va_start(argv, first_property);
    query = x_object_new_valist (xtype, first_property, argv);
    va_end(argv);
    query->scene = scene;

    return query;
}
