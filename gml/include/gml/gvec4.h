#ifndef __G_VEC4_H__
#define __G_VEC4_H__
#include "gtype.h"

X_BEGIN_DECLS

#if defined (X_CAN_INLINE) || defined (__G_VEC4_C__)
X_INLINE_FUNC gvec
g_vec4_std              (const gvec     v)
{
#ifdef G_SSE_INTRINSICS
    return _mm_div_ps(v, _mm_shuffle_ps (v, v, _MM_SHUFFLE(3, 3, 3, 3)));
#else
    gvec ret;
    int i;
    for (i=0;i<4;++i)
        ret.f[i] = v.f[i]/v.f[3];
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec4_dot              (const gvec     v1,
                         const gvec     v2)
{
#ifdef G_SSE_INTRINSICS
    gvec tmp = v2;
    gvec dot = _mm_mul_ps(v1,v2);
    tmp = _mm_shuffle_ps(v2,dot,_MM_SHUFFLE(1,0,0,0));
    tmp = _mm_add_ps(tmp,dot);
    dot = _mm_shuffle_ps(dot,tmp,_MM_SHUFFLE(0,3,0,0));
    dot = _mm_add_ps(dot,tmp);
    return _mm_shuffle_ps(dot,dot,_MM_SHUFFLE(2,2,2,2));
#else
    greal dot = v1.f[0]*v2.f[0]+ v1.f[1]*v2.f[1]+ v1.f[2]*v2.f[2]+ v1.f[3]*v2.f[3];
    gvec ret={dot, dot, dot, dot};
    return ret;
#endif
}
X_INLINE_FUNC gvec
g_vec4_len_sq           (const gvec     v)
{
    return g_vec4_dot (v, v);
}
X_INLINE_FUNC gvec
g_vec4_norm             (const gvec     v)
{
#ifdef G_SSE_INTRINSICS
    gvec lensq, tmp, mask, ret;
    lensq = _mm_mul_ps (v, v);
    /* tmp=(z,w, z, w) */
    tmp = _mm_shuffle_ps(lensq,lensq,_MM_SHUFFLE(3,2,3,2));
    /* lensq=(x+z,y+w,z+z,w+w) */
    lensq = _mm_add_ps(lensq,tmp);
    /* lensq=(x+z,x+z,x+z,y+w) */
    lensq = _mm_shuffle_ps(lensq, lensq, _MM_SHUFFLE(1,0,0,0));
    /* tmp=(z,w,y+w,y+w)*/
    tmp = _mm_shuffle_ps(tmp,lensq,_MM_SHUFFLE(3,3,0,0));
    /* lensq=(?,?,x+z+y+w,?) */
    lensq = _mm_add_ps (lensq, tmp);
    /* splat the length */
    lensq = _mm_shuffle_ps (lensq, lensq, _MM_SHUFFLE(2,2,2,2));
    /* prepare for divison */
    ret = _mm_sqrt_ps (lensq);
    /* Test for a divide by zero (Must be FP to detect -0.0) */
    mask = _mm_setzero_ps ();
    mask = _mm_cmpneq_ps (mask, ret);
    ret = _mm_div_ps (v, ret);
    ret = _mm_and_ps (ret, mask);
    /* If the length is infinity, set the elements to zero */
    lensq = _mm_cmpneq_ps (lensq, G_VEC_INF);
    return _mm_and_ps (ret, lensq);
#else
    gvec ret;
    greal d = 0;
    int i;
    for (i =0;i<4;++i)
        d += v.f[i]*v.f[i];
    if (d < G_EPSILON)
        return G_VEC_0;
    d = g_real_rsqrt(d);
    for (i=0;i<4;++i)
        ret.f[i] = v.f[i]*d;
    return ret;
#endif
}

X_INLINE_FUNC gvec
g_vec4_xform            (const gvec     v,
                         const gmat4    *m)
{
#ifdef G_SSE_INTRINSICS
    gvec ret, tmp;
    ret = _mm_shuffle_ps(v,v,_MM_SHUFFLE(0,0,0,0));
    ret = _mm_mul_ps(ret, m->r[0]);

    tmp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(1,1,1,1));
    tmp = _mm_mul_ps(tmp, m->r[1]);
    ret = _mm_add_ps(ret, tmp);

    tmp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(2,2,2,2));
    tmp = _mm_mul_ps(tmp, m->r[2]);
    ret = _mm_add_ps(ret, tmp);

    tmp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(3,3,3,3));
    tmp = _mm_mul_ps(tmp, m->r[3]);
    ret = _mm_add_ps(ret, tmp);

    return ret;
#else
    gvec ret;
    int i;
    for (i=0; i<4;++i)
        ret.f[i] = v.f[0]*m->m[0][i] + v.f[1]*m->m[1][i] + v.f[2]*m->m[2][i] + v.f[3]*m->m[3][i]; 
    return ret;
#endif
}

#endif /* X_CAN_INLINE || __G_VEC4_C__ */

X_END_DECLS
#endif /* __G_VEC4_C__ */
