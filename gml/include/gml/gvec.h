#ifndef __G_VEC_H__
#define __G_VEC_H__
#include "greal.h"
X_BEGIN_DECLS
#define G_VEC_MAX                   g_vec_1(X_MAXFLOAT)
#define G_VEC_INF                   g_vec_1(0x7F800000)
#define G_VEC_NAN                   g_vec_1(0x7FC00000)
#define G_VEC_EPSILON               g_vec_1(G_EPSILON)
#define G_VEC_PI                    g_vec_1(G_PI)
#define G_VEC_X                     g_vec(1,0,0,0)
#define G_VEC_Y                     g_vec(0,1,0,0)
#define G_VEC_Z                     g_vec(0,0,1,0)
#define G_VEC_W                     g_vec(0,0,0,1)
#define G_VEC_NEG_Z                 g_vec(0,0,-1,0)
#define G_VEC_0                     g_vec_1(0)
#define G_VEC_1                     g_vec_1(1)
#define G_QUAT_EYE                  G_VEC_W

X_INLINE_FUNC
gvec        g_vec_mask_xyz          (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_mask_xyz1         (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_mask_x            (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_mask_y            (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_mask_z            (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_mask_w            (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_abs               (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_flip_yw           (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_flip_zw           (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_flip_xw           (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_flip_xw           (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_flip_xyz          (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_inv               (const gvec     rec);
X_INLINE_FUNC
gvec        g_vec_1                 (const greal    xyzw);                  
X_INLINE_FUNC
gvec        g_vec                   (const greal    x,
                                     const greal    y,
                                     const greal    z,
                                     const greal    w);
X_INLINE_FUNC
gvec        g_vec_load              (const greal    *src);
X_INLINE_FUNC
void        g_vec_store             (const gvec     vec,
                                     xfloat         *dst);
X_INLINE_FUNC
gvec        g_vec_splat_x           (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_splat_y           (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_splat_z           (const gvec     vec);
X_INLINE_FUNC
gvec        g_vec_splat_w           (const gvec     vec);
X_INLINE_FUNC
greal       g_vec_get_x             (const gvec     vec);
X_INLINE_FUNC
greal       g_vec_get_y             (const gvec     vec);
X_INLINE_FUNC
greal       g_vec_get_z             (const gvec     vec);
X_INLINE_FUNC
greal       g_vec_get_w             (const gvec     vec);
X_INLINE_FUNC
xint        g_vec_mmsk              (const gvec     v);
X_INLINE_FUNC
gvec        g_vec_le                (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
xbool       g_vec_le_s              (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_lt                (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
xbool       g_vec_lt_s              (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_ge                (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
xbool       g_vec_ge_s              (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_gt                (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
xbool       g_vec_gt_s              (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_eq                (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
xbool       g_vec_eq_s              (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_aeq               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
xbool       g_vec_aeq_s             (const gvec     v1,
                                     const gvec     v2);
#define     g_vec_aeq0_s(v)         g_vec_aeq_s (v, G_VEC_0)
X_INLINE_FUNC
gvec        g_vec_and               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_or                (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_min               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_max               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_clamp             (const gvec     v,
                                     const gvec     min,
                                     const gvec     max);
X_INLINE_FUNC
gvec        g_vec_select            (const gvec     v1,
                                     const gvec     v2,
                                     const gvec     ctrl);
X_INLINE_FUNC
gvec        g_vec_mul               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_div               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_add               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_sub               (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_scale             (const gvec     v1,
                                     const greal    s);
X_INLINE_FUNC
gvec        g_vec_lerp              (const gvec     t,
                                     const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_rlerp             (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec_sqrt              (const gvec     v);
X_INLINE_FUNC
gvec        g_vec_rsqrt             (const gvec     v);
X_INLINE_FUNC
gvec        g_vec_negate            (const gvec     v);
X_INLINE_FUNC
gvec        g_vec_mod_2pi           (const gvec     r);
X_INLINE_FUNC
gvec        g_vec_sin               (const gvec     r);
X_INLINE_FUNC
gvec        g_vec_cos               (const gvec     r);
X_INLINE_FUNC
void        g_vec_sincos            (const gvec     r,
                                     gvec           *sin,
                                     gvec           *cos);
X_INLINE_FUNC
guint       g_vec_color             (const gvec     c);
X_INLINE_FUNC
gvec        g_vec_rand              (const greal    start,
                                     const greal    end);

X_INLINE_FUNC
gvec        g_vec3_dot              (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec3_abs_dot          (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec3_len_sq           (const gvec     v);
X_INLINE_FUNC
gvec        g_vec3_len_sq           (const gvec     v);
X_INLINE_FUNC
gvec        g_vec3_norm             (const gvec     v);
X_INLINE_FUNC
gvec        g_vec3_cross            (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec3_rotate           (const gvec     v,
                                     const gquat    q);
X_INLINE_FUNC
gvec        g_vec3_inv_rot          (const gvec     v,
                                     const gquat    q);
X_INLINE_FUNC
gvec        g_vec3_affine           (const gvec     v,
                                     const gxform   *f);
X_INLINE_FUNC
gvec        g_vec3_inv_affine       (const gvec     v,
                                     const gxform   *f);
/* v * m, left multiply */
X_INLINE_FUNC
gvec        g_vec3_xform            (const gvec     v,
                                     const gmat3    *m);
X_INLINE_FUNC
gvec        g_vec3_rand             (const gvec     dir,
                                     const gvec     up,
                                     const grad     angle);

X_INLINE_FUNC
gvec        g_vec4_std              (const gvec     v);
X_INLINE_FUNC
gvec        g_vec4_dot              (const gvec     v1,
                                     const gvec     v2);
X_INLINE_FUNC
gvec        g_vec4_len_sq           (const gvec     v);
X_INLINE_FUNC
gvec        g_vec4_norm             (const gvec     v);
/* v * m, left multiply */
X_INLINE_FUNC
gvec        g_vec4_xform            (const gvec     v,
                                     const gmat4    *m);

#if defined (X_CAN_INLINE) || defined (__G_VEC_C__)
X_INLINE_FUNC gvec
g_vec_1                 (const greal    xyzw)
{
#ifdef G_SSE_INTRINSICS
    return _mm_set_ps1 (xyzw);
#else
    gvec ret = {xyzw,xyzw,xyzw,xyzw};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec                   (const greal    x,
                         const greal    y,
                         const greal    z,
                         const greal    w)
{
#ifdef G_SSE_INTRINSICS
    return _mm_set_ps (w, z, y, x);
#else
    gvec ret = {x,y,z,w};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_load              (const greal    *src)
{
#ifdef G_SSE_INTRINSICS
    if (((unsigned)src&0xF) == 0)
        return _mm_load_ps ((const float*)src);
    else
        return _mm_loadu_ps ((const float*)src);
#else
    gvec ret = {src[0], src[1], src[2], src[3]};
    return ret;
#endif
}
X_INLINE_FUNC void
g_vec_store             (const gvec     vec,
                         greal          *dst)
{
#ifdef G_SSE_INTRINSICS
    if (((unsigned)dst&0xF) == 0)
        _mm_store_ps ((float*)dst, vec);
    else
        _mm_storeu_ps ((float*)dst, vec);
#else
    int i;
    for(i=0;i<4;++i)
        dst[i] = vec.f[i];
#endif
}
X_INLINE_FUNC gvec
g_vec_eq                (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cmpeq_ps( v1, v2 );
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i]= (v1.u[i] == v2.u[i]) ? 0xffffffff : 0;
    return ret;
#endif
}
X_INLINE_FUNC xbool
g_vec_eq_s              (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_movemask_ps (_mm_cmpeq_ps (v1, v2)) == 0xF;
#else
    int i;
    for (i=0;i<4;++i)
        if (v1.u[i] != v2.u[i])
            return FALSE;
    return TRUE;
#endif
}
X_INLINE_FUNC gvec
g_vec_aeq               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    gvec delta = _mm_max_ps(_mm_sub_ps(v1,v2),
                            _mm_sub_ps(v2, v1));
    return _mm_cmple_ps(delta, G_VEC_EPSILON);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        if (v1.f[i] - v2.f[i] > G_EPSILON ||
            v2.f[i] - v1.f[i] > G_EPSILON)
            ret.u[i] = 0;
        else
            ret.u[i] = 0xffffffff;
    return ret;
#endif
}
X_INLINE_FUNC xbool
g_vec_aeq_s             (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return g_vec_mmsk (g_vec_aeq (v1, v2)) == 0xF;
#else
    int i;
    for (i=0;i<4;++i)
        if (v1.f[i] - v2.f[i] > G_EPSILON ||
            v2.f[i] - v1.f[i] > G_EPSILON)
            return FALSE;
    return TRUE;
#endif
}
X_INLINE_FUNC gvec
g_vec_splat_x            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_shuffle_ps (vec, vec, _MM_SHUFFLE(0, 0, 0, 0));
#else
    gvec ret = {vec.f[0], vec.f[0],vec.f[0],vec.f[0]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_splat_y            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_shuffle_ps (vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
#else
    gvec ret = {vec.f[1], vec.f[1],vec.f[1],vec.f[1]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_splat_z            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_shuffle_ps (vec, vec, _MM_SHUFFLE(2, 2, 2, 2));
#else
    gvec ret = {vec.f[2], vec.f[2],vec.f[2],vec.f[2]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_splat_w            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_shuffle_ps (vec, vec, _MM_SHUFFLE(3, 3, 3, 3));
#else
    gvec ret = {vec.f[3], vec.f[3],vec.f[3],vec.f[3]};
    return ret;
#endif
}
X_INLINE_FUNC greal
g_vec_get_x              (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cvtss_f32(_mm_shuffle_ps(vec,vec,_MM_SHUFFLE(0,0,0,0)));
#else
    return vec.f[0];
#endif
}
X_INLINE_FUNC greal
g_vec_get_y              (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cvtss_f32(_mm_shuffle_ps(vec,vec,_MM_SHUFFLE(1,1,1,1)));
#else
    return vec.f[1];
#endif
}
X_INLINE_FUNC greal
g_vec_get_z              (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cvtss_f32(_mm_shuffle_ps(vec,vec,_MM_SHUFFLE(2,2,2,2)));
#else
    return vec.f[2];
#endif
}
X_INLINE_FUNC greal
g_vec_get_w              (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cvtss_f32(_mm_shuffle_ps(vec,vec,_MM_SHUFFLE(3,3,3,3)));
#else
    return vec.f[3];
#endif
}
X_INLINE_FUNC xint
g_vec_mmsk              (const gvec     v)
{
#ifdef G_SSE_INTRINSICS
    return _mm_movemask_ps(v);
#else
    xint ret=0;
    int i;
    for (i=0;i<4;++i)
        ret |= ((v.f[i] > 0) << i);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_le                (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cmple_ps ( v1, v2 );
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i]= (v1.f[i] <= v2.f[i]) ? 0xffffffff : 0;
    return ret;
#endif
}
X_INLINE_FUNC xbool
g_vec_le_s              (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return g_vec_mmsk (g_vec_le (v1, v2)) == 0xF;
#else
    int i;
    for (i=0;i<4;++i)
        if (v1.f[i] > v2.f[i])
            return FALSE;
    return TRUE;
#endif
}
X_INLINE_FUNC gvec
g_vec_lt                (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cmplt_ps ( v1, v2 );
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i]= (v1.f[i] < v2.f[i]) ? 0xffffffff : 0;
    return ret;
#endif
}
X_INLINE_FUNC xbool
g_vec_lt_s              (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return g_vec_mmsk (g_vec_lt (v1, v2)) == 0xF;
#else
    int i;
    for (i=0;i<4;++i)
        if (v1.f[i] >= v2.f[i])
            return FALSE;
    return TRUE;
#endif
}
X_INLINE_FUNC gvec
g_vec_ge                (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cmpge_ps ( v1, v2 );
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i]= (v1.f[i] >= v2.f[i]) ? 0xffffffff : 0;
    return ret;
#endif
}
X_INLINE_FUNC xbool
g_vec_ge_s              (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return g_vec_mmsk (g_vec_ge (v1, v2)) == 0xF;
#else
    int i;
    for (i=0;i<4;++i)
        if (v1.f[i] < v2.f[i])
            return FALSE;
    return TRUE;
#endif
}
X_INLINE_FUNC gvec
g_vec_gt                (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_cmpgt_ps ( v1, v2 );
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i]= (v1.f[i] > v2.f[i]) ? 0xffffffff : 0;
    return ret;
#endif
}
X_INLINE_FUNC xbool
g_vec_gt_s              (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return g_vec_mmsk (g_vec_gt (v1, v2)) == 0xF;
#else
    int i;
    for (i=0;i<4;++i)
        if (v1.f[i] <= v2.f[i])
            return FALSE;
    return TRUE;
#endif
}
X_INLINE_FUNC gvec
g_vec_and               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_and_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i] = (v1.u[i] & v2.u[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_or                (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_or_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i] = (v1.u[i] | v2.u[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_min               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_min_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = MIN(v1.f[i], v2.f[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_max               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_max_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = MAX(v1.f[i], v2.f[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_clamp             (const gvec     v,
                         const gvec     min,
                         const gvec     max)
{
#ifdef G_SSE_INTRINSICS
    return _mm_min_ps (_mm_max_ps(min, v),max);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = CLAMP(v.f[i], min.f[i], max.f[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_select            (const gvec     v1,
                         const gvec     v2,
                         const gvec     ctrl)
{
#ifdef G_SSE_INTRINSICS
    gvec tmp1 = _mm_andnot_ps (ctrl, v1);
    gvec tmp2 = _mm_and_ps (v2, ctrl);
    return _mm_or_ps (tmp1, tmp2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.u[i] = (v1.u[i] & ~ ctrl.u[i]) | (v2.u[i] & ctrl.u[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_mul               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_mul_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = v1.f[i] * v2.f[i];
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_div               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_div_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = v1.f[i] / v2.f[i];
    return ret;
#endif
}

X_INLINE_FUNC gvec
g_vec_add               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_add_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = v1.f[i] + v2.f[i];
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_sub               (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_sub_ps (v1, v2);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = v1.f[i] - v2.f[i];
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_scale             (const gvec     v1,
                         const greal    s)
{
#ifdef G_SSE_INTRINSICS
    return _mm_mul_ps (v1, _mm_set_ps1(s));
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = v1.f[i] * s;
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_lerp              (const gvec     t,
                         const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    return _mm_add_ps(v1, _mm_mul_ps (_mm_sub_ps(v2, v1), t));
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] =(v2.f[i]-v1.f[i])*t.f[i] + v1.f[i];
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_rlerp             (const gvec     v1,
                         const gvec     v2)
{
    return g_vec_lerp (g_vec_rand(0,1), v1, v2);
}
X_INLINE_FUNC gvec
g_vec_sqrt              (const gvec     v)
{
#ifdef G_SSE_INTRINSICS
    return _mm_sqrt_ps(v);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = (greal)sqrt(v.f[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_rsqrt             (const gvec     v)
{
#ifdef G_SSE_INTRINSICS
    return _mm_rsqrt_ps(v);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] =g_real_rsqrt(v.f[i]);
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_negate            (const gvec     v)
{
#ifdef G_SSE_INTRINSICS
    return _mm_sub_ps (G_VEC_0, v);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = -v.f[i];
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_round             (const gvec    v)
{
    /* Truncate to (-1, 1) */
#ifdef G_SSE_INTRINSICS
    gvec ret;
    gveci vec_abs     = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,0x7FFFFFFF};
    gvecf vec_8388608 = {8388608.0f,8388608.0f,8388608.0f,8388608.0f};
    __m128i vInt;
    __m128i vTest = _mm_and_si128(*(const __m128i*)&v,*(const __m128i*)&vec_abs.v);
    /* Test for greater than 8388608 (All floats with NO fractionals, NAN and INF */
    vTest = _mm_cmplt_epi32(vTest,*(const __m128i*)&vec_8388608.v);
    /* Convert to int and back to float for rounding */
    vInt = _mm_cvtps_epi32(v);
    /* Convert back to floats */
    ret = _mm_cvtepi32_ps(vInt);
    /* All numbers less than 8388608 will use the round to int */
    ret = _mm_and_ps(ret, *(gvec*)&vTest);
    /* All others, use the ORIGINAL value */
    vTest = _mm_andnot_si128(vTest,*(const __m128i*)&v);
    return _mm_or_ps(ret,*(gvec*)&vTest);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i) {
        greal bias = (v.f[i] > 0) ? 0.5f : -0.5f;
        bias += v.f[i];
        if (G_IS_NAN (bias))
            ret.u[i] = 0x7FC00000;
        if (bias < 8388608.0f)
            ret.f[i] = (greal)(int)bias;
        else
            ret.f[i] = v.f[i];
    }
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_mod_2pi           (const gvec     r)
{
#ifdef G_SSE_INTRINSICS
    gvec ret = _mm_mul_ps (r, _mm_set_ps1(G_INV_2PI));
    ret = g_vec_round (ret);
    ret = _mm_mul_ps (ret, _mm_set_ps1(G_2PI));
    return _mm_sub_ps (r, ret);
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i) {
        greal n = r.f[i]* G_INV_2PI;
        greal bias = (n > 0) ? 0.5f : -0.5f;
        bias = (greal)(int)(bias + n);
        ret.f[i] = r.f[i] - bias*G_2PI;
    }
    return ret;
#endif
}
X_INLINE_FUNC void
g_vec_sincos            (const gvec     r,
                         gvec           *pSin,
                         gvec           *pCos)
{
    /*
       sin(V) ~= V - V^3 / 3! + V^5 / 5! - V^7 / 7! + V^9 / 9! - V^11 / 11! + V^13 / 13! - 
       V^15 / 15! + V^17 / 17! - V^19 / 19! + V^21 / 21! - V^23 / 23! (for -PI <= V < PI)
       cos(V) ~= 1 - V^2 / 2! + V^4 / 4! - V^6 / 6! + V^8 / 8! - V^10 / 10! + V^12 / 12! - 
       V^14 / 14! + V^16 / 16! - V^18 / 18! + V^20 / 20! - V^22 / 22! (for -PI <= V < PI)
       */
#ifdef G_SSE_INTRINSICS
    gvec v1,v2,v3,v4,v5,v6,v7,v8,v9,v10;
    gvec v11,v12,v13,v14,v15,v16,v17,v18,v19,v20,v21,v22,v23;
    gvec s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11;
    gvec c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11;
    gvec sin, cos;

    v1 = g_vec_mod_2pi (r);
    v2 = g_vec_mul (v1, v1);
    v3 = g_vec_mul (v2, v1);
    v4 = g_vec_mul (v2, v2);
    v5 = g_vec_mul (v3, v2);
    v6 = g_vec_mul (v3, v3);
    v7 = g_vec_mul (v4, v3);
    v8 = g_vec_mul (v4, v4);
    v9 = g_vec_mul (v5, v4);
    v10 = g_vec_mul (v5, v5);
    v11 = g_vec_mul (v6, v5);
    v12 = g_vec_mul (v6, v6);
    v13 = g_vec_mul (v7, v6);
    v14 = g_vec_mul (v7, v7);
    v15 = g_vec_mul (v8, v7);
    v16 = g_vec_mul (v8, v8);
    v17 = g_vec_mul (v9, v8);
    v18 = g_vec_mul (v9, v9);
    v19 = g_vec_mul (v10, v9);
    v20 = g_vec_mul (v10, v10);
    v21 = g_vec_mul (v11, v10);
    v22 = g_vec_mul (v11, v11);
    v23 = g_vec_mul (v12, v11);


    if (pSin) {
        s1  = _mm_set_ps1(-0.166666667f);
        s2  = _mm_set_ps1(8.333333333e-3f);
        s3  = _mm_set_ps1(-1.984126984e-4f);
        s4  = _mm_set_ps1(2.755731922e-6f);
        s5  = _mm_set_ps1(-2.505210839e-8f);
        s6  = _mm_set_ps1(1.605904384e-10f);
        s7  = _mm_set_ps1(-7.647163732e-13f);
        s8  = _mm_set_ps1(2.811457254e-15f);
        s9  = _mm_set_ps1(-8.220635247e-18f);
        s10  = _mm_set_ps1(1.957294106e-20f);
        s11  = _mm_set_ps1(-3.868170171e-23f);

        s1 = _mm_mul_ps(s1,v3);
        sin = _mm_add_ps(s1,v1);
        sin = _mm_add_ps (sin, _mm_mul_ps (s2, v5));
        sin = _mm_add_ps (sin, _mm_mul_ps (s3, v7));
        sin = _mm_add_ps (sin, _mm_mul_ps (s4, v9));
        sin = _mm_add_ps (sin, _mm_mul_ps (s5, v11));
        sin = _mm_add_ps (sin, _mm_mul_ps (s6, v13));
        sin = _mm_add_ps (sin, _mm_mul_ps (s7, v15));
        sin = _mm_add_ps (sin, _mm_mul_ps (s8, v17));
        sin = _mm_add_ps (sin, _mm_mul_ps (s9, v19));
        sin = _mm_add_ps (sin, _mm_mul_ps (s10, v21));
        sin = _mm_add_ps (sin, _mm_mul_ps (s11, v23));
        *pSin = sin;
    }
    if (pCos) {
        c1 = _mm_set_ps1(-0.5f);
        c2 = _mm_set_ps1(4.166666667e-2f);
        c3 = _mm_set_ps1(-1.388888889e-3f);
        c4 = _mm_set_ps1(2.480158730e-5f);
        c5 = _mm_set_ps1(-2.755731922e-7f);
        c6 = _mm_set_ps1(2.087675699e-9f);
        c7 = _mm_set_ps1(-1.147074560e-11f);
        c8 = _mm_set_ps1(4.779477332e-14f);
        c9 = _mm_set_ps1(-1.561920697e-16f);
        c10 = _mm_set_ps1(4.110317623e-19f);
        c11 = _mm_set_ps1(-8.896791392e-22f);

        cos = _mm_mul_ps(c1,v2);
        cos = _mm_add_ps(cos, _mm_set_ps1(1));
        cos = _mm_add_ps (cos, _mm_mul_ps (c2, v4));
        cos = _mm_add_ps (cos, _mm_mul_ps (c3, v6));
        cos = _mm_add_ps (cos, _mm_mul_ps (c4, v8));
        cos = _mm_add_ps (cos, _mm_mul_ps (c5, v10));
        cos = _mm_add_ps (cos, _mm_mul_ps (c6, v12));
        cos = _mm_add_ps (cos, _mm_mul_ps (c7, v14));
        cos = _mm_add_ps (cos, _mm_mul_ps (c8, v16));
        cos = _mm_add_ps (cos, _mm_mul_ps (c9, v18));
        cos = _mm_add_ps (cos, _mm_mul_ps (c10, v20));
        cos = _mm_add_ps (cos, _mm_mul_ps (c11, v22));
        *pCos = cos;
    }
#else
    int i;
    for (i=0;i<4;++i) {
        if (pSin)
            pSin->f[i] = (greal)sin(r.f[i]);
        if (pCos)
            pCos->f[i] = (greal)cos(r.f[i]);
    }
#endif
}

X_INLINE_FUNC gvec
g_vec_sin               (const gvec     r)
{
    gvec ret;
    g_vec_sincos (r, &ret, NULL);
    return ret;
}

X_INLINE_FUNC gvec
g_vec_cos               (const gvec     r)
{
    gvec ret;
    g_vec_sincos (r, NULL, &ret);
    return ret;
}
X_INLINE_FUNC guint
g_vec_color             (const gvec     c)
{
#ifdef G_SSE_INTRINSICS
    X_ALIGN(16) float tmp[4];
    _mm_store_ps (tmp, _mm_mul_ps (c, _mm_set_ps1(255)));
    return (((guint)tmp[0])<<24)|(((guint)tmp[0])<<16)|(((guint)tmp[0])<<8)|((guint)tmp[0]);
#else
    guint ret=0;
    int i;
    for(i=0;i<4;++i) /* a,r,g,b */
        ret = (ret<<8) | ((int)(c.f[i]*255));
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_rand              (const greal    start,
                         const greal    end)
{
    return g_vec(g_real_rand(start, end),
                 g_real_rand(start, end), 
                 g_real_rand(start, end),
                 g_real_rand(start, end));
}
X_INLINE_FUNC gvec
g_vec_mask_xyz          (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    gveci mask = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,0};
    return _mm_and_ps(vec, mask.v);
#else
    gvec ret = vec;
    ret.f[3] = 0;
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_mask_xyz1         (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_add_ps (_mm_set_ss (1), g_vec_mask_xyz(vec));
#else
    gvec ret = vec;
    ret.f[3] = 1;
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_mask_x            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    gveci mask = {0xFFFFFFFF, 0, 0, 0};
    return _mm_and_ps(vec, mask.v); 
#else
    gvec ret={vec.f[0], 0,0,0};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_mask_y            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    gveci mask = {0, 0xFFFFFFFF, 0, 0};
    return _mm_and_ps(vec, mask.v); 
#else
    gvec ret={0, vec.f[1],0,0};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_mask_z            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    gveci mask = {0, 0, 0xFFFFFFFF, 0};
    return _mm_and_ps(vec, mask.v); 
#else
    gvec ret={0, 0, vec.f[2], 0};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_mask_w            (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    gveci mask = {0, 0, 0,0xFFFFFFFF};
    return _mm_and_ps(vec, mask.v); 
#else
    gvec ret={0, 0, 0, vec.f[3]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_abs               (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    gveci mask = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,0x7FFFFFFF};
    return _mm_and_ps(vec, mask.v); 
#else
    gvec ret;
    int i;
    for(i=0;i<4;++i)
        ret.u[i] = vec.u[i]&0x7FFFFFFF;
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_flip_xw           (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_mul_ps(vec, _mm_set_ps(-1,1,1,-1));
#else
    gvec ret = {-vec.f[0], vec.f[1], vec.f[2], -vec.f[3]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_flip_yw           (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_mul_ps(vec, _mm_set_ps(-1,1,-1,1));
#else
    gvec ret = {vec.f[0], -vec.f[1], vec.f[2], -vec.f[3]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_flip_zw           (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_mul_ps(vec, _mm_set_ps(-1,-1,1,1));
#else
    gvec ret = {vec.f[0], vec.f[1], -vec.f[2], -vec.f[3]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_flip_xyz          (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_mul_ps(vec, _mm_set_ps(1,-1,-1,-1));
#else
    gvec ret = {-vec.f[0], -vec.f[1], -vec.f[2], vec.f[3]};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec_inv               (const gvec     vec)
{
#ifdef G_SSE_INTRINSICS
    return _mm_div_ps(_mm_set_ss(1), vec);
#else
    gvec ret;
    int i;
    for(i=0;i<4;++i)
        ret.f[i] = 1/vec.f[i];
    return ret;
#endif
}

#endif /* X_CAN_INLINE || __G_VEC_C__ */

X_END_DECLS
#endif /* __G_VEC_H__ */
