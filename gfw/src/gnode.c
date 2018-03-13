#include "config.h"
#include "gnode.h"
#include "gfw-prv.h"

struct NodePrivate
{
    xstr            name;
    gxform          local;
    gxform          derived;
    gxform          saved;
    gxform          inversed;
    gxform          offseted;
    gmat4           cached;
    xPtrArray       *children;
    xPtrArray       *dirty_children;
    xbool           queued_dirty    : 1;
    xbool           parent_dirty    : 1;
    xbool           child_dirty     : 1;
    xbool           cached_dirty    : 1;
    xbool           offseted_dirty  : 1;
    xbool           parent_notified : 1;
    xbool           inherit_scale   : 1;
    xbool           inherit_rotate  : 1;
};
X_DEFINE_TYPE (gNode, g_node, X_TYPE_OBJECT);
#define NODE_PRIVATE(o) ((struct NodePrivate*) (G_NODE (o)->priv))
enum {
    SIG_CHANGED,
    SIG_ATTACHED,
    SIG_DETACHED,
    N_SIGNALS
};
/* node property */
enum {
    PROP_0,
    PROP_NAME,
    PROP_POSITON,
    PROP_ROTATION,
    N_PROPERTIES
};

static xuint        _signal[N_SIGNALS];
static xHashTable   *_nodes = NULL;
static xPtrArray    *_queued_nodes = NULL;

static void
update_from_parent      (gNode          *node)
{
    struct NodePrivate *priv;
    priv = NODE_PRIVATE(node);

    priv->derived = priv->local;

    if (node->parent) {
        const gxform* pxform = g_node_xform (node->parent);

        if (priv->inherit_rotate)
            priv->derived.rotate = g_quat_mul(priv->derived.rotate, pxform->rotate);

        if (priv->inherit_scale)
            priv->derived.scale = g_vec_mul (priv->derived.scale, pxform->scale);

        priv->derived.offset = g_vec3_affine (priv->derived.offset, pxform);
    }
    priv->offseted_dirty = TRUE;
    priv->cached_dirty = TRUE;
    priv->parent_dirty = FALSE;
}
/* 将child放入node->dirty_children */
static void
request_update          (gNode          *node,
                         gNode          *child,
                         xbool          update_parent)
{
    struct NodePrivate *priv;
    priv = NODE_PRIVATE(node);

    if (priv->child_dirty) /*将更新所有children,故无须放入dirty_children */
        return;
    x_ptr_array_append1 (priv->dirty_children, child);

    if (node->parent && (!priv->parent_notified || update_parent)) {
        request_update (node->parent, node, update_parent);
        priv->parent_notified = TRUE;
    }
}
/* 设置更新标志,向上层请求更新 */
static void
need_update             (gNode          *node,
                         xbool          update_parent)
{
    struct NodePrivate *priv;
    priv = NODE_PRIVATE(node);

    node->stamp++;
    priv->parent_dirty = TRUE;
    priv->child_dirty  = TRUE;
    priv->cached_dirty = TRUE;
    priv->offseted_dirty = TRUE;
    priv->dirty_children->len = 0;

    if (node->parent && (!priv->parent_notified || update_parent)) {
        request_update (node->parent, node, update_parent);
        priv->parent_notified = TRUE;
    }
    x_signal_emit (node, _signal[SIG_CHANGED],0);
}
void
_g_process_queued_nodes  (void)
{
    gNode *node;
    xsize i;

    for (i=0; i<_queued_nodes->len; ++i) {
        node = _queued_nodes->data[i];
        NODE_PRIVATE(node)->queued_dirty = FALSE;
        need_update (node, TRUE);
        x_object_unref (node);
    }
    _queued_nodes->len = 0;
}
xptr
g_node_new_child        (gNode          *node,
                         xcstr          first_property,
                         ...)
{
    va_list argv;
    gNode *child;
    struct NodePrivate *priv;

    x_return_val_if_fail (G_IS_NODE (node), NULL);
    priv = NODE_PRIVATE(node);

    va_start(argv, first_property);
    child = x_object_new_valist (X_INSTANCE_TYPE (node), first_property, argv);
    va_end(argv);

    g_node_attach(child, node);
    return child;
}

