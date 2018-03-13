#include "config.h"
#include "grenderable.h"
#include "gnode.h"

X_DEFINE_IFACE (gRenderable, g_renderable);

static gTechnique*
renderable_technique    (gRenderable    *renderable,
                         xptr           user_data)
{
    return NULL;
}
static xbool
renderable_shadow       (gRenderable    *renderable,
                         xptr           user_data)
{
    return FALSE;
}
static xsize
renderable_mmatrix      (gRenderable    *renderable,
                         gmat4          *model_matrix,
                         xptr           user_data)
{
    *model_matrix = g_mat4_eye();
    return 1;
}
static xsize
renderable_wmatrix      (gRenderable    *renderable,
                         gmat4          *world_matrix,
                         gNode          *node,
                         xptr           user_data)
{
    *world_matrix = *g_node_mat4 (node);
    return 1;
}

static xbool
renderable_pmatrix      (gRenderable    *renderable,
                         gmat4          *proj_matrix,
                         xptr            user_data)
{
    return FALSE;
}
static xbool
renderable_vmatrix      (gRenderable    *renderable,
                         gmat4          *view_matrix,
                         xptr           user_data)
{
    return FALSE;
}
static void
renderable_uniform      (gRenderable    *renderable,
                         gUniform       *manual_uniform,
                         xptr           address,
                         xptr           user_data)
{
}
static void
renderable_op           (gRenderable    *renderable,
                         gRenderOp      *op,
                         xptr           user_data)
{
}
static void
g_renderable_default_init   (gRenderableIFace *iface)
{
    iface->technique = renderable_technique;
    iface->model_matrix = renderable_mmatrix;
    iface->world_matrix = renderable_wmatrix;
    iface->proj_matrix = renderable_pmatrix;
    iface->view_matrix = renderable_vmatrix;
    iface->cast_shadow = renderable_shadow;
    iface->manual_uniform = renderable_uniform;
    iface->render_op = renderable_op;
}

gTechnique*
g_renderable_technique  (gRenderable    *renderable,
                         xptr           user_data)
{
    gRenderableIFace  *iface;

    x_return_val_if_fail (G_IS_RENDERABLE(renderable), NULL);

    iface = G_RENDERABLE_GET_CLASS (renderable);

    return iface->technique (renderable, user_data);
}

xbool
g_renderable_shadow     (gRenderable    *renderable,
                         xptr           user_data)
{
    gRenderableIFace  *iface;

    x_return_val_if_fail (G_IS_RENDERABLE(renderable), FALSE);

    iface = G_RENDERABLE_GET_CLASS (renderable);
    
    return iface->cast_shadow (renderable, user_data);
}
xsize
g_renderable_mmatrix    (gRenderable    *renderable,
                         gmat4          *model_matrix,
                         xptr           user_data)
{
    gRenderableIFace  *iface;

    x_return_val_if_fail (G_IS_RENDERABLE(renderable), 0);

    iface = G_RENDERABLE_GET_CLASS (renderable);

    return iface->model_matrix(renderable, model_matrix, user_data);
}

xsize
g_renderable_wmatrix    (gRenderable    *renderable,
                         gmat4          *world_matrix,
                         gNode          *node,
                         xptr           user_data)
{
    gRenderableIFace  *iface;

    x_return_val_if_fail (G_IS_RENDERABLE(renderable), 0);

    iface = G_RENDERABLE_GET_CLASS (renderable);

    return iface->world_matrix(renderable, world_matrix, node, user_data);
}

xbool
g_renderable_pmatrix    (gRenderable    *renderable,
                         gmat4          *proj_matrix,
                         xptr            user_data)
{
    gRenderableIFace  *iface;

    x_return_val_if_fail (G_IS_RENDERABLE(renderable), FALSE);

    iface = G_RENDERABLE_GET_CLASS (renderable);

    return iface->proj_matrix(renderable, proj_matrix, user_data);
}

xbool
g_renderable_vmatrix    (gRenderable    *renderable,
                         gmat4          *view_matrix,
                         xptr           user_data)
{
    gRenderableIFace  *iface;

    x_return_val_if_fail (G_IS_RENDERABLE(renderable), FALSE);

    iface = G_RENDERABLE_GET_CLASS (renderable);

    return iface->view_matrix(renderable, view_matrix, user_data);
}

void
g_renderable_uniform    (gRenderable    *renderable,
                         gUniform       *manual_uniform,
                         xptr           address,
                         xptr           user_data)
{
    gRenderableIFace  *iface;

    x_return_if_fail (G_IS_RENDERABLE(renderable));

    iface = G_RENDERABLE_GET_CLASS (renderable);

    iface->manual_uniform(renderable, manual_uniform, address, user_data);
}
void
g_renderable_op         (gRenderable    *renderable,
                         gRenderOp      *op,
                         xptr           user_data)
{
    gRenderableIFace  *iface;

    x_return_if_fail (G_IS_RENDERABLE(renderable));

    iface = G_RENDERABLE_GET_CLASS (renderable);

    iface->render_op(renderable, op, user_data);
}

