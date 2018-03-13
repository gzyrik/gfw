#include "config.h"
#include "gnode.h"
#include "gfrustum.h"
struct FrustumPrivate
{
    gmat4           proj_matrix;
    gmat4           view_matrix;
    gmat4           reflect_matrix;

    gbox            proj_box;
    grad            fov_y;
    greal           aspect;

    gxform          local;
    gxform          derived;
    gxform          saved;
    gplane          plane[G_BOX_FACES];

    xsize           ortho_height;
    xbool           ortho_proj : 1;
    xbool           custom_view_matrix : 1;
    xbool           cached_view_matrix : 1;
    xbool           custom_proj_matrix : 1;
    xbool           cached_proj_matrix : 1;
    xbool           cached_frustum_planes : 1;
    xbool           cached_derived_xform : 1;
    xbool           cached_worldspace_corners : 1;
    xbool           inherit_scale   : 1;
    xbool           inherit_rotate  : 1;
    xbool           reflect : 1;
};
X_DEFINE_TYPE (gFrustum, g_frustum, G_TYPE_MOVABLE);
#define FRUSTUM_PRIVATE(o) ((struct FrustumPrivate*) (G_FRUSTUM (o)->priv))
static void 
frustum_xform_changed   (gFrustum       *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    priv->cached_derived_xform = FALSE;
    frustum->stamp++;
}
static void
frustum_attached        (gMovable       *movable,
                         gNode          *node)
{
    gFrustum *frustum = (gFrustum*)movable;
    x_return_if_fail (frustum->node == NULL);

    frustum->node = node;
    frustum_xform_changed (frustum);
    x_signal_connect_swapped (node, "changed",
                              frustum_xform_changed, frustum);
}
static void
frustum_detached        (gMovable       *movable,
                         gNode          *node)
{
    gFrustum *frustum = (gFrustum*)movable;
    x_return_if_fail (frustum->node == node);

    frustum->node = NULL;
    frustum_xform_changed (frustum);
    x_signal_disconnect_func (node, frustum_xform_changed, frustum);
}

static void
frustum_enqueue         (gMovable       *movable,
                         gMovableContext*context)
{
}

static void
g_frustum_init          (gFrustum       *frustum)
{
    struct FrustumPrivate *priv;
    priv = x_instance_private(frustum, G_TYPE_FRUSTUM);
    frustum->priv = priv;

    priv->aspect = 4.0f/3.0f;
    priv->fov_y = G_PI/4.0f;
    priv->proj_box.front = 5;
    priv->proj_box.back = 100000;
    priv->local = g_xform_eye();
    priv->ortho_proj = FALSE;
    priv->ortho_height = 300;
    priv->inherit_rotate = TRUE;
    priv->inherit_scale = FALSE;
}
static void
g_frustum_class_init    (gFrustumClass  *klass)
{
    gMovableClass *mclass;

    x_class_set_private(klass, sizeof(struct FrustumPrivate));
    mclass = G_MOVABLE_CLASS (klass);
    mclass->type_flags = G_QUERY_FRUSTUM;
    mclass->attached = frustum_attached;
    mclass->detached = frustum_detached;
    mclass->enqueue = frustum_enqueue;
}
static void
calc_proj_box           (gFrustum       *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    if (priv->custom_proj_matrix){
    }
    else {
        greal half_w, half_h;
        if (priv->ortho_proj) {
            half_w = priv->ortho_height * priv->aspect * 0.5f;
            half_h = priv->ortho_height * 0.5f;
        }
        else {
            grad thetaY =  priv->fov_y * 0.5f;
            greal tanThetaY = g_real_tan (thetaY);
            greal tanThetaX = tanThetaY * priv->aspect;
            half_w = tanThetaX * priv->proj_box.front;
            half_h = tanThetaY * priv->proj_box.front;
        }
        priv->proj_box.left   = - half_w;
        priv->proj_box.right  = half_w;
        priv->proj_box.bottom = - half_h;
        priv->proj_box.top    = half_h;
    }
}
static void
frustum_update_proj     (gFrustum    *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    if (!priv->custom_proj_matrix) {
        calc_proj_box(frustum);

        if (priv->ortho_proj)
            priv->proj_matrix = g_mat4_ortho (&priv->proj_box);
        else
            priv->proj_matrix = g_mat4_persp (&priv->proj_box);

        priv->cached_proj_matrix = TRUE;
        priv->cached_frustum_planes = FALSE;
    }
}
static void
frustum_update_xform    (gFrustum   *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    if (priv->cached_derived_xform)
        return;

    if (frustum->node) {
        const gxform* pxform = g_node_xform (frustum->node);
        if (priv->inherit_rotate)
            priv->derived.rotate = g_quat_mul(priv->local.rotate, pxform->rotate);
        else
            priv->derived.rotate = priv->local.rotate;

        if (priv->inherit_scale)
            priv->derived.scale = g_vec_mul (priv->local.scale, pxform->scale);
        else
            priv->derived.scale = priv->local.scale;

        priv->derived.offset = g_vec3_affine (priv->local.offset, pxform);
        priv->cached_derived_xform = TRUE;
        priv->cached_view_matrix = FALSE;
    }
    else {
        priv->derived = priv->local;
        priv->cached_derived_xform = TRUE;
        priv->cached_view_matrix = FALSE;
    }
}
static void
frustum_update_planes   (gFrustum   *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    if (!priv->cached_frustum_planes) {
        xsize i;
        gmat4 combo;
        combo = g_mat4_mul (g_frustum_vmatrix (frustum),
                            g_frustum_pmatrix (frustum));
        combo = g_mat4_trans (&combo);

        priv->plane[G_BOX_LEFT]  = g_vec_add (combo.r[3], combo.r[0]);
        priv->plane[G_BOX_RIGHT] = g_vec_sub (combo.r[3], combo.r[0]);
        priv->plane[G_BOX_UP]    = g_vec_add (combo.r[3], combo.r[1]);
        priv->plane[G_BOX_DOWN]  = g_vec_sub (combo.r[3], combo.r[1]);
        priv->plane[G_BOX_FRONT] = g_vec_add (combo.r[3], combo.r[2]);
        priv->plane[G_BOX_BACK]  = g_vec_sub (combo.r[3], combo.r[2]);
        for (i=0;i<6;++i)
            priv->plane[i] = g_plane_norm (priv->plane[i]);

        priv->cached_frustum_planes = TRUE;
    }
}
gxform*
g_frustum_xform         (gFrustum       *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    frustum_update_xform (frustum);
    return &priv->derived;
}
static void
frustum_update_view     (gFrustum   *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    frustum_update_xform (frustum);

    if (!priv->custom_view_matrix) {
        const gxform *xform = g_frustum_xform (frustum);
        priv->view_matrix = g_mat4_view (xform->offset, xform->rotate);
        if (priv->reflect) {
            priv->view_matrix = g_mat4_mul(&priv->reflect_matrix, &priv->view_matrix);
        }
        priv->cached_view_matrix = TRUE;
        priv->cached_frustum_planes = FALSE;
        priv->cached_worldspace_corners = FALSE;
    }
}

