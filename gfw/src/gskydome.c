#include "config.h"
#include "gscene.h"
#include "gmaterial.h"
#include "gscenenode.h"
#include "gentity.h"
#include "gmesh.h"
#include "grenderqueue.h"
static void
fill_mesh_param (xBoxFace bf,
                 greal curvature,
                 greal tiling,
                 greal distance,
                 gquat orient,
                 xint segments,
                 xint keep_segments, 
                 gMeshBuildParam *param)
{
    switch(bf) {
    case X_BOX_FRONT:
        param->rotate = g_quat_around (G_VEC_Y, G_PI);
        param->offset = g_vec(0,0,distance,0);
        break;
    case X_BOX_BACK:
        param->rotate = G_QUAT_EYE;
        param->offset = g_vec(0,0,-distance,0);
        break;
    case X_BOX_LEFT:
        param->rotate = g_quat_around (G_VEC_Y, -G_PI_2);
        param->offset = g_vec(distance,0,0,0);
        break;
    case X_BOX_RIGHT:
        param->rotate = g_quat_around (G_VEC_Y, G_PI_2);
        param->offset = g_vec(-distance,0,0,0);
        break;
    case X_BOX_UP:
        param->rotate = g_quat_around (G_VEC_X, G_PI_2);
        param->offset = g_vec(0, distance,0,0);
        break;
    case X_BOX_DOWN:
        break;
    }
    param->rotate = g_quat_mul (param->rotate, orient);
    param->type = G_MESH_BUILD_ILLUSION_PLANE;
    param->width=distance*2;
    param->height=param->width;
    param->curvature = curvature;
    param->x_segments = segments;
    param->y_segments = segments;
    param->keep_y_segments = keep_segments;
    param->normals = FALSE;
    param->n_texcoords = 1;
    param->u_tile = tiling;
    param->v_tile = tiling;
}

gSceneNode*
g_scene_new_sky_dome    (gScene         *scene,
                         xcstr          material,
                         greal          distance,
                         gquat          orient,
                         gRenderGroup   group,
                         gSkyDomeParam  *param)
{
    gSceneNode *node;
    greal curvature = 10;
    greal tiling=8;
    xint segments=16, keep_segments=16;
    xint i;

    x_return_val_if_fail (G_IS_SCENE (scene), NULL);
    x_return_val_if_fail (material!=NULL, NULL);
    x_return_val_if_fail (distance >0, NULL);

    if (param) {
        curvature = param->curvature;
        tiling = param->tiling;
        segments = param->segments;
        keep_segments = param->keep_segments;
    }
    if (!group)
        group = G_RENDER_SKIES_EARLY;

    node = g_node_new_child (g_scene_root_node (scene), NULL);

    for (i=0; i<5; ++i) {
        gEntity* entity;
        gMesh *mesh;
        xstr name;

        name = x_strdup_printf ("sky_dome_%d", i);
        mesh = g_mesh_attach (name, NULL);
        if (!mesh) {
            gMeshBuildParam param;
            fill_mesh_param ((xBoxFace)i,
                             curvature, tiling, distance,
                             orient, segments, 
                             i!=X_BOX_UP ? keep_segments : segments,
                             &param);
            mesh = g_mesh_build(&param,
                                "name", name,
                                "group", "",
                                NULL); 
        }
        entity = x_object_new (G_TYPE_ENTITY,
                               "material", material,
                               "mesh", name,
                               "group", group,
                               "query-flags", G_QUERY_SKY,
                               NULL);
        g_mesh_detach (mesh, NULL);
        g_scene_node_attach (node, G_MOVABLE(entity));
        x_free (name);
    }
    return node;
}
