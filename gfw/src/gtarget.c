#include "config.h"
#include "gtarget.h"
#include "gcamera.h"
#include "grender.h"
struct TargetPrivate
{
    xstr                name;
    xArray              *views;
    xsize               width;
    xsize               height;
};
X_DEFINE_TYPE_EXTENDED (gTarget, g_target, X_TYPE_OBJECT,X_TYPE_ABSTRACT,);
#define TARGET_PRIVATE(o) ((struct TargetPrivate*) (G_TARGET (o)->priv))
struct view {
    xint            zorder;
    gViewport       *viewport;
};
static void
view_free               (struct view    *v)
{
    g_viewport_set_camera (v->viewport, NULL);
    x_slice_free (gViewport,v->viewport);
}
static void
g_target_init           (gTarget        *target)
{
    struct TargetPrivate *priv;
    priv = x_instance_private(target, G_TYPE_TARGET);
    target->priv = priv;

    priv = TARGET_PRIVATE(target);

    priv->views = x_array_new (sizeof (struct view));
    priv->views->free_func = (xFreeFunc) view_free;
}
static void
target_finalize         (xObject        *target)
{
    struct TargetPrivate *priv;
    priv = TARGET_PRIVATE(target);

    x_array_unref (priv->views);
}
static void
g_target_class_init     (gTargetClass  *klass)
{
    xObjectClass   *oclass = X_OBJECT_CLASS (klass);

    x_class_set_private(klass, sizeof(struct TargetPrivate));
    oclass->finalize = target_finalize;
}

void
g_target_update         (gTarget        *target)
{
    xsize i;
    struct view *view;
    struct TargetPrivate *priv;

    x_return_if_fail (G_IS_TARGET (target));
    priv = TARGET_PRIVATE(target);
    x_return_if_fail (priv->views != NULL);

    priv = TARGET_PRIVATE(target);

    for (i=0; i<priv->views->len; ++i) {
        view = &x_array_index (priv->views, struct view, i);
        g_viewport_update (view->viewport);
    }
}

void
g_target_swap           (gTarget        *target)
{
    gTargetClass *klass;
    x_return_if_fail (G_IS_TARGET (target));

    klass = G_TARGET_GET_CLASS (target);

    if (klass->swap)
        klass->swap (target);
}
static xint
view_zorder_cmp         (struct view    *v1,
                         struct view    *v2)
{
    return v1->zorder - v2->zorder;
}
gViewport*
g_target_add_view       (gTarget        *target,
                         xint           zorder)
{
    xsize i;
    struct view *view;
    gViewport *viewport;
    struct TargetPrivate *priv;


    x_return_val_if_fail (G_IS_TARGET (target), NULL);
    priv = TARGET_PRIVATE(target);

    i = x_array_insert_sorted (priv->views, &zorder, NULL,
                               (xCmpFunc)view_zorder_cmp, FALSE);
    view = &x_array_index (priv->views, struct view, i);
    view->zorder = zorder;
    view->viewport = viewport = x_slice_new0 (gViewport);

    viewport->buffers = G_FRAME_COLOR|G_FRAME_DEPTH;
    viewport->depth = 1.0f;
    viewport->target = target;
    viewport->rx = 0;
    viewport->ry = 0;
    viewport->rw = 1;
    viewport->rh = 1;
    viewport->x = 0;
    viewport->y = 0;
    viewport->w = priv->width;
    viewport->h = priv->height;

    return viewport;
}
gViewport*
g_target_get_view       (gTarget        *target,
                         xint           zorder)
{
    xuint d, min_d;
    xsize i, j;
    struct view *view;
    struct TargetPrivate *priv;

    x_return_val_if_fail (G_IS_TARGET (target), NULL);
    priv = TARGET_PRIVATE(target);
    x_return_val_if_fail (priv->views != NULL, NULL);
    min_d = (xuint)-1;
    for (i=0; i<priv->views->len; ++i) {
        view = &x_array_index (priv->views, struct view, i);
        d = ABS (view->zorder, zorder);
        if (d <= min_d) {
            min_d = d;
            j = i;
        }
    }
    view = &x_array_index (priv->views, struct view, j);
    return view->viewport;
}
void
g_target_resize         (gTarget        *target,
                         xsize          width,
                         xsize          height)
{
    xsize i;
    gViewport *viewport;
    struct TargetPrivate *priv;

    x_return_if_fail (G_IS_TARGET (target));
    priv = TARGET_PRIVATE(target);
    x_return_if_fail (priv->views != NULL);

    priv->width = width;
    priv->height = height;
    for (i=0; i<priv->views->len; ++i) {
        viewport = x_array_index (priv->views, struct view, i).viewport;
        viewport->x = (xint)(viewport->rx * priv->width);
        viewport->y = (xint)(viewport->ry * priv->height);
        viewport->w = (xint)(viewport->rw * priv->width);
        viewport->h = (xint)(viewport->rh * priv->height);
    }
}

void
g_viewport_update       (gViewport      *view)
{
    x_return_if_fail (view != NULL);

    if (view->camera)
        g_camera_render (view->camera, view, view->show_overlay);
}

gray
g_viewport_ray          (gViewport      *view,
                         xint           x,
                         xint           y)
{
    greal fx = (greal)x, fy=(greal)y;

    fx = (x-fx)/view->w;
    fy = (y-fy)/view->h;

    return g_frustum_ray (G_FRUSTUM (view->camera), fx, fy);
}
void
g_viewport_set_camera   (gViewport      *view,
                         gCamera        *camera)
{
    x_return_if_fail (view != NULL);
    x_return_if_fail (view->camera != camera);

    if (view->camera) {
        x_object_add_weakptr (view->camera, &view->camera);
        view->camera = NULL;
    }
    if (camera) {
        view->camera = camera;
        x_object_add_weakptr(camera, &view->camera);
    }
}
