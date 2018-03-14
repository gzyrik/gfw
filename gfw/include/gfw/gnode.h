#ifndef __G_NODE_H__
#define __G_NODE_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_NODE                 (g_node_type())
#define G_NODE(object)              (X_INSTANCE_CAST((object), G_TYPE_NODE, gNode))
#define G_NODE_CLASS(klass)         (X_CLASS_CAST((klass), G_TYPE_NODE, gNodeClass))
#define G_IS_NODE(object)           (X_INSTANCE_IS_TYPE((object), G_TYPE_NODE))
#define G_NODE_GET_CLASS(object)    (X_INSTANCE_GET_CLASS((object), G_TYPE_NODE, gNodeClass))

typedef enum {
    G_XS_LOCAL,
    G_XS_PARENT,
    G_XS_WORLD,
} gXFormSpace;

/** �����ӽ�� */
xptr        g_node_new_child        (gNode          *node,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

/** ƽ�ƽ��
 * space ָ��ƽ�Ʋ������ڵ�����ϵ
 */
void        g_node_move             (gNode          *node,
                                     gvec           offset,
                                     gXFormSpace    space);

/** ��ת���
 * space ָ����ת�������ڵ�����ϵ
 */
void        g_node_rotate           (gNode          *node,
                                     gquat          rotate,
                                     gXFormSpace    space);

/** ��Z����ת
 * space ָ����ת�������ڵ�����ϵ
 */
#define     g_node_roll(node, radian, space)                                \
    g_node_rotate(node, g_quat_around (G_VEC_Z, radian), space)

/** ��X����ת
 * space ָ����ת�������ڵ�����ϵ
 */
#define     g_node_pitch(node, radian, space)                               \
    g_node_rotate(node, g_quat_around (G_VEC_X, radian), space)

/** ��Y����ת
 * space ָ����ת�������ڵ�����ϵ
 */
#define     g_node_yaw(node, radian, space)                                 \
    g_node_rotate(node, g_quat_around (G_VEC_Y, radian), space)

/** �ڱ�������ϵ��,���������任 */
void        g_node_scale            (gNode          *node,
                                     const gvec     scale);

/* �ڸ�����ϵ��,���з���任 */
void        g_node_affine           (gNode          *node,
                                     const gxform   *xform);

/* �ڸ�����ϵ��,���ÿռ�λ�� */
void        g_node_place            (gNode          *node,
                                     const gxform   *xform);

/* ����㳯��Ŀ���
 * local_dir��ԭʼ�ı��س���
 * spaceָ��Ŀ���target���ڵ�����ϵ
 */
void        g_node_lookat           (gNode          *node,
                                     const gvec     target,
                                     const gXFormSpace space,
                                     const gvec     local_dir);
/* ���ý�㳯��
 * local_dir��ԭʼ�ı��س���
 * spaceָ��Ŀ�곯��dir���ڵ�����ϵ
 */
void        g_node_face             (gNode          *node,
                                     const gvec     dir,
                                     const gXFormSpace space,
                                     const gvec     local_dir);

/** ���浱ǰ�ı��ر任���� */
void        g_node_save             (gNode          *node);

/** �ָ��ѱ���ı��ر任���� */
void        g_node_restore          (gNode          *node);

/** ֪ͨ���ľ��Ա任������Ҫ���� */
void        g_node_dirty            (gNode          *node,
                                     xbool          notify_parent,
                                     xbool          queued_dirty);

/** �������ľ��Ա任����ĸ��� */
void        g_node_undirty          (gNode          *node);


/** ���½��ľ��Ա任���� */
void        g_node_update           (gNode          *node,
                                     xbool          update_children,
                                     xbool          parent_chagned);
/** ���ص������ */
void        g_node_attach           (gNode          *node,
                                     gNode          *parent);

/** �Ӹ�����з��� */
void        g_node_detach           (gNode          *node);

/** ���ؽ��ľ��Է���任 */
gxform*     g_node_xform            (gNode          *node);

/** ���ؽ��ľ��Ա任���� */
gmat4*      g_node_mat4             (gNode          *node);

/** ���ؽ�����Ա��ر����ı任 */
gxform*     g_node_offseted         (gNode          *node);

/** ���������ӽ��
 *  ��ÿ���ӽ�����func(child, user_data)
 */
void        g_node_foreach          (gNode          *node,
                                     xCallback      func,
                                     xptr           user_data);
struct _gNode
{
    xObject         _;
    gNode           *parent;
    xsize           stamp;
    xptr            priv;
};
struct _gNodeClass
{
    xObjectClass    object;
    void (*update)  (gNode *node, xbool children, xbool parent);
    void (*changed) (gNode *node);
    void (*attached)(gNode *node, gNode *parent);
    void (*detached)(gNode *node, gNode *parent);
};
xType       g_node_type             (void);

X_END_DECLS
#endif /* __G_NODE_H__ */