gmat4*
g_frustum_pmatrix       (gFrustum   *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);

    frustum_update_proj (frustum);
    return &priv->proj_matrix;
}
gmat4*
g_frustum_vmatrix       (gFrustum   *frustum)
{
    struct FrustumPrivate *priv;
    priv = FRUSTUM_PRIVATE (frustum);
    
    frustum_update_view (frustum);
    return &priv->view_matrix;
}
void
g_frustum_move          (gFrustum       *frustum,
                         const gvec     offset,
                         const gXFormSpace space)
{
    struct FrustumPrivate *priv;

    x_return_if_fail (G_IS_FRUSTUM (frustum));
    priv = FRUSTUM_PRIVATE (frustum);

    switch (space) {
    case G_XS_LOCAL:
        priv->local.offset = g_vec_add (priv->local.offset,
            g_vec3_rotate (offset, priv->local.rotate));
        break;
    default:
    case G_XS_PARENT:
        priv->local.offset = g_vec_add (priv->local.offset, offset);
        break;
    case G_XS_WORLD:
        if (priv->inherit_rotate && frustum->node) {
            priv->local.offset = g_vec_add (priv->local.offset,
                g_vec3_inv_rot (offset, g_node_xform (frustum->node)->rotate));
        }
        else {
            priv->local.offset = g_vec_add (priv->local.offset, offset);
        }
        break;
    }
    frustum_xform_changed (frustum);
}

void
g_frustum_rotate        (gFrustum       *frustum,
                         const gquat    rotate,
                         const gXFormSpace space)
{
    struct FrustumPrivate *priv;

    x_return_if_fail (G_IS_FRUSTUM (frustum));
    priv = FRUSTUM_PRIVATE (frustum);

    switch (space) {
    case G_XS_LOCAL:
        priv->local.rotate = g_quat_mul (rotate, priv->local.rotate);
        break;
    default:
    case G_XS_PARENT:
        priv->local.rotate = g_quat_mul (priv->local.rotate, rotate);
        break;
    case G_XS_WORLD:
        if (priv->inherit_rotate && frustum->node) {
            gquat tmp = g_frustum_xform (frustum)->rotate;
            gquat rot = g_quat_mul (tmp, rotate);
            rot = g_quat_mul (rot, g_quat_inv (tmp));
            priv->local.rotate = g_quat_mul (rot,priv->local.rotate);
        }
        break;
    }
    frustum_xform_changed (frustum);
}

