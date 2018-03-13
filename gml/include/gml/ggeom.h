#ifndef __G_GEOM_H__
#define __G_GEOM_H__
#include "gtype.h"

X_BEGIN_DECLS

X_INLINE_FUNC
xbool       g_aabb_eq               (gaabb          *a,
                                     const gaabb    *b);
X_INLINE_FUNC
void        g_aabb_merge            (gaabb          *a,
                                     const gaabb    *b);
X_INLINE_FUNC
void        g_aabb_extent           (gaabb          *a,
                                     const gvec     v);
X_INLINE_FUNC
void        g_aabb_affine           (gaabb          *a,
                                     const gxform   *f);
X_INLINE_FUNC
gvec        g_aabb_center           (const gaabb    *a);
X_INLINE_FUNC
gvec        g_aabb_size             (const gaabb    *a);
X_INLINE_FUNC
gvec        g_aabb_half             (const gaabb    *a);
X_INLINE_FUNC
xbool       g_aabb_inside           (const gaabb    *a,
                                     const gaabb    *b);
X_INLINE_FUNC
gvec        g_plane_dist            (const gplane   p,
                                     const gvec     v);
X_INLINE_FUNC
gPlaneSide  g_plane_side            (const gplane   p,
                                     const gvec     v);
X_INLINE_FUNC
gPlaneSide  g_plane_side_box        (const gplane   p,
                                     const gvec     center,
                                     const gvec     halfsize);
X_INLINE_FUNC
gplane      g_plane_norm            (const gplane   p);

X_INLINE_FUNC
gBoxSide    g_ray_collide_aabb      (const gray     *ray,
                                     const gaabb    *a,
                                     greal          *dist);

#if defined (X_CAN_INLINE) || defined (__G_GEOM_C__)