void
g_node_move             (gNode          *node,
                         gvec           offset,
                         gXFormSpace    space)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    switch (space) {
    case G_XS_LOCAL:
        offset = g_vec3_rotate(offset, priv->local.rotate);
        priv->local.offset = g_vec_add (priv->local.offset, offset);
        break;
    case G_XS_PARENT:
        priv->local.offset = g_vec_add (priv->local.offset, offset);
        break;
    case G_XS_WORLD:
        if (node->parent) {
            const gquat r = g_node_xform (node->parent)->rotate;
            offset = g_vec3_inv_rot (offset, r);
        }
        priv->local.offset = g_vec_add (priv->local.offset, offset);
        break;
    default:
        x_error_if_reached();
    }
    need_update (node, FALSE);
}

void
g_node_rotate           (gNode          *node,
                         gquat          rotate,
                         gXFormSpace    space)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    switch (space) {
    case G_XS_LOCAL:
        priv->local.rotate = g_quat_mul (rotate, priv->local.rotate);
        break;
    case G_XS_PARENT:
        priv->local.rotate = g_quat_mul (priv->local.rotate, rotate);
        break;
    case G_XS_WORLD:
        {
            const gquat tmp = g_node_xform (node)->rotate;
            rotate = g_quat_mul (g_quat_inv (tmp), rotate);
            rotate = g_quat_mul (rotate, tmp);
            priv->local.rotate = g_quat_mul (priv->local.rotate, rotate);
        }
        break;
    default:
        x_error_if_reached();
    }
    need_update (node, FALSE);
}
void
g_node_scale            (gNode          *node,
                         const gvec     scale)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    priv->local.scale = g_vec_mul (priv->local.scale, scale);

    need_update (node, FALSE);
}
void
g_node_affine           (gNode          *node,
                         const gxform   *xform)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    priv->local.offset = g_vec_add (priv->local.offset, xform->offset);
    priv->local.rotate = g_quat_mul (priv->local.rotate, xform->rotate);
    priv->local.scale = g_vec_mul (priv->local.scale, xform->scale);

    need_update (node, FALSE);
}

