#ifndef __G_FRUSTUM_H__
#define __G_FRUSTUM_H__
#include "gmovable.h"
#include "gnode.h"
X_BEGIN_DECLS

#define G_TYPE_FRUSTUM              (g_frustum_type())
#define G_FRUSTUM(object)           (X_INSTANCE_CAST((object), G_TYPE_FRUSTUM, gFrustum))
#define G_FRUSTUM_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_FRUSTUM, gFrustumClass))
#define G_IS_FRUSTUM(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_FRUSTUM))
#define G_FRUSTUM_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_FRUSTUM, gFrustumClass))

gmat4*      g_frustum_pmatrix       (gFrustum       *frustum);

gmat4*      g_frustum_vmatrix       (gFrustum       *frustum);

void        g_frustum_move          (gFrustum       *frustum,
                                     const gvec     offset,
                                     const gXFormSpace space);

void        g_frustum_rotate        (gFrustum       *frustum,
                                     const gquat    rotate,
                                     const gXFormSpace space);
/* rotate around Z axis */
#define     g_frustum_roll(frustum, radian, space)                          \
    g_frustum_rotate(frustum, g_quat_around(G_VEC_Z, radian), space)

/* rotate around X axis */
#define     g_frustum_pitch(frustum, radian, space)                         \
    g_frustum_rotate(frustum, g_quat_around(G_VEC_X, radian), space)

/* rotate around Y axis */
#define     g_frustum_yaw(frustum, radian, space)                           \
    g_frustum_rotate(frustum, g_quat_around(G_VEC_Y, radian),space)

/* 在父坐标系中,进行仿射变换 */
void        g_frustum_affine        (gFrustum       *frustum,
                                     const gxform   *xform);
/* 在父坐标系中,设置空间位置 */
void        g_frustum_place         (gFrustum       *frustum,
                                     const gxform   *xform);

/* 将结点朝向目标点
 * local_dir是原始的本地朝向
 * space指定目标点target所在的坐标系
 */
void        g_frustum_lookat        (gFrustum       *frustum,
                                     const gvec     target,
                                     const gXFormSpace space,
                                     const gvec     local_dir);

/* 设置结点朝向
 * local_dir是原始的本地朝向
 * space指定目标朝向dir所在的坐标系
 */
void        g_frustum_face          (gFrustum       *frustum,
                                     const gvec     dir,
                                     const gXFormSpace space,
                                     const gvec     local_dir);

/** 保存当前的本地变换矩阵 */
void        g_frustum_save          (gFrustum       *frustum);

/** 恢复已保存的本地变换矩阵 */
void        g_frustum_restore       (gFrustum       *frustum);

/** 返回结点的绝对仿射变换 */
gxform*     g_frustum_xform         (gFrustum       *frustum);

gBoxSide    g_frustum_side_box      (gFrustum       *frustum,
                                     gaabb          *aabb);

void        g_frustum_reflect       (gFrustum       *frustum,
                                     gplane         *plane);

gray        g_frustum_ray           (gFrustum       *frustum,
                                     greal          screen_x,
                                     greal          screen_y);


struct _gFrustum
{
    gMovable        parent;
    gNode           *node;
    xsize           stamp;
    xptr            priv;
};

struct _gFrustumClass
{
    gMovableClass       parent;
};
xType       g_frustum_type          (void);

X_END_DECLS
#endif /* __G_FRUSTUM_H__ */
