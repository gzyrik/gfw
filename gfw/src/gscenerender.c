#include "config.h"
#include "gscenerender.h"
#include "gshader.h"
#include "grenderable.h"
#include "grender.h"
#include "gcamera.h"
#include "gscene.h"
#include "gnode.h"
#include "gpass.h"
#include <string.h>

X_DEFINE_TYPE (gSceneRender, g_scene_render, X_TYPE_OBJECT);
#define SCENE_RENDER_PRIVATE(o) ((struct SceneRenderPrivate*) (G_SCENE_RENDER (o)->priv))

struct SceneRenderPrivate
{
    gFrameTimer     frame_timer;
    gTarget         *target;

    gPass           *pass;
    xsize           pass_index;
    xuint           shader_scope;

    gRenderable     *renderable;
    gNode           *node;
    xptr            user_data;

    xbool           camera_relative_rendering;
    gvec            camera_relative_pos;

    gmat4           model_matrix[256];
    const gmat4     *ptr_model_matrix;
    xsize           n_model_matrix;
    xbool           cached_model_matrix;

    gmat4           node_matrix;
    xbool           cached_node_matrix;

    gmat4           nodeviewproj_matrix;
    xbool           cached_nodeviewproj_matrix;

    gmat4           world_matrix[256];
    const gmat4     *ptr_world_matrix;
    xsize           n_world_matrix;
    xbool           cached_world_matrix;
    gmat4           inverse_world_matrix;
    xbool           cached_inverse_world_matrix;
    gmat4           transpose_world_matrix;
    xbool           cached_transpose_world_matrix;
    gmat4           inverse_transpose_world_matrix;
    xbool           cached_inverse_transpose_world_matrix;

    gmat4           view_matrix;
    xbool           cached_view_matrix;
    gmat4           inverse_view_matrix;
    xbool           cached_inverse_view_matrix;
    gmat4           transpose_view_matrix;
    xbool           cached_transpose_view_matrix;
    gmat4           inverse_transpose_view_matrix;
    xbool           cached_inverse_transpose_view_matrix;

    gmat4           proj_matrix;
    xbool           cached_proj_matrix;
    gmat4           inverse_proj_matrix;
    xbool           cached_inverse_proj_matrix;
    gmat4           transpose_proj_matrix;
    xbool           cached_transpose_proj_matrix;
    gmat4           inverse_transpose_proj_matrix;
    xbool           cached_inverse_transpose_proj_matrix;

    gmat4           viewproj_matrix;
    xbool           cached_viewproj_matrix;
    gmat4           inverse_viewproj_matrix;
    xbool           cached_inverse_viewproj_matrix;
    gmat4           transpose_viewproj_matrix;
    xbool           cached_transpose_viewproj_matrix;
    gmat4           inverse_transpose_viewproj_matrix;
    xbool           cached_inverse_transpose_viewproj_matrix;

    gmat4           worldview_matrix;
    xbool           cached_worldview_matrix;
    gmat4           inverse_worldview_matrix;
    xbool           cached_inverse_worldview_matrix;
    gmat4           transpose_worldview_matrix;
    xbool           cached_transpose_worldview_matrix;
    gmat4           inverse_transpose_worldview_matrix;
    xbool           cached_inverse_transpose_worldview_matrix;


    gmat4           worldviewproj_matrix;
    xbool           cached_worldviewproj_matrix;
    gmat4           inverse_worldviewproj_matrix;
    xbool           cached_inverse_worldviewproj_matrix;
    gmat4           transpose_worldviewproj_matrix;
    xbool           cached_transpose_worldviewproj_matrix;
    gmat4           inverse_transpose_worldviewproj_matrix;
    xbool           cached_inverse_transpose_worldviewproj_matrix;