void
g_node_place            (gNode          *node,
                         const gxform   *xform)
{
    struct NodePrivate *priv;

    x_return_if_fail (xform != NULL);
    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    priv->local = *xform;
    need_update (node, FALSE);
}
void
g_node_lookat           (gNode          *node,
                         const gvec     target,
                         const gXFormSpace space,
                         const gvec     local_dir)
{
    gvec origin;
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    switch (space) {
    case G_XS_WORLD:
        origin = g_node_xform (node)->offset;
        break;
    case G_XS_PARENT:
        origin = priv->local.offset;
        break;
    case G_XS_LOCAL:
        origin = G_VEC_0;
        break;
    default:
        x_error_if_reached();
    }
    g_node_face (node, g_vec_sub (target, origin), space, local_dir);
}
void
g_node_face             (gNode          *node,
                         const gvec     dir,
                         const gXFormSpace space,
                         const gvec     local_dir)
{
    gvec target_dir, cur_dir;
    const gxform* xform;
    struct NodePrivate *priv;

    if (g_vec_aeq0_s (dir))
        return;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    target_dir = g_vec3_norm (dir);
    xform = g_node_xform (node);
    /* Transform target direction to world space */
    switch (space) {
    case G_XS_PARENT:
        if (priv->inherit_rotate && node->parent) {
            const gxform* pxform = g_node_xform (node->parent);
            target_dir = g_vec3_inv_rot (target_dir, pxform->rotate);
        }
        break;
    case G_XS_LOCAL:
        target_dir = g_vec3_inv_rot (target_dir, xform->rotate);
        break;
    case G_XS_WORLD:
        /* default orientation */
        break;
    default:
        x_error_if_reached();
    }
    cur_dir = g_vec3_rotate (local_dir, xform->rotate);
    priv->local.rotate = g_quat_betw_vec3 (cur_dir, target_dir, G_VEC_0);

    need_update (node, FALSE);
}
void
g_node_save             (gNode          *node)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    priv->saved = priv->local;
    priv->inversed = g_xform_inv (g_node_xform (node));
    priv->offseted_dirty = TRUE;
}
void
g_node_restore          (gNode          *node)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    priv->local  = priv->saved;
    need_update (node, FALSE);
}
void
g_node_attach           (gNode          *node,
                         gNode          *parent)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    x_return_if_fail (G_IS_NODE (parent));
    x_return_if_fail (node->parent == NULL);

    priv = NODE_PRIVATE(parent);
    x_object_sink (node);
    node->parent = parent;
    x_ptr_array_append1 (priv->children, node);

    x_signal_emit (node, _signal[SIG_ATTACHED],0, parent);
    need_update (node, FALSE);
}
void
g_node_detach           (gNode          *node)
{
    struct NodePrivate *priv;
    gNode* parent;

    x_return_if_fail (G_IS_NODE (node));
    x_return_if_fail (node->parent != NULL);

    priv = NODE_PRIVATE(node->parent);
    parent = node->parent;
    node->parent = NULL;

    x_ptr_array_remove_data(priv->children, node);
    x_signal_emit (parent, _signal[SIG_DETACHED],0, parent);
    need_update (node, FALSE);
}
gxform*
g_node_xform            (gNode          *node)
{
    struct NodePrivate *priv;

    x_return_val_if_fail (G_IS_NODE (node), NULL);
    priv = NODE_PRIVATE(node);

    if (priv->parent_dirty)
        update_from_parent (node);

    return &priv->derived;
}
gmat4*
g_node_mat4             (gNode          *node)
{
    struct NodePrivate *priv;

    x_return_val_if_fail (G_IS_NODE (node), NULL);
    priv = NODE_PRIVATE(node);

    if (priv->cached_dirty) {
        priv->cached = g_mat4_xform (g_node_xform (node));
        priv->cached_dirty = FALSE;
    }
    return &priv->cached;
}
gxform*
g_node_offseted         (gNode          *node)
{
    struct NodePrivate *priv;

    x_return_val_if_fail (G_IS_NODE (node), NULL);
    priv = NODE_PRIVATE(node);

    if (priv->offseted_dirty) {
        priv->offseted_dirty = FALSE;
        priv->offseted = g_xform_mul (g_node_xform (node),
                                      &priv->inversed);
    }
    return &priv->offseted;
}
void
g_node_foreach          (gNode          *node,
                         xCallback      func,
                         xptr           user_data)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    x_ptr_array_foreach (priv->children, func, user_data);
}
void
g_node_dirty            (gNode          *node,
                         xbool          notify_parent,
                         xbool          queued_dirty)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    if (queued_dirty) {
        if (!priv->queued_dirty) {
            priv->queued_dirty = TRUE;
            x_object_ref(node);
            x_ptr_array_append1 (_queued_nodes, node);
        }
    }
    else
        need_update (node, notify_parent);
}

