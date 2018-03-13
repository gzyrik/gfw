#ifndef __G_SCENE_H__
#define __G_SCENE_H__
#include "gtype.h"
#include "grenderqueue.h"
X_BEGIN_DECLS

#define G_TYPE_SCENE              (g_scene_type())
#define G_SCENE(object)           (X_INSTANCE_CAST((object), G_TYPE_SCENE, gScene))
#define G_SCENE_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_SCENE, gSceneClass))
#define G_IS_SCENE(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_SCENE))
#define G_SCENE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_SCENE, gSceneClass))


gScene*     g_scene_new             (xcstr          type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

xptr        g_scene_root_node       (gScene         *scene);

gCamera*    g_scene_add_camera      (gScene         *scene,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

void        g_scene_render          (gScene         *scene,
                                     gViewport      *viewport,
                                     gCamera        *camera,
                                     xbool          overlay);

gSceneNode* g_scene_new_sky_box     (gScene         *scene,
                                     xcstr          material,
                                     greal          distance,
                                     gquat          orient,
                                     gRenderGroup   group);

gSceneNode* g_scene_new_sky_dome    (gScene         *scene,
                                     xcstr          material,
                                     greal          distance,
                                     gquat          orient,
                                     gRenderGroup   group,
                                     gSkyDomeParam  *param);

gQuery*     g_scene_new_query       (gScene         *scene,
                                     xcstr          type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;


enum {
    SHADOWTYPE_STENCIL_ADDITIVE,
    SHADOWDETAILTYPE_ADDITIVE,
    SHADOWDETAILTYPE_TEXTURE,
};
enum {
    IRS_NONE,
    IRS_RENDER_TO_TEXTURE,
    IRS_RENDER_RECEIVER_PASS,
};
enum {
    OM_PASS_GROUP       = 1,
    OM_SORT_DESCENDING  = 2,
    OM_SORT_ASCENDING    =4
};

/** 可见物体集合的边界信息 */
struct _gVisibleBounds
{
    gCamera             *camera;
    /** 可见物体集合边界盒 */
    gaabb               aabb;
    /** 可见物体的离摄像机的最近距离 */
    gvec                minimum;
    gvec                maximum;
    xbool               include_children : 1;
    xbool               include_nodes    : 1;
    xbool               only_shadowcaster: 1;
};

struct _gScene
{
    xObject             parent;
    xptr                priv;
};

struct _gSceneClass
{
    xObjectClass        parent;

    /* object */
    xType               queue_type;
    xType               node_type;
    xType               camera_type;
    xType               render_type;

    /* update */
    void (*update) (gScene *scene, gCamera *camera);

    /* collect renderable into queue */
    void (*collect) (gScene *scene, gRenderQueue *queue, gVisibleBounds *bounds);
};
struct _gRenderContext
{
    gNode               *node;
    xptr                user_data;
    gRenderable         *renderable;
};

struct _gSkyDomeParam
{
    greal               curvature;
    greal               tiling;
    xint                segments;
    xint                keep_segments;
};

xType       g_scene_type            (void);


X_END_DECLS
#endif /* __G_TEXTURE_H__ */