    xbool           cached_camera_pos_object_space;
    xbool           cached_camera_pos;

};
static void
g_scene_render_init (gSceneRender  *render)
{
    struct SceneRenderPrivate *priv;
    priv = x_instance_private(render, G_TYPE_SCENE_RENDER);
    render->priv = priv;

    priv->frame_timer.time_factor = 1;
    render->organisation = OM_PASS_GROUP;
}
void
g_scene_render_set_viewport (gSceneRender       *render,
                             gViewport          *viewport)
{
    greal ftime;
    gFrameTimer frame_timer;
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);


    ftime = g_render_ftimer()->frame_time;
    frame_timer = priv->frame_timer;
    if(frame_timer.ftime_fixed)
        frame_timer.time_factor =  frame_timer.frame_time/ ftime;
    else 
        frame_timer.frame_time =  frame_timer.time_factor * ftime;
    frame_timer.elapsed_time += frame_timer.frame_time;

    memset (priv, 0, sizeof (struct SceneRenderPrivate));
    priv->frame_timer = frame_timer;
    render->viewport = viewport;
}
void
g_scene_render_set_camera   (gSceneRender       *render,
                             gCamera            *camera,
                             xbool              relative)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    render->camera = camera;
    priv->camera_relative_rendering = relative;
    priv->camera_relative_pos = g_frustum_xform (G_FRUSTUM (camera))->offset;
    priv->cached_view_matrix = FALSE;
    priv->cached_proj_matrix = FALSE;
    priv->cached_worldview_matrix = FALSE;
    priv->cached_viewproj_matrix = FALSE;
    priv->cached_worldviewproj_matrix = FALSE;
    priv->cached_camera_pos_object_space = FALSE;
    priv->cached_camera_pos = FALSE;
}
void
g_scene_render_set_renderable   (gSceneRender       *render,
                                 gRenderable        *renderable,
                                 gNode              *node,
                                 xptr               user_data)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    priv->renderable = renderable;
    priv->node  = node;
    priv->user_data = user_data;

    priv->cached_node_matrix = FALSE;
    priv->cached_nodeviewproj_matrix = FALSE;
    priv->cached_world_matrix = FALSE;
    priv->cached_view_matrix = FALSE;
    priv->cached_proj_matrix = FALSE;
    priv->cached_worldview_matrix = FALSE;
    priv->cached_viewproj_matrix = FALSE;
    priv->cached_worldviewproj_matrix = FALSE;
    priv->cached_inverse_world_matrix = FALSE;
    priv->cached_camera_pos_object_space = FALSE;
}

void
g_scene_render_set_pass (gSceneRender       *render,
                         gPass              *pass,
                         xsize              pass_index)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    priv->pass = pass;
    priv->pass_index = pass_index;
    priv->shader_scope |= G_UNIFORM_GLOBAL;
}
void
g_scene_render_set_wmatrix  (gSceneRender       *render,
                             const gmat4        *matrix,
                             xsize              count)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    priv->ptr_world_matrix = matrix;
    priv->n_world_matrix = count;
    priv->cached_world_matrix = TRUE;
}
void
g_scene_render_set_ambient  (gSceneRender       *render,
                             gcolor             color)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    render->ambient_color = color;
}
X_INLINE_FUNC const gmat4*
model_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_model_matrix) {
        priv->n_model_matrix = 
            g_renderable_mmatrix (priv->renderable,
                                  priv->model_matrix,
                                  priv->user_data);

        priv->ptr_model_matrix = priv->model_matrix;
        priv->cached_model_matrix = TRUE;
    }
    return priv->ptr_model_matrix;
}
X_INLINE_FUNC const gmat4*
node_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_node_matrix) {
        priv->node_matrix = *g_node_mat4 (priv->node);
        priv->cached_node_matrix = TRUE;
    }
    return &priv->node_matrix;
}

