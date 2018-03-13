#ifndef __G_RENDERABLE_H__
#define __G_RENDERABLE_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_RENDERABLE                 (g_renderable_type())
#define G_RENDERABLE(object)              (X_INSTANCE_CAST((object), G_TYPE_RENDERABLE, gRenderable))
#define G_RENDERABLE_CLASS(klass)         (X_CLASS_CAST(     (klass), G_TYPE_RENDERABLE, gRenderableIFace))
#define G_IS_RENDERABLE(object)           (X_INSTANCE_IS_TYPE((object), G_TYPE_RENDERABLE))
#define G_RENDERABLE_GET_CLASS(object)    (X_INSTANCE_GET_IFACE((object), G_TYPE_RENDERABLE, gRenderableIFace))

struct _gRenderableIFace
{
    xIFace                      parent;
    gTechnique* (*technique)    (gRenderable *renderable, xptr user_data);
    xsize (*model_matrix)       (gRenderable *renderable, gmat4 *mat, xptr user_data);
    xsize (*world_matrix)       (gRenderable *renderable, gmat4 *mat, gNode *node, xptr user_data);
    xbool (*proj_matrix)        (gRenderable *renderable, gmat4 *mat, xptr user_data);
    xbool (*view_matrix)        (gRenderable *renderable, gmat4 *mat, xptr user_data);
    xbool (*cast_shadow)        (gRenderable *renderable, xptr user_data);
    void  (*manual_uniform)     (gRenderable *renderable, gUniform *uniform, xptr address, xptr user_data);
    void  (*bind_textures)      (gRenderable *renderable, xptr user_data);
    void  (*render_op)          (gRenderable *renderable, gRenderOp *op, xptr user_data);
};

xType       g_renderable_type       (void);

/* get material technique */
gTechnique* g_renderable_technique  (gRenderable    *renderable,
                                     xptr           user_data);

/* whether caster shadow */
xbool       g_renderable_shadow     (gRenderable    *renderable,
                                     xptr           user_data);

/* fill model matrix */
xsize       g_renderable_mmatrix    (gRenderable    *renderable,
                                     gmat4          *model_matrix,
                                     xptr           user_data);
/* fill world matrix */
xsize       g_renderable_wmatrix    (gRenderable    *renderable,
                                     gmat4          *world_matrix,
                                     gNode          *node,
                                     xptr           user_data);

/* whether use custom proj matrix */
xbool       g_renderable_pmatrix    (gRenderable    *renderable,
                                     gmat4          *proj_matrix,
                                     xptr           user_data);

/* whether use custom view matrix */
xbool       g_renderable_vmatrix    (gRenderable    *renderable,
                                     gmat4          *view_matrix,
                                     xptr           user_data);

/* write manual uniform */
void        g_renderable_uniform    (gRenderable    *renderable,
                                     gUniform       *manual_uniform,
                                     xptr           address,
                                     xptr           user_data);
/* fill gRenderOp */
void        g_renderable_op         (gRenderable    *renderable,
                                     gRenderOp      *op,
                                     xptr           user_dara);
X_END_DECLS
#endif /* __G_RENDERABLE_H__ */
