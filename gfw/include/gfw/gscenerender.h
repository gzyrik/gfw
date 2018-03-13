#ifndef __G_SCENE_RENDER_H__
#define __G_SCENE_RENDER_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_SCENE_RENDER              (g_scene_render_type())
#define G_SCENE_RENDER(object)           (X_INSTANCE_CAST((object), G_TYPE_SCENE_RENDER, gSceneRender))
#define G_SCENE_RENDER_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_SCENE_RENDER, gSceneRenderClass))
#define G_IS_SCENE_RENDER(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_SCENE_RENDER))
#define G_SCENE_RENDER_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_SCENE_RENDER, gSceneRenderClass))

void    g_scene_render_set_viewport (gSceneRender       *render,
                                     gViewport          *viewport);

void    g_scene_render_set_camera   (gSceneRender       *render,
                                     gCamera            *camera,
                                     xbool              relative);

void   g_scene_render_set_renderable(gSceneRender       *render,
                                     gRenderable        *renderable,
                                     gNode              *node,
                                     xptr               user_data);

void    g_scene_render_set_ambient  (gSceneRender       *render,
                                     gcolor             color);

void    g_scene_render_set_pass     (gSceneRender       *render,
                                     gPass              *pass,
                                     xsize              pass_index);

void    g_scene_render_set_wmatrix  (gSceneRender       *render,
                                     const gmat4        *matrix,
                                     xsize              count);

void    g_scene_render_write_uniform(gSceneRender       *render,
                                     gUniform           *uniform,
                                     xptr               address);
struct _gSceneRender
{
    xObject         parent;

    /*< private >*/
    gViewport       *viewport;
    gCamera         *camera;
    gcolor          ambient_color;
    gcolor          shadow_color;

    xbool           transparent_shadow_caster;
    xint            shadow_technique;
    xint            illumination_stage;
    xint            organisation;
    xptr            priv;
};

struct _gSceneRenderClass
{
    xObjectClass    parent;
    /* visit render queue */
    xbool(*on_group) (gSceneRender *render, xint group_id, xsize times);
    void (*on_pass)  (gSceneRender *render, gPass *pass);
    void (*on_renderable) (gSceneRender *render, gRenderContext *cxt,
                           xPtrArray *manual_lights, xbool scissoring);
    void (*on_render) (gSceneRender *scene, gPass *pass, gRenderContext *cxt,
                       xPtrArray *manual_lights, xbool scissoring);
};

xType       g_scene_render_type     (void);

X_END_DECLS
#endif /* __G_SCENE_RENDER_H__ */