X_INLINE_FUNC const gmat4*
world_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_world_matrix) {
        xsize i;

        priv->n_world_matrix = 
            g_renderable_wmatrix (priv->renderable,
                                  priv->world_matrix,
                                  priv->node,
                                  priv->user_data);
        if (priv->camera_relative_rendering) {
            for (i=0; i< priv->n_world_matrix; ++i)
                priv->world_matrix[i].r[3] =
                    g_vec_sub (priv->world_matrix[i].r[3],
                               priv->camera_relative_pos);
        }
        priv->ptr_world_matrix = priv->world_matrix;
        priv->cached_world_matrix = TRUE;
    }
    return priv->ptr_world_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_world_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_world_matrix) {
        priv->inverse_world_matrix = g_mat4_inv (world_matrix (render));
        priv->cached_inverse_world_matrix = TRUE;
    }
    return &priv->inverse_world_matrix;
}
X_INLINE_FUNC const gmat4*
transpose_world_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_transpose_world_matrix) {
        priv->transpose_world_matrix = g_mat4_trans (world_matrix (render));
        priv->cached_transpose_world_matrix = TRUE;
    }
    return &priv->transpose_world_matrix;

}
X_INLINE_FUNC const gmat4*
inverse_transpose_world_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_transpose_world_matrix) {
        priv->inverse_transpose_world_matrix = 
            g_mat4_trans (inverse_world_matrix (render));
        priv->cached_inverse_transpose_world_matrix = TRUE;
    }
    return &priv->inverse_transpose_world_matrix;
}
X_INLINE_FUNC const gmat4*
proj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_proj_matrix) {
        if (!priv->renderable ||
            !g_renderable_pmatrix (priv->renderable,
                                   &priv->proj_matrix,
                                   priv->user_data)) {
            priv->proj_matrix = *g_frustum_pmatrix (G_FRUSTUM (render->camera));
        }
        priv->cached_proj_matrix = TRUE;
    }
    return &priv->proj_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_proj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_proj_matrix) {
        priv->inverse_proj_matrix = g_mat4_inv (proj_matrix (render));
        priv->cached_inverse_proj_matrix = TRUE;
    }
    return &priv->inverse_proj_matrix;
}
X_INLINE_FUNC const gmat4*
transpose_proj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_transpose_proj_matrix) {
        priv->transpose_proj_matrix = g_mat4_trans (proj_matrix (render));
        priv->cached_transpose_proj_matrix = TRUE;
    }
    return &priv->transpose_proj_matrix;
}
X_INLINE_FUNC  const gmat4*
inverse_transpose_proj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_transpose_proj_matrix) {
        priv->inverse_transpose_proj_matrix = g_mat4_inv (transpose_proj_matrix(render));
        priv->cached_inverse_transpose_proj_matrix = TRUE;
    }
    return &priv->inverse_transpose_proj_matrix;
}
X_INLINE_FUNC const gmat4*
view_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_view_matrix) {
        if (!priv->renderable ||
            !g_renderable_vmatrix (priv->renderable,
                                   &priv->view_matrix,
                                   priv->user_data)) {
            priv->view_matrix = *g_frustum_vmatrix (G_FRUSTUM (render->camera));
            if (priv->camera_relative_rendering)
                priv->view_matrix.r[3] = G_VEC_W;
        }
        priv->cached_view_matrix = TRUE;
    }
    return &priv->view_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_view_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_view_matrix) {
        priv->inverse_view_matrix = g_mat4_inv (view_matrix(render));
        priv->cached_inverse_view_matrix = TRUE;
    }
    return &priv->inverse_view_matrix;
}
X_INLINE_FUNC const gmat4*
transpose_view_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_transpose_view_matrix) {
        priv->transpose_view_matrix = g_mat4_trans (view_matrix(render));
        priv->cached_transpose_view_matrix = TRUE;
    }
    return &priv->transpose_view_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_transpose_view_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_transpose_view_matrix) {
        priv->inverse_transpose_view_matrix = g_mat4_inv (transpose_view_matrix(render));
        priv->cached_inverse_transpose_view_matrix = TRUE;
    }
    return &priv->inverse_transpose_view_matrix;
}
X_INLINE_FUNC const gmat4*
viewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_viewproj_matrix) {
        priv->viewproj_matrix = g_mat4_mul(view_matrix (render),
                                           proj_matrix (render));
        priv->cached_viewproj_matrix = TRUE;
    }
    return &priv->viewproj_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_viewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_viewproj_matrix) {
        priv->inverse_viewproj_matrix = g_mat4_inv (viewproj_matrix (render));
        priv->cached_inverse_viewproj_matrix = TRUE;
    }
    return &priv->inverse_viewproj_matrix;
}
X_INLINE_FUNC const gmat4*
transpose_viewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_transpose_viewproj_matrix) {
        priv->transpose_viewproj_matrix = g_mat4_trans (viewproj_matrix (render));
        priv->cached_transpose_viewproj_matrix = TRUE;
    }
    return &priv->transpose_viewproj_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_transpose_viewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_transpose_viewproj_matrix) {
        priv->inverse_transpose_viewproj_matrix = 
            g_mat4_inv (transpose_viewproj_matrix (render));
        priv->cached_inverse_transpose_viewproj_matrix = TRUE;
    }
    return &priv->inverse_transpose_viewproj_matrix;
}
X_INLINE_FUNC const gmat4*
worldview_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_worldview_matrix) {
        priv->worldview_matrix = 
            g_mat4_mul(worldview_matrix (render), view_matrix (render));
        priv->cached_worldview_matrix = TRUE;
    }
    return &priv->worldview_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_worldview_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_worldview_matrix) {
        priv->inverse_worldview_matrix = g_mat4_inv (worldview_matrix (render));
        priv->cached_inverse_worldview_matrix = TRUE;
    }
    return &priv->inverse_worldview_matrix;
}
X_INLINE_FUNC const gmat4*
transpose_worldview_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_transpose_worldview_matrix) {
        priv->transpose_worldview_matrix = g_mat4_trans (worldview_matrix (render));
        priv->cached_transpose_worldview_matrix = TRUE;
    }
    return &priv->transpose_worldview_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_transpose_worldview_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_transpose_worldview_matrix) {
        priv->inverse_transpose_worldview_matrix = 
            g_mat4_inv (transpose_worldview_matrix (render));
        priv->cached_inverse_transpose_worldview_matrix = TRUE;
    }
    return &priv->inverse_transpose_worldview_matrix;
}
X_INLINE_FUNC const gmat4*
nodeviewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_nodeviewproj_matrix) {
        priv->nodeviewproj_matrix = 
            g_mat4_mul(node_matrix (render), viewproj_matrix (render));
        priv->cached_nodeviewproj_matrix = TRUE;
    }
    return &priv->nodeviewproj_matrix;
}
X_INLINE_FUNC const gmat4*
worldviewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_worldviewproj_matrix) {
        priv->worldviewproj_matrix = 
            g_mat4_mul(world_matrix (render), viewproj_matrix (render));
        priv->cached_worldviewproj_matrix = TRUE;
    }
    return &priv->worldviewproj_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_worldviewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_worldviewproj_matrix) {
        priv->inverse_worldviewproj_matrix = g_mat4_inv (worldviewproj_matrix (render));
        priv->cached_inverse_worldviewproj_matrix = TRUE;
    }
    return &priv->inverse_worldviewproj_matrix;
}
X_INLINE_FUNC const gmat4*
transpose_worldviewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_transpose_worldviewproj_matrix) {
        priv->transpose_worldviewproj_matrix = g_mat4_trans (worldviewproj_matrix (render));
        priv->cached_transpose_worldviewproj_matrix = TRUE;
    }
    return &priv->transpose_worldviewproj_matrix;
}
X_INLINE_FUNC const gmat4*
inverse_transpose_worldviewproj_matrix (gSceneRender* render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    if (!priv->cached_inverse_transpose_worldviewproj_matrix) {
        priv->inverse_transpose_worldviewproj_matrix = 
            g_mat4_inv (transpose_worldviewproj_matrix (render));
        priv->cached_inverse_transpose_worldviewproj_matrix = TRUE;
    }
    return &priv->inverse_transpose_worldviewproj_matrix;
}
X_INLINE_FUNC greal
time_0_x (gSceneRender* render, greal extra)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    return g_real_mod (priv->frame_timer.elapsed_time, extra);
}
X_INLINE_FUNC greal
time_0_1 (gSceneRender* render, greal extra)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    return g_real_mod (priv->frame_timer.elapsed_time, extra)/extra;
}

