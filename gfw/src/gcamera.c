#include "config.h"
#include "gcamera.h"
#include "gscene.h"
X_DEFINE_TYPE (gCamera, g_camera, G_TYPE_FRUSTUM);

static void
g_camera_init           (gCamera        *texture)
{
}

static void
g_camera_class_init     (gCameraClass   *klass)
{
    gMovableClass *mclass;

    mclass = G_MOVABLE_CLASS (klass);
}

void
g_camera_track          (gCamera        *camera)
{

}

void
g_camera_render         (gCamera        *camera,
                         gViewport      *viewport,
                         xbool          overlay)
{
    g_scene_render (camera->scene, viewport, camera,  overlay);
}

