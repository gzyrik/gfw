#ifndef __G_TARGET_H__
#define __G_TARGET_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_TARGET              (g_target_type())
#define G_TARGET(object)           (X_INSTANCE_CAST((object), G_TYPE_TARGET, gTarget))
#define G_TARGET_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_TARGET, gTargetClass))
#define G_IS_TARGET(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_TARGET))
#define G_TARGET_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_TARGET, gTargetClass))

void        g_target_update         (gTarget        *target);

void        g_target_swap           (gTarget        *target);

void        g_target_resize         (gTarget        *target,
                                     xsize          width,
                                     xsize          height);

gViewport*  g_target_add_view       (gTarget        *target,
                                     xint           zorder);

gViewport*  g_target_get_view       (gTarget        *target,
                                     xint           zorder);

void        g_viewport_update       (gViewport      *view);

gray        g_viewport_ray          (gViewport      *view,
                                     xint           x,
                                     xint           y);

void        g_viewport_set_camera   (gViewport      *view,
                                     gCamera        *camera);

struct _gViewport
{
    gTarget         *target;
    xbool           shadow_enabled;
    xbool           show_overlay;
    xuint           buffers;
    gcolor          color;
    greal           depth;
    guint           stencil;
    gCamera         *camera;
    greal           rx,ry,rw,rh;
    xint            x,y,w,h;
};

struct _gTarget
{
    xObject             _;
    xstr                name;
    xptr                priv;
};
struct _gTargetClass
{
    xObjectClass        _;
    xint                priority;
    void (*swap) (gTarget *target);
};
xType       g_target_type           (void);

X_END_DECLS
#endif /* __G_TARGET_H__ */
