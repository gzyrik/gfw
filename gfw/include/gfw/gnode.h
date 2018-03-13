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

/** 创建子结点 */
xptr        g_node_new_child        (gNode          *node,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

/** 平移结点
 * space 指定平移操作所在的坐标系
 */
void        g_node_move             (gNode          *node,
                                     gvec           offset,
                                     gXFormSpace    space);

/** 旋转结点
 * space 指定旋转操作所在的坐标系
 */
void        g_node_rotate           (gNode          *node,
                                     gquat          rotate,
                                     gXFormSpace    space);

/** 绕Z轴旋转
 * space 指定旋转操作所在的坐标系
 */
#define     g_node_roll(node, radian, space)                                \
    g_node_rotate(node, g_quat_around (G_VEC_Z, radian), space)

/** 绕X轴旋转
 * space 指定旋转操作所在的坐标系
 */
#define     g_node_pitch(node, radian, space)                               \
    g_node_rotate(node, g_quat_around (G_VEC_X, radian), space)

/** 绕Y轴旋转
 * space 指定旋转操作所在的坐标系
 */
#define     g_node_yaw(node, radian, space)                                 \
    g_node_rotate(node, g_quat_around (G_VEC_Y, radian), space)

/** 在本地坐标系中,进行伸缩变换 */
void        g_node_scale            (gNode          *node,
                                     const gvec     scale);

/* 在父坐标系中,进行仿射变换 */
void        g_node_affine           (gNode          *node,
                                     const gxform   *xform);

/* 在父坐标系中,设置空间位置 */
void        g_node_place            (gNode          *node,
                                     const gxform   *xform);

/* 将结点朝向目标点
 * local_dir是原始的本地朝向
 * space指定目标点target所在的坐标系
 */
void        g_node_lookat           (gNode          *node,
                                     const gvec     target,
                                     const gXFormSpace space,
                                     const gvec     local_dir);
/* 设置结点朝向
 * local_dir是原始的本地朝向
 * space指定目标朝向dir所在的坐标系
 */
void        g_node_face             (gNode          *node,
                                     const gvec     dir,
                                     const gXFormSpace space,
                                     const gvec     local_dir);

/** 保存当前的本地变换矩阵 */
void        g_node_save             (gNode          *node);

/** 恢复已保存的本地变换矩阵 */
void        g_node_restore          (gNode          *node);

/** 通知结点的绝对变换矩阵需要更新 */
void        g_node_dirty            (gNode          *node,
                                     xbool          notify_parent,
                                     xbool          queued_dirty);

/** 撤消结点的绝对变换矩阵的更新 */
void        g_node_undirty          (gNode          *node);


/** 更新结点的绝对变换矩阵 */
void        g_node_update           (gNode          *node,
                                     xbool          update_children,
                                     xbool          parent_chagned);
/** 挂载到父结点 */
void        g_node_attach           (gNode          *node,
                                     gNode          *parent);

/** 从父结点中分离 */
void        g_node_detach           (gNode          *node);

/** 返回结点的绝对仿射变换 */
gxform*     g_node_xform            (gNode          *node);

/** 返回结点的绝对变换矩阵 */
gmat4*      g_node_mat4             (gNode          *node);

/** 返回结点的相对本地保存点的变换 */
gxform*     g_node_offseted         (gNode          *node);

/** 遍历所有子结点
 *  对每个子结点调用func(child, user_data)
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
