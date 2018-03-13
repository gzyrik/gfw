#include "config.h"
#include "gscene.h"
#include "gmanual.h"
#include "gmaterial.h"
#include "grender.h"
#include "gtexture.h"
#include "gscenenode.h"
#include "grenderqueue.h"
#include "gfw-prv.h"
static void
fill_2d_sky_box         (gManual        *skybox,
                         xcstr          material,
                         xBoxFace       i,
                         gvec           mid,
                         gvec           up,
                         gvec           right)
{
    gvec pos;
    g_manual_begin (skybox, material, G_RENDER_TRIANGLE_LIST);

    pos = g_vec_sub (g_vec_add (mid, up), right);
    g_manual_position (skybox, pos);
    g_manual_texcoord2 (skybox, 0, 0);

    pos = g_vec_sub (g_vec_sub (mid, up), right);
    g_manual_position (skybox, pos);
    g_manual_texcoord2 (skybox, 0, 1);

    pos = g_vec_add (g_vec_sub (mid, up), right);
    g_manual_position (skybox, pos);
    g_manual_texcoord2 (skybox, 1, 1);

    pos = g_vec_add (g_vec_add (mid, up), right);
    g_manual_position (skybox, pos);
    g_manual_texcoord2 (skybox, 1, 0);

    g_manual_quad (skybox, 0, 1, 2, 3);
    g_manual_end (skybox);
}

static void
fill_3d_sky_box         (gManual        *skybox,
                         xBoxFace       i,
                         gvec           mid,
                         gvec           up,
                         gvec           right)
{
    gvec pos, tex;
    const gvec cvt = g_vec (1,1,-1,0);
    pos = g_vec_sub (g_vec_add (mid, up), right);
    tex = g_vec_mul (g_vec3_norm (pos), cvt);
    g_manual_position (skybox, pos);
    g_manual_texcoord3 (skybox, tex);

    pos = g_vec_sub (g_vec_sub (mid, up), right);
    tex = g_vec_mul (g_vec3_norm (pos), cvt);
    g_manual_position (skybox, pos);
    g_manual_texcoord3 (skybox, tex);

    pos = g_vec_add (g_vec_sub (mid, up), right);
    tex = g_vec_mul (g_vec3_norm (pos), cvt);
    g_manual_position (skybox, pos);
    g_manual_texcoord3 (skybox, tex);

    pos = g_vec_add (g_vec_add (mid, up), right);
    tex = g_vec_mul (g_vec3_norm (pos), cvt);
    g_manual_position (skybox, pos);
    g_manual_texcoord3 (skybox, tex);

    i *= 4;
    g_manual_quad (skybox, i, i+1, i+2, i+3);
}

static inline void
get_box_plane           (xBoxFace       i,
                         gvec           *mid,
                         gvec           *up,
                         gvec           *right,
                         greal          d)
{
    switch(i) {
    case X_BOX_FRONT:
        *mid  = g_vec(0,0,-d,0);
        *up   = g_vec(0,d,0,0);
        *right= g_vec(d,0,0,0);
        break;
    case X_BOX_BACK:
        *mid  = g_vec(0,0,d,0);
        *up   = g_vec(0,d,0,0);
        *right= g_vec(-d,0,0,0);
        break;
    case X_BOX_LEFT:
        *mid  = g_vec(d,0,0,0);
        *up   = g_vec(0,d,0,0);
        *right= g_vec(0,0,d,0);
        break;
    case X_BOX_RIGHT:
        *mid  = g_vec(-d,0,0,0);
        *up   = g_vec(0,d,0,0);
        *right= g_vec(0,0,-d,0);
        break;
    case X_BOX_UP:
        *mid  = g_vec(0,d,0,0);
        *up   = g_vec(0,0,d,0);
        *right= g_vec(d,0,0,0);
        break;
    case X_BOX_DOWN:
        *mid  = g_vec(0,-d,0,0);
        *up   = g_vec(0,0,-d,0);
        *right= g_vec(d,0,0,0);
        break;
    }
}
gSceneNode*
g_scene_new_sky_box     (gScene         *scene,
                         xcstr          material_name,
                         greal          distance,
                         gquat          orient,
                         gRenderGroup   group)
{
    xbool t3d;
    xint i;
    gvec mid, up,right;
    gManual *skybox;
    gSceneNode *node;
    gMaterial *material;

    x_return_val_if_fail (G_IS_SCENE (scene), NULL);
    x_return_val_if_fail (material_name != NULL, NULL);
    x_return_val_if_fail (distance >0, NULL);

    if (g_real_aeq (distance, 0))
        distance = 5000;

    if (g_vec_aeq0_s (orient))
        orient = G_VEC_W;

    if (!group)
        group = G_RENDER_SKIES_EARLY;

    material = g_material_attach(material_name, NULL);
    x_return_val_if_fail (G_IS_MATERIAL(material), NULL);

    t3d = g_material_is_3d (material);

    node = g_node_new_child (g_scene_root_node (scene), NULL);
    skybox = x_object_new (G_TYPE_MANUAL,
                                   "group", group,
                                   "query-flags", G_QUERY_SKY,
                                   NULL);
    g_scene_node_attach (node, skybox);

    if (t3d)
        g_manual_begin (skybox, material_name, G_RENDER_TRIANGLE_LIST);
    for(i=0;i<6;++i) {
        get_box_plane (i,&mid,&up,&right,distance);
        mid = g_vec3_rotate (mid,orient);
        up = g_vec3_rotate (up, orient);
        right = g_vec3_rotate (right, orient);
        if(t3d)
            fill_3d_sky_box(skybox, i,mid,up,right);
        else
            fill_2d_sky_box(skybox, material_name,i,mid,up,right);
    }
    if(t3d)
        g_manual_end (skybox);

    g_material_detach(material, NULL);
    return node;
}