X_INLINE_FUNC xbool
g_aabb_eq               (gaabb          *a,
                         const gaabb    *b)
{
    if (a->extent != b->extent)
        return FALSE;
    else if (a->extent > 0) {
        if (!g_vec_aeq_s(a->minimum, b->minimum))
            return FALSE;
        if (!g_vec_aeq_s(a->maximum, b->maximum))
            return FALSE;
    }
    return TRUE;
}
X_INLINE_FUNC void
g_aabb_merge            (gaabb          *a,
                         const gaabb    *b)
{
    if (a->extent < 0 || b->extent < 0)
        a->extent = -1;
    else if (a->extent == 0)
        *a = *b;
    else if (b->extent > 0) {
        a->minimum = g_vec_min (a->minimum, b->minimum);
        a->maximum = g_vec_max (a->maximum, b->maximum);
        a->extent = 1;
    }
}
X_INLINE_FUNC void
g_aabb_extent           (gaabb          *a,
                         const gvec     v)
{
    if (a->extent == 0) {
        a->minimum = a->maximum = v;
        a->extent = 1;
    }
    else if (a->extent > 0) {
        a->minimum = g_vec_min (a->minimum, v);
        a->maximum = g_vec_max (a->maximum, v);
    }
}
X_INLINE_FUNC gvec
g_aabb_center           (const gaabb    *a)
{
    if (a->extent <= 0)
        return G_VEC_0;
    return g_vec_scale (g_vec_add (a->maximum, a->minimum), 0.5f);
}
X_INLINE_FUNC gvec
g_aabb_size             (const gaabb    *a)
{
    if (a->extent <= 0)
        return G_VEC_0;
    return g_vec_sub (a->maximum, a->minimum);
}
X_INLINE_FUNC gvec
g_aabb_half             (const gaabb    *a)
{
    return g_vec_scale (g_aabb_size (a), 0.5f);
}
X_INLINE_FUNC xbool
g_aabb_inside           (const gaabb    *a,
                         const gaabb    *b)
{
    gvec center;
    if (a->extent == 0 || b->extent < 0)
        return TRUE;
    else if (b->extent == 0)
        return FALSE;
    center = g_aabb_center (a);
    if (!g_vec_le_s (center, b->maximum))
        return FALSE;
    else if (!g_vec_ge_s (center, b->minimum))
        return FALSE;
    return g_vec_le_s (g_aabb_size(a), g_aabb_size(b));
}
X_INLINE_FUNC void
g_aabb_affine            (gaabb         *a,
                          const gxform  *f)
{
    if (a->extent > 0){
        gvec center, half;

        half = g_aabb_half (a);
        center = g_vec3_affine (g_aabb_center (a), f);
        half = g_vec_mul (half, f->scale);
        half = g_vec3_rotate (half, f->rotate);
        a->maximum = g_vec_add (center, half);
        a->minimum = g_vec_sub (center, half);
    }
}
X_INLINE_FUNC gvec
g_plane_dist            (const gplane   p,
                         const gvec     v)
{
    gvec ret;
    ret = g_vec_add (g_vec3_dot(p, v), p);
    return g_vec_splat_w (ret);
}
X_INLINE_FUNC gPlaneSide
g_plane_side            (const gplane   p,
                         const gvec     v)
{
    gvec d = g_plane_dist (p, v);
    if (g_vec_aeq0_s (d))
        return G_NO_SIDE;
    else if (g_vec_le_s(d, G_VEC_0))
        return G_NEGATIVE_SIDE;
    else
        return G_POSITIVE_SIDE;
}
X_INLINE_FUNC gPlaneSide
g_plane_side_box        (const gplane   p,
                         const gvec     center,
                         const gvec     halfsize)
{
    gvec min_d = g_plane_dist (p, center);
    gvec max_d = g_vec3_abs_dot (p, halfsize);
    if (g_vec_lt_s (min_d, g_vec_negate (max_d)))
        return G_NEGATIVE_SIDE;
    else if (g_vec_gt_s (min_d, max_d))
        return G_POSITIVE_SIDE;
    else 
        return G_BOTH_SIDE;
}
X_INLINE_FUNC gplane
g_plane_norm            (const gplane   p)
{
    return g_vec_mul (p, g_vec_rsqrt(g_vec3_len_sq (p)));
}
X_INLINE_FUNC gBoxSide
g_ray_collide_aabb      (const gray     *ray,
                         const gaabb    *a,
                         greal          *dist)
{
    if (a->extent == 0)
        return G_BOX_OUTSIDE;

    if (a->extent < 0) {
        if (dist) *dist= 0;
        return G_BOX_INSIDE;
    }

    if (g_vec_ge_s(ray->origin, a->minimum)
        && g_vec_le_s (ray->origin, a->maximum)) {
        if (dist) *dist= 0;
        return G_BOX_INSIDE;
    }
    else {
        xint msk;
        gvec m, h;

        m = g_vec_and (g_vec_le (ray->origin, a->minimum),
            g_vec_gt (ray->direction, G_VEC_0));
        h = g_vec_and (g_vec_ge (ray->origin, a->maximum),
            g_vec_lt (ray->direction, G_VEC_0));

        msk = g_vec_mmsk(g_vec_or (m, h));
        if (!msk)
            return G_BOX_OUTSIDE;

        if (dist) {
            gvec d;
            greal tmp, min_d;

            d = g_vec_select (G_VEC_MAX, a->minimum, m);
            d = g_vec_select (d, a->maximum, h);
            d = g_vec_div (g_vec_sub (d, ray->origin), ray->direction);

            min_d = (msk&1) ? g_vec_get_x(d) : X_MAXFLOAT;
            if ( (msk&2) && min_d > (tmp = g_vec_get_y(d)))
                min_d = tmp;
            if ( (msk&4) && min_d > (tmp = g_vec_get_z(d)))
                min_d = tmp;
            /* ignore w */

            *dist = min_d;
        }
        return G_BOX_PARTIAL;
    }
}

#endif /* X_CAN_INLINE || __G_GEOM_C__*/

X_END_DECLS
#endif /* __G_GEOM_H__ */
