#ifndef __G_QUAT_H__
#define __G_QUAT_H__
#include "gtype.h"

X_BEGIN_DECLS

X_INLINE_FUNC
gquat       g_quat_around           (const gvec     v,
                                     const grad     angle);
X_INLINE_FUNC
gquat       g_quat_betw_vec3        (const gvec     v1,
                                     const gvec     v2,
                                     const gvec     n);
X_INLINE_FUNC
gquat       g_quat_mat3             (const gmat3    *rot);
X_INLINE_FUNC
gquat       g_quat_axis             (const gvec     x_axis,
                                     const gvec     y_axis,
                                     const gvec     z_axis);
X_INLINE_FUNC
gquat       g_quat_norm             (const gquat    q);
X_INLINE_FUNC
gquat       g_quat_inv              (const gquat    q);
X_INLINE_FUNC
gquat       g_quat_conj             (const gquat    q);
/* q2 x q1, first rotation q1, then rotation q2 */
X_INLINE_FUNC
gquat       g_quat_mul              (const gquat    q1,
                                     const gquat    q2);
X_INLINE_FUNC
gquat       g_quat_nlerp            (const greal    t,
                                     const gquat    q1,
                                     const gquat    q2,
                                     xbool          shortest);
X_INLINE_FUNC
gquat       g_quat_slerp            (const greal    t,
                                     const gquat    q1,
                                     const gquat    q2,
                                     xbool          shortest);

#if defined (X_CAN_INLINE) || defined (__G_QUAT_C__)

X_INLINE_FUNC gquat
g_quat_around           (const gvec     v,
                         const grad     r)
{
    /* q = cos(r/2)+sin(r/2)*(v.x*i+v.y*j+v.z*k) */
#ifdef G_SSE_INTRINSICS
    gvec normal, angle, sine, cosi;
    normal = g_vec_mask_xyz1(v);
    angle = _mm_set_ps1(0.5f * r);
    g_vec_sincos (angle, &sine, &cosi);
    sine = g_vec_mask_xyz(sine);
    cosi = g_vec_mask_w(cosi);
    angle = _mm_or_ps(sine,cosi);
    return _mm_mul_ps(normal,angle);
#else
    gquat q;
    grad half;
    greal sine;
    half = r*0.5f;
    sine = sinf (half);
    q.f[0] = sine*v.f[0];
    q.f[1] = sine*v.f[1];
    q.f[2] = sine*v.f[2];
    q.f[3] = cosf (half);
    return q;
#endif
}
X_INLINE_FUNC gquat
g_quat_norm             (const gquat    q)
{
    return g_vec4_norm (q);
}
/** 从v1到v2的最近旋转,若v1==-v2,则n为旋转轴 */
X_INLINE_FUNC gquat
g_quat_betw_vec3        (const gvec     v1,
                         const gvec     v2,
                         const gvec     n)
{
    gvec half,d,c;
    d = g_vec3_norm(v1);
    c = g_vec3_norm(v2);
    half = g_vec3_norm(g_vec_add(d, c));
    c = g_vec3_cross(d,half);
    d = g_vec3_dot (d, half);
    c = g_vec_mask_xyz (c);
    d = g_vec_mask_w (d);
    return g_vec_add (c,d);
}
X_INLINE_FUNC gquat
g_quat_mat3             (const gmat3    *rot)
{
#ifdef G_SSE_INTRINSICS
#else
    /* Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
     * article "Quaternion Calculus and Fast Animation".
     */
    gquat ret;
    greal fTrace = rot->m[0][0]+rot->m[1][1]+rot->m[2][2];
    greal fRoot;

    if ( fTrace > 0.0f ) {
        /* |w| > 1/2, may as well choose w > 1/2 */
        fRoot =  g_real_sqrt(fTrace + 1.0f);
        ret.f[3] = 0.5f*fRoot;
        fRoot = 0.5f/fRoot;  // 1/(4w)
        ret.f[0] = (rot->m[2][1]-rot->m[1][2])*fRoot;
        ret.f[1] = (rot->m[0][2]-rot->m[2][0])*fRoot;
        ret.f[2] = (rot->m[1][0]-rot->m[0][1])*fRoot;
    }
    else
    {
        /* |w| <= 1/2 */
        static int s_iNext[3] = { 1, 2, 0 };
        int i = 0, j, k;
        if ( rot->m[1][1] > rot->m[0][0] )
            i = 1;
        if ( rot->m[2][2] > rot->m[i][i] )
            i = 2;
        j = s_iNext[i];
        k = s_iNext[j];

        fRoot = g_real_sqrt(rot->m[i][i]-rot->m[j][j]-rot->m[k][k] + 1.0f);
        ret.f[i] = 0.5f*fRoot;
        fRoot = 0.5f/fRoot;
        ret.f[3] = (rot->m[k][j]-rot->m[j][k])*fRoot;
        ret.f[j] = (rot->m[j][i]+rot->m[i][j])*fRoot;
        ret.f[k] = (rot->m[k][i]+rot->m[i][k])*fRoot;
    }
    return ret;
#endif
}