void
g_scene_render_write_uniform    (gSceneRender       *render,
                                 gUniform           *uniform,
                                 xptr               address)
{
    xcptr buf;
    xfloat *fbuf = (xfloat*)address;
    xint *ibuf   = (xint*)address;
    xbyte *cbuf   = (xbyte*)address;

    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);


    switch (uniform->semantic) {
        xsize i;
        const gmat4* mat4;
    case G_UNIFORM_MANUAL:
        g_renderable_uniform (priv->renderable, uniform, address, priv->user_data);
        return;
    case G_UNIFORM_MODEL_AFFINE_ARRAY:
        x_assert (uniform->elem_size == sizeof(gvec)*3);
        mat4 = model_matrix (render);
        for (i=0;i<priv->n_model_matrix;++i,++mat4) {
            gmat4 trans = g_mat4_trans (mat4);
            memcpy (cbuf, &trans, uniform->elem_size);
            cbuf += uniform->elem_size;
        }
        return;
    case G_UNIFORM_NODE_MATRIX:
        buf = node_matrix(render);
        break;
    case G_UNIFORM_NODEVIEWPROJ_MATRIX:
        buf = nodeviewproj_matrix (render);
        break;

    case G_UNIFORM_WORLD_MATRIX:
        buf = world_matrix (render);
        break;
    case G_UNIFORM_INVERSE_WORLD_MATRIX:
        buf = inverse_world_matrix (render);
        break;
    case G_UNIFORM_TRANSPOSE_WORLD_MATRIX:
        buf= transpose_world_matrix (render);
        break;
    case G_UNIFORM_INVERSE_TRANSPOSE_WORLD_MATRIX:
        buf =  inverse_transpose_world_matrix (render);
        break;

    case G_UNIFORM_VIEW_MATRIX:
        buf =  view_matrix (render);
        break;
    case G_UNIFORM_INVERSE_VIEW_MATRIX:
        buf =  inverse_view_matrix (render);
        break;
    case G_UNIFORM_TRANSPOSE_VIEW_MATRIX:
        buf =  transpose_view_matrix (render);
        break;
    case G_UNIFORM_INVERSE_TRANSPOSE_VIEW_MATRIX:
        buf =  inverse_transpose_view_matrix(render);
        break;

    case G_UNIFORM_PROJECTION_MATRIX:
        buf =  proj_matrix (render);
        break;
    case G_UNIFORM_INVERSE_PROJECTION_MATRIX:
        buf =  inverse_proj_matrix (render);
        break;
    case G_UNIFORM_TRANSPOSE_PROJECTION_MATRIX:
        buf =  transpose_proj_matrix (render);
        break;
    case G_UNIFORM_INVERSE_TRANSPOSE_PROJECTION_MATRIX:
        buf =  inverse_transpose_proj_matrix (render);
        break;

    case G_UNIFORM_VIEWPROJ_MATRIX:
        buf =  viewproj_matrix (render);
        break;
    case G_UNIFORM_INVERSE_VIEWPROJ_MATRIX:
        buf =  inverse_viewproj_matrix (render);
        break;
    case G_UNIFORM_TRANSPOSE_VIEWPROJ_MATRIX:
        buf =  transpose_viewproj_matrix (render);
        break;
    case G_UNIFORM_INVERSE_TRANSPOSE_VIEWPROJ_MATRIX:
        buf =  inverse_transpose_viewproj_matrix (render);
        break;

    case G_UNIFORM_WORLDVIEW_MATRIX:
        buf =  worldview_matrix (render);
        break;
    case G_UNIFORM_INVERSE_WORLDVIEW_MATRIX:
        buf =  inverse_worldview_matrix (render);
        break;
    case G_UNIFORM_TRANSPOSE_WORLDVIEW_MATRIX:
        buf =  transpose_worldview_matrix (render);
        break;
    case G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEW_MATRIX:
        buf =  inverse_transpose_worldview_matrix (render);
        break;

    case G_UNIFORM_WORLDVIEWPROJ_MATRIX:
        buf =  worldviewproj_matrix (render);
        break;
    case G_UNIFORM_INVERSE_WORLDVIEWPROJ_MATRIX:
        buf =  inverse_worldviewproj_matrix (render);
        break;
    case G_UNIFORM_TRANSPOSE_WORLDVIEWPROJ_MATRIX:
        buf =  transpose_worldviewproj_matrix (render);
        break;
    case G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX:
        buf =  inverse_transpose_worldviewproj_matrix (render);
        break;

    case G_UNIFORM_TIME:
        fbuf[0] = priv->frame_timer.elapsed_time;
        return;
    case G_UNIFORM_FRAME_TIME:
        fbuf[0] = priv->frame_timer.frame_time;
        return;

    case G_UNIFORM_TIME_0_X:
        fbuf[0] = time_0_x (render, uniform->extra);
        return;
    case G_UNIFORM_COS_TIME_0_X:
        fbuf[0] = g_real_cos (time_0_x (render, uniform->extra));
        return;
    case G_UNIFORM_SIN_TIME_0_X:
        fbuf[0] = g_real_sin (time_0_x (render, uniform->extra));
        return;
    case G_UNIFORM_TAN_TIME_0_X:
        fbuf[0] = g_real_tan (time_0_x (render, uniform->extra));
        return;
    case G_UNIFORM_TIME_0_X_PACKED:
        fbuf[0] = time_0_x (render, uniform->extra);
        fbuf[1] = g_real_cos (fbuf[0]);
        fbuf[2] = g_real_sin (fbuf[0]);
        fbuf[3] = g_real_tan (fbuf[0]);
        return;

    case G_UNIFORM_TIME_0_1:
        fbuf[0] = time_0_1 (render, uniform->extra);
        return;
    case G_UNIFORM_COS_TIME_0_1:
        fbuf[0] = g_real_cos (time_0_1 (render, uniform->extra));
        return;
    case G_UNIFORM_SIN_TIME_0_1:
        fbuf[0] = g_real_sin (time_0_1 (render, uniform->extra));
        return;
    case G_UNIFORM_TAN_TIME_0_1:
        fbuf[0] = g_real_tan (time_0_1 (render, uniform->extra));
        return;
    case G_UNIFORM_TIME_0_1_PACKED:
        fbuf[0] = time_0_1 (render, uniform->extra);
        fbuf[1] = g_real_cos (fbuf[0]);
        fbuf[2] = g_real_sin (fbuf[0]);
        fbuf[3] = g_real_tan (fbuf[0]);
        return;
    default:
        x_error_if_reached();
        return;
    }
    memcpy (address, buf,
            uniform->elem_size * uniform->array_count);
}

