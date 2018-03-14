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

/* �ڸ�����ϵ��,���з���任 */
void        g_frustum_affine        (gFrustum       *frustum,
                                     const gxform   *xform);
/* �ڸ�����ϵ��,���ÿռ�λ�� */
void        g_frustum_place         (gFrustum       *frustum,
                                     const gxform   *xform);

/* ����㳯��Ŀ���
 * local_dir��ԭʼ�ı��س���
 * spaceָ��Ŀ���target���ڵ�����ϵ
 */
void        g_frustum_lookat        (gFrustum       *frustum,
                                     const gvec     target,
                                     const gXFormSpace space,
                                     const gvec     local_dir);

/* ���ý�㳯��
 * local_dir��ԭʼ�ı��س���
 * spaceָ��Ŀ�곯��dir���ڵ�����ϵ
 */
void        g_frustum_face          (gFrustum       *frustum,
                                     const gvec     dir,
                                     const gXFormSpace space,
                                     const gvec     local_dir);

/** ���浱ǰ�ı��ر任���� */
void        g_frustum_save          (gFrustum       *frustum);

/** �ָ��ѱ���ı��ر任���� */
void        g_frustum_restore       (gFrustum       *frustum);

/** ���ؽ��ľ��Է���任 */
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
