#ifndef __G_CAMERA_H__
#define __G_CAMERA_H__
#include "gfrustum.h"
X_BEGIN_DECLS

#define G_TYPE_CAMERA              (g_camera_type())
#define G_CAMERA(object)           (X_INSTANCE_CAST((object), G_TYPE_CAMERA, gCamera))
#define G_CAMERA_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_CAMERA, gCameraClass))
#define G_IS_CAMERA(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_CAMERA))
#define G_CAMERA_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_CAMERA, gCameraClass))

xType      g_camera_type           (void);

void       g_camera_track          (gCamera        *camera);

#define    g_camera_see(camera, aabb)                                   \
    (g_frustum_side_box ((gFrustum*)camera, aabb) >= G_BOX_PARTIAL)

void       g_camera_render         (gCamera        *camera,
                                    gViewport      *viewport,
                                    xbool          overlay);

struct _gCamera
{
    gFrustum        parent;
    gScene          *scene;
};

struct _gCameraClass
{
    gFrustumClass   parent;
};

X_END_DECLS
#endif /* __G_CAMERA_H__ */