X_INLINE_FUNC gquat
g_quat_axis             (const gvec     x_axis,
                         const gvec     y_axis,
                         const gvec     z_axis)
{
    gmat3 mat;
    mat.r[0] = x_axis;
    mat.r[1] = y_axis;
    mat.r[2] = z_axis;
    return g_quat_mat3(&mat);
}
X_INLINE_FUNC gquat
g_quat_inv              (const gquat    q)
{
#ifdef G_SSE_INTRINSICS
    gvec len, conj, ctrl, ret;
    len = g_vec4_len_sq (q);
    conj = g_quat_conj (q);
    ctrl = _mm_cmple_ps (len, G_VEC_EPSILON);
    ret = _mm_div_ps (conj, len);
    return g_vec_select (ret, G_VEC_0, ctrl);
#else
    gvec ret;
    greal inv=0;
    int i;
    for (i=0;i<4;++i)
        inv += q.f[i] * q.f[i];
    if (inv <= G_EPSILON)
        return G_VEC_0;
    inv = g_real_rsqrt(inv);
    ret.f[0] = -q.f[0]*inv;
    ret.f[1] = -q.f[1]*inv;
    ret.f[2] = -q.f[2]*inv;
    ret.f[3] =  q.f[3]*inv;
    return ret;
#endif
}
X_INLINE_FUNC gquat
g_quat_conj             (const gquat    q)
{
#ifdef G_SSE_INTRINSICS
    return g_vec_flip_xyz(q);
#else
    gvec ret={-q.f[0],-q.f[1],-q.f[2],q.f[3]};
    return ret;
#endif
}
X_INLINE_FUNC gquat
g_quat_mul              (const gquat    q1,
                         const gquat    q2)
{
#ifdef G_SSE_INTRINSICS
    gvec ret, q2x, q2y, q2z,q1s;
    q2x = q2y = q2z = q2;
    q1s = q1;
    ret = q2;

    ret = _mm_shuffle_ps(ret,ret,_MM_SHUFFLE(3,3,3,3));
    q2x = _mm_shuffle_ps(q2x,q2x,_MM_SHUFFLE(0,0,0,0));
    q2y = _mm_shuffle_ps(q2y,q2y,_MM_SHUFFLE(1,1,1,1));
    q2z = _mm_shuffle_ps(q2z,q2z,_MM_SHUFFLE(2,2,2,2));

    /* [col 0] q2.w = q2.w * q1.xyzw */
    ret = _mm_mul_ps(ret,q1);

    /* [col 1] q2.x = q2.x * q1.wzyx, flip y,w */
    q1s = _mm_shuffle_ps(q1s,q1s,_MM_SHUFFLE(0,1,2,3));
    q2x = _mm_mul_ps(q2x,q1s);
    q2x = g_vec_flip_yw(q2x);

    /* [col 2] q2.y = q2.y * q1.zwxy, flip z,w */
    q1s = _mm_shuffle_ps(q1s,q1s,_MM_SHUFFLE(2,3,0,1));
    q2y = _mm_mul_ps(q2y,q1s);
    q2y = g_vec_flip_zw(q2y);

    /* [col 3] q2.z = q2.z * q1.yxwz, flip x,w*/
    q1s = _mm_shuffle_ps(q1s,q1s,_MM_SHUFFLE(0,1,2,3));
    q2z = _mm_mul_ps(q2z,q1s);
    q2z = g_vec_flip_xw(q2z);

    /* sum [col 0-3] */
    ret = _mm_add_ps(ret,q2x);
    ret = _mm_add_ps(ret,q2y);
    ret = _mm_add_ps(ret,q2z);
    return ret;
#else
    gvec ret;
    ret.f[0]=q2.f[3] * q1.f[0] + q2.f[0] * q1.f[3] + q2.f[1] * q1.f[2] - q2.f[2] * q1.f[1];
    ret.f[1]=q2.f[3] * q1.f[1] - q2.f[0] * q1.f[2] + q2.f[1] * q1.f[3] + q2.f[2] * q1.f[0];
    ret.f[2]=q2.f[3] * q1.f[2] + q2.f[0] * q1.f[1] - q2.f[1] * q1.f[0] + q2.f[2] * q1.f[3];
    ret.f[3]=q2.f[3] * q1.f[3] - q2.f[0] * q1.f[0] - q2.f[1] * q1.f[1] - q2.f[2] * q1.f[2];
    return ret;
#endif
}
X_INLINE_FUNC gquat
g_quat_nlerp            (const greal    t,
                         const gquat    q1,
                         const gquat    q2,
                         xbool          shortest)
{
    gquat ret;
    gvec fcos = g_vec4_dot(q1, q2);
    if (g_vec_le_s (fcos, G_VEC_0) && shortest)
        ret = g_vec_negate (q2);
    else
        ret = q2;
    ret = g_vec_sub(ret, q1);
    ret = g_vec_scale (ret, t);
    ret = g_vec_add (ret, q1);
    return g_quat_norm(ret);
}

X_INLINE_FUNC gquat
g_quat_slerp            (const greal    t,
                         const gquat    q1,
                         const gquat    q2,
                         xbool          shortest)
{
    return g_vec(0,0,0,0);
}

#endif /* X_CAN_INLINE || __G_QUAT_C__ */

X_END_DECLS
#endif /* __G_QUAT_H__ */
