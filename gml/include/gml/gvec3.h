#ifndef __G_VEC3_H__
#define __G_VEC3_H__
#include "gtype.h"

X_BEGIN_DECLS

#if defined (X_CAN_INLINE) || defined (__G_VEC3_C__)
X_INLINE_FUNC gvec
g_vec3_dot              (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    gvec dot, tmp;
    dot = _mm_mul_ps (v1, v2);
    tmp = _mm_shuffle_ps(dot,dot,_MM_SHUFFLE(2,1,2,1));/* z*z, y*y, z*z, y*y */
    dot = _mm_add_ss(dot,tmp); /* x*x+z*z,y*y,z*z,w*w */
    tmp = _mm_shuffle_ps(tmp,tmp,_MM_SHUFFLE(1,1,1,1));/* y*y, y*y, y*y, y*y */
    dot = _mm_add_ss(dot,tmp); /* x*x+z*z+y*y, ... */
    return _mm_shuffle_ps(dot,dot,_MM_SHUFFLE(0,0,0,0));
#else
    greal dot = v1.f[0]*v2.f[0]+ v1.f[1]*v2.f[1]+ v1.f[2]*v2.f[2];
    gvec ret = {dot, dot, dot, dot};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec3_abs_dot          (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    gvec dot, tmp;
    dot = g_vec_abs(_mm_mul_ps (v1, v2));
    tmp = _mm_shuffle_ps(dot,dot,_MM_SHUFFLE(2,1,2,1));/* z*z, y*y, z*z, y*y */
    dot = _mm_add_ss(dot, tmp); /* x*x+z*z,y*y,z*z,w*w */
    tmp = _mm_shuffle_ps(tmp,tmp,_MM_SHUFFLE(1,1,1,1));/* y*y, y*y, y*y, y*y */
    dot = _mm_add_ss(dot,tmp); /* x*x+z*z+y*y, ... */
    return _mm_shuffle_ps(dot,dot,_MM_SHUFFLE(0,0,0,0));
#else
    gvec ret;
    greal dot = 0;
    int i;
    for (i=0;i<3;++i)
        dot += ABS(v1.f[i]*v2.f[i],0);
    ret.f[0]=ret.f[1]=ret.f[2]=ret.f[3]=dot;
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec3_len_sq           (const gvec     v)
{
    return g_vec3_dot (v, v);
}
X_INLINE_FUNC gvec
g_vec3_norm             (const gvec     v)
{
#ifdef G_SSE_INTRINSICS
    gvec lensq, mask, ret;
    lensq = g_vec3_len_sq (v);
    /* prepare for divison */
    ret = _mm_sqrt_ps (lensq);
    /* Test for a divide by zero (Must be FP to detect -0.0) */
    mask = _mm_setzero_ps ();
    mask = _mm_cmpneq_ps (mask, ret);
    /* If the length is infinity, set the elements to zero */
    lensq = _mm_cmpneq_ps (lensq, G_VEC_INF);
    ret = _mm_div_ps (v, ret);
    ret = _mm_and_ps (ret, mask);
    mask = _mm_andnot_ps(lensq, G_VEC_NAN);
    ret = _mm_and_ps(ret,lensq);
    return _mm_or_ps(ret, mask);
#else
    gvec ret;
    int i;
    greal d = 0;
    for (i=0;i<3;++i)
        d += v.f[i]*v.f[i];
    if (d < G_EPSILON)
        return G_VEC_0;
    d = g_real_rsqrt(d);
    for (i=0;i<3;++i)
        ret.f[i] = v.f[i]*d;
    ret.f[3] = 0;
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec3_cross            (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    gvec tmp0, tmp1, ret;

    tmp0 = _mm_shuffle_ps ( v1, v1, _MM_SHUFFLE(3,0,2,1) );
    tmp1 = _mm_shuffle_ps ( v2, v2, _MM_SHUFFLE(3,1,0,2) );
    ret = _mm_mul_ps (tmp0, tmp1);

    tmp0 = _mm_shuffle_ps ( v1, v1, _MM_SHUFFLE(3,1,0,2) );
    tmp1 = _mm_shuffle_ps ( v2, v2, _MM_SHUFFLE(3,0,2,1) );
    tmp0 = _mm_mul_ps(tmp0, tmp1);

    ret = _mm_sub_ps(ret, tmp0);
    /* Set w to zero */
    return g_vec_mask_xyz(ret);
#else
    gvec ret;
    ret.f[0] = v1.f[1] * v2.f[2] - v1.f[2] * v2.f[1];
    ret.f[1] = v1.f[2] * v2.f[0] - v1.f[0] * v2.f[2];
    ret.f[2] = v1.f[0] * v2.f[1] - v1.f[1] * v2.f[0];
    ret.f[3] = 0;
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec3_rotate           (const gvec     v,
                         const gquat    quat)
{
    gvec v3, conj;
    v3 = g_vec_mask_xyz(v);
    conj = g_quat_conj (quat);
    v3 = g_quat_mul (conj, v3);
    return g_quat_mul (v3, quat);
}
X_INLINE_FUNC gvec
g_vec3_inv_rot          (const gvec     v,
                         const gquat    quat)
{
    gvec v3, conj;
    v3 = g_vec_mask_xyz(v);
    v3 = g_quat_mul (quat, v3);
    conj = g_quat_conj (quat);
    return g_quat_mul (v3, conj);
}
X_INLINE_FUNC gvec
g_vec3_affine           (const gvec     v,
                         const gxform   *f)
{
    gvec ret;
    ret = g_vec_mul (v, f->scale);
    ret = g_vec3_rotate (ret, f->rotate);
    return g_vec_add (ret, f->offset);
}
X_INLINE_FUNC gvec
g_vec3_inv_affine       (const gvec     v,
                         const gxform   *f)
{
    gvec ret;
    ret = g_vec_sub (v, f->offset);
    ret = g_vec3_inv_rot(ret, f->rotate);
    return g_vec_div (ret, f->scale);
}
X_INLINE_FUNC gvec
g_vec3_xform            (const gvec     v,
                         const gmat3    *m)
{
#ifdef G_SSE_INTRINSICS
    gvec ret, tmp;
    ret = _mm_shuffle_ps(v,v,_MM_SHUFFLE(0,0,0,0));
    ret = _mm_mul_ps(ret, m->r[0]);

    tmp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(1,1,1,1));
    tmp = _mm_mul_ps(tmp, m->r[1]);
    ret = _mm_add_ps(ret, tmp);

    tmp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(2,2,2,2));
    tmp = _mm_mul_ps(tmp,m->r[2]);
    ret = _mm_add_ps(ret,tmp);

    return ret;
#else
    gvec ret;
    int i;
    for (i=0; i<3;++i)
        ret.f[i] = v.f[0]*m->m[0][i] + v.f[1]*m->m[1][i] + v.f[2]*m->m[2][i]; 
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec3_rand             (const gvec     dir,
                         const gvec     up,
                         const grad     angle)
{
    gquat q;
    q = g_quat_around (dir, g_real_rand(0, 1)*G_2PI);
    q = g_quat_around (g_vec3_rotate (up, q), angle);
    return g_vec3_rotate (dir, q);
}
#endif /* X_CAN_INLINE || __G_VEC3_C__ */

X_END_DECLS
#endif /* __G_VEC3_H__ */