static xbool
on_group                (gSceneRender   *render,
                         xint           group_id,
                         xsize          times)
{
    return times == 0;
}
static void
on_pass                 (gSceneRender   *render,
                         gPass          *pass)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    g_scene_render_set_pass (render, pass, 0);
}
static void refresh_shader (gShader *shader, gSceneRender *render)
{
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    g_shader_refresh(shader, priv->pass, render, priv->shader_scope);
    g_shader_upload(shader, priv->pass);
}
static void
on_renderable           (gSceneRender   *render,
                         gRenderContext *cxt,
                         xPtrArray      *manual_lights,
                         xbool          scissoring)
{
    gRenderOp op;
    struct SceneRenderPrivate *priv;
    priv = SCENE_RENDER_PRIVATE (render);

    g_scene_render_set_renderable (render,cxt->renderable, cxt->node, cxt->user_data);
    priv->shader_scope |= G_UNIFORM_OBJECT;

    g_pass_foreach_shader(priv->pass, refresh_shader, render);
    priv->shader_scope = 0;
    //g_renderable_textures(cxt->renderable, priv->pass, cxt->user_data);;

    g_render_setup (priv->pass);
    g_renderable_op (cxt->renderable, &op, cxt->user_data);
    g_render_draw (&op);
}
static void
on_render               (gSceneRender   *render,
                         gPass          *pass,
                         gRenderContext *cxt,
                         xPtrArray      *manual_lights,
                         xbool          scissoring)
{
    on_pass (render, pass);
    on_renderable (render, cxt, manual_lights, scissoring);
}
static void
g_scene_render_class_init (gSceneRenderClass*klass)
{
    x_class_set_private(klass, sizeof(struct SceneRenderPrivate));
    klass->on_group = on_group;
    klass->on_pass  = on_pass;
    klass->on_renderable = on_renderable;
    klass->on_render = on_render;
}