void
g_frustum_place         (gFrustum       *frustum,
                         const gxform   *xform)
{
    struct FrustumPrivate *priv;

    x_return_if_fail (xform != NULL);
    x_return_if_fail (G_IS_FRUSTUM (frustum));
    priv = FRUSTUM_PRIVATE (frustum);

    priv->local = *xform;
    frustum_xform_changed (frustum);
}
void
g_frustum_lookat        (gFrustum       *frustum,
                         const gvec     target,
                         const gXFormSpace space,
                         const gvec     local_dir)
{
    gvec origin;
    struct FrustumPrivate *priv;

    x_return_if_fail (G_IS_FRUSTUM (frustum));
    priv = FRUSTUM_PRIVATE (frustum);

    switch (space) {
    default:
    case G_XS_WORLD:
        origin = g_frustum_xform (frustum)->offset;
        break;
    case G_XS_PARENT:
        origin = priv->local.offset;
        break;
    case G_XS_LOCAL:
        origin = G_VEC_0;
        break;
    }
    g_frustum_face (frustum, g_vec_sub (target, origin), space, local_dir);
}
void
g_frustum_face          (gFrustum       *frustum,
                         const gvec     dir,
                         const gXFormSpace space,
                         const gvec     local_dir)
{
    gvec target_dir, cur_dir;
    gquat rotate, target_orient;
    const gxform* xform;
    struct FrustumPrivate *priv;

    x_return_if_fail (G_IS_FRUSTUM (frustum));
    priv = FRUSTUM_PRIVATE (frustum);

    if (g_vec_aeq0_s (dir))
        return;

    target_dir = g_vec3_norm (dir);
    xform = g_frustum_xform (frustum);
    /* Transform target direction to world space */
    switch (space) {
    case G_XS_PARENT:
        if (priv->inherit_rotate && frustum->node) {
            const gxform* pxform = g_node_xform (frustum->node);
            target_dir = g_vec3_inv_rot (target_dir, pxform->rotate);
        }
        break;
    case G_XS_LOCAL:
        target_dir = g_vec3_inv_rot (target_dir, xform->rotate);
        break;
    default:
    case G_XS_WORLD:
        /* default orientation */
        break;
    }
    cur_dir = g_vec3_rotate (local_dir, xform->rotate);
    rotate =  g_quat_betw_vec3 (cur_dir, target_dir, G_VEC_0);
    target_orient = g_quat_mul (xform->rotate, rotate);
    if (priv->inherit_rotate && frustum->node) {
        const gxform* pxform = g_node_xform (frustum->node);
        priv->local.rotate = g_quat_mul (target_orient, g_quat_inv(pxform->rotate));
    }
    else
        priv->local.rotate = target_orient;
    frustum_xform_changed (frustum);
}
void
g_frustum_save          (gFrustum       *frustum)
{
    struct FrustumPrivate *priv;

    x_return_if_fail (G_IS_FRUSTUM (frustum));
    priv = FRUSTUM_PRIVATE (frustum);

    priv->saved = priv->local;
}
void
g_frustum_restore       (gFrustum       *frustum)
{
    struct FrustumPrivate *priv;

    x_return_if_fail (G_IS_FRUSTUM (frustum));
    priv = FRUSTUM_PRIVATE (frustum);

    priv->local  = priv->saved;
    frustum_xform_changed (frustum);
}
gBoxSide
g_frustum_side_box      (gFrustum       *frustum,
                         gaabb          *aabb)
{

    xsize i;
    xbool inside = TRUE;
    gPlaneSide side;
    gvec center, halfsize;
    struct FrustumPrivate *priv;

    x_return_val_if_fail (G_IS_FRUSTUM (frustum), -1);
    priv = FRUSTUM_PRIVATE (frustum);

    if (aabb->extent == 0)
        return FALSE;
    if (aabb->extent < 0)
        return TRUE;
    frustum_update_planes (frustum);
    center = g_aabb_center (aabb);
    halfsize = g_aabb_half (aabb);
    for (i=0;i<G_BOX_FACES;++i) {
        if (i == G_BOX_BACK && g_real_aeq0 (priv->proj_box.back))
            continue;
        side = g_plane_side_box (priv->plane[i], center, halfsize);
        if (side == G_NEGATIVE_SIDE)
            return i;
        else if(side == G_BOTH_SIDE)
            inside = FALSE;
    }
    return inside ? G_BOX_INSIDE : G_BOX_PARTIAL;
}
void
g_frustum_reflect       (gFrustum   *frustum,
                         gplane     *plane)
{
}
gray
g_frustum_ray           (gFrustum   *frustum,
                         greal      x,
                         greal      y)
{
    gray ret;
    gvec origin, target;
    gmat4 inv_vp;

    inv_vp = g_mat4_mul (g_frustum_vmatrix (frustum),
                         g_frustum_pmatrix (frustum));
    inv_vp = g_mat4_inv (&inv_vp);
    /* coord form window to 3d */
    x = 2*x - 1;
    y = 1- 2*y;
    origin = g_vec (x, y, -1, 0);
    origin = g_vec4_xform (origin, &inv_vp);
    origin = g_vec4_std (origin);

    target = g_vec (x, y, 1, 0);
    target = g_vec4_xform (target, &inv_vp);
    target = g_vec4_std (target);

    ret.origin = g_vec_mask_xyz (origin);
    ret.direction = g_vec3_norm (g_vec_sub(target, origin));

    return ret;
}