void
g_node_undirty          (gNode          *child)
{
    gNode *node;
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (child));
    node = child->parent;

    if (!node) return;
    priv = NODE_PRIVATE(node);

    x_ptr_array_remove_data (priv->dirty_children, child);
    if (priv->dirty_children->len == 0 && node->parent && !priv->child_dirty) {
        g_node_undirty (node);
        priv->parent_notified = FALSE;
    }
}
void
g_node_update           (gNode          *node,
                         xbool          update_children,
                         xbool          parent_chagned)
{
    gNodeClass *klass;
    x_return_if_fail (G_IS_NODE (node));

    klass = G_NODE_GET_CLASS (node);
    klass->update (node, update_children, parent_chagned);
}
static void
node_update             (gNode          *node,
                         xbool          update_children,
                         xbool          parent_chagned)
{
    struct NodePrivate *priv;

    x_return_if_fail (G_IS_NODE (node));
    priv = NODE_PRIVATE(node);

    priv->parent_notified = FALSE;

    if (priv->parent_dirty || parent_chagned)
        update_from_parent (node);

    if (update_children) {
        xsize   i;
        gNode   *child;
        if (priv->child_dirty || parent_chagned) {
            for (i=0; i<priv->children->len; ++i) {
                child = priv->children->data[i];
                g_node_update (child, TRUE, TRUE);
            }
        }
        else { /* Just update dirty children */
            for (i=0; i<priv->dirty_children->len; ++i) {
                child = priv->dirty_children->data[i];
                g_node_update (child, TRUE, FALSE);
            }
        }
        priv->dirty_children->len = 0;
        priv->child_dirty = FALSE;
    }
}
static void
set_property            (xObject            *node,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    struct NodePrivate *priv;
    priv = NODE_PRIVATE(node);

    switch(property_id) {
    case PROP_NAME:
        if (priv->name) {
            x_hash_table_remove (_nodes, priv->name);
            x_free (priv->name);
        }
        priv->name = x_value_dup_str (value);
        if (priv->name) {
            x_hash_table_insert (_nodes, priv->name, node);
        }
        break;
    }
}
static void
get_property            (xObject            *node,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    struct NodePrivate *priv;
    priv = NODE_PRIVATE(node);

    switch(property_id) {
    case PROP_NAME:
        x_value_set_str (value, priv->name);
        break;
    }
}
static void
g_node_init          (gNode       *node)
{
    struct NodePrivate *priv;
    priv = x_instance_private(node, G_TYPE_NODE);
    node->priv = priv;

    priv->local = g_xform_eye();
    priv->children = x_ptr_array_new ();
    priv->children->free_func = x_object_unref;
    priv->dirty_children = x_ptr_array_new ();
    priv->cached_dirty = TRUE;
    priv->offseted_dirty = TRUE;
    priv->parent_dirty = TRUE;
    priv->inherit_rotate = TRUE;
    priv->inherit_scale = TRUE;

    x_object_unsink (node);
}

static void
g_node_class_init       (gNodeClass  *klass)
{
    xParam         *param;
    xObjectClass   *oclass;

    x_class_set_private(klass, sizeof(struct NodePrivate));
    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;

    klass->update = node_update;

    param = x_param_str_new ("name",
                             "Node name",
                             "The name of the Node",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);

    x_oclass_install_param(oclass, PROP_NAME,param);

    /**
     * gNode::changed:
     * @node: the object which received the signal
     *
     *  the "changed" signal is emitted on a node transform changed
     */
    _signal[SIG_CHANGED] =
        x_signal_new ("changed",
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_LAST,
                      X_STRUCT_OFFSET (gNodeClass, changed),
                      NULL,
                      NULL,
                      X_TYPE_VOID,
                      0);
    /**
     * gNode::attached:
     * @node: the object which received the signal
     * @parent: the parent which attach to
     *
     *  the "attached" signal is emitted on a node attached to parent
     */
    _signal[SIG_ATTACHED] =
        x_signal_new ("attached",
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_LAST,
                      X_STRUCT_OFFSET (gNodeClass, attached),
                      NULL,
                      NULL,
                      X_TYPE_VOID,
                      1,
                      G_TYPE_NODE);
    /**
     * gNode::detached:
     * @node: the object which received the signal
     * @parent: the parent which detached from
     *
     *  the "detached" signal is emitted on a node detached from parent
     */
    _signal[SIG_DETACHED] =
        x_signal_new ("detached",
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_LAST,
                      X_STRUCT_OFFSET (gNodeClass, detached),
                      NULL,
                      NULL,
                      X_TYPE_VOID,
                      1,
                      G_TYPE_NODE);
    if (!_nodes)
        _nodes = x_hash_table_new (x_str_hash, x_str_cmp);

    if (!_queued_nodes)
        _queued_nodes = x_ptr_array_new ();
}
