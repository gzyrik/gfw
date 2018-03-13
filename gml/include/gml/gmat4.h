#ifndef __G_MAT4_H__
#define __G_MAT4_H__
#include "gtype.h"

X_BEGIN_DECLS

X_INLINE_FUNC
gmat4       g_mat4_eye              (void);
X_INLINE_FUNC
gmat4       g_mat4_scale            (const gvec     scale);
X_INLINE_FUNC
gmat4       g_mat4_rotate           (const gquat    rotate);
X_INLINE_FUNC
gmat4       g_mat4_offset           (const gvec     offset);
X_INLINE_FUNC
gmat4       g_mat4_trans            (const gmat4    *m);
X_INLINE_FUNC
gmat4       g_mat4_inv              (const gmat4    *m);
X_INLINE_FUNC
gmat4       g_mat4_mul              (const gmat4    *m1,
                                     const gmat4    *m2);
X_INLINE_FUNC
gmat4       g_mat4_affine           (const gvec     scale,
                                     const gquat    rotate,
                                     const gvec     offset);
X_INLINE_FUNC
gmat4       g_mat4_xform            (const gxform   *f);
X_INLINE_FUNC
gmat4       g_mat4_ortho            (const gbox     *box);
X_INLINE_FUNC
gmat4       g_mat4_persp            (const gbox     *box);
X_INLINE_FUNC
gmat4       g_mat4_view             (const gvec     pos,
                                     const gquat    rotate);

#if defined (X_CAN_INLINE) || defined (__G_MAT4_C__)

X_INLINE_FUNC gmat4
g_mat4_eye         (void)
{
    gmat4 ret={
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    return ret;
}
X_INLINE_FUNC gmat4
g_mat4_offset           (const gvec     v)
{
    gmat4 ret = g_mat4_eye ();
#ifdef G_SSE_INTRINSICS
    ret.r[3] = g_vec_mask_xyz1(v);
#else
    ret.r[3] = v;
    ret.r[3].f[3] = 1;
#endif
    return ret;
}
X_INLINE_FUNC gmat4
g_mat4_scale            (const gvec     s)
{
#ifdef G_SSE_INTRINSICS
    gmat4 ret;
    ret.r[0] = g_vec_mask_x (s);
    ret.r[1] = g_vec_mask_y (s);
    ret.r[2] = g_vec_mask_z (s);
    ret.r[3] = _mm_set_ss(1);
    return ret;
#else
    gmat4 ret={s.f[0],0,0,0,0,s.f[1],0,0,0,0,s.f[2],0,0,0,0,1};
    return ret;
#endif
}
X_INLINE_FUNC gmat4
g_mat4_rotate           (const gquat    q)
{
#ifdef G_SSE_INTRINSICS
    gmat4 ret;
    gvec q0, q1, v0, v1, v2, r0,r1,r2;

    q0 = _mm_add_ps (q, q);
    q1 = _mm_mul_ps (q, q0);

    v0 = _mm_shuffle_ps (q1,q1,_MM_SHUFFLE(3,0,0,1));
    v0 = g_vec_mask_xyz (v0);
    v1 = _mm_shuffle_ps (q1,q1,_MM_SHUFFLE(3,1,2,2));
    v1 = g_vec_mask_xyz (v1);

    r0 = _mm_sub_ps(_mm_set_ps(0,1,1,1),v0);
    r0 = _mm_sub_ps(r0, v1);

    v0 = _mm_shuffle_ps (q, q, _MM_SHUFFLE(3,1,0,0));
    v1 = _mm_shuffle_ps (q0,q0,_MM_SHUFFLE(3,2,1,2));
    v0 = _mm_mul_ps(v0, v1);

    v1 = _mm_shuffle_ps (q,q,_MM_SHUFFLE(3,3,3,3));
    v2 = _mm_shuffle_ps(q0,q0,_MM_SHUFFLE(3,0,2,1));
    v1 = _mm_mul_ps(v1, v2);

    r1 = _mm_add_ps(v0, v1);
    r2 = _mm_sub_ps(v0, v1);

    v0 = _mm_shuffle_ps(r1,r2,_MM_SHUFFLE(1,0,2,1));
    v0 = _mm_shuffle_ps(v0,v0,_MM_SHUFFLE(1,3,2,0));
    v1 = _mm_shuffle_ps(r1,r2,_MM_SHUFFLE(2,2,0,0));
    v1 = _mm_shuffle_ps(v1,v1,_MM_SHUFFLE(2,0,2,0));

    q1 = _mm_shuffle_ps(r0,v0,_MM_SHUFFLE(1,0,3,0));
    q1 = _mm_shuffle_ps(q1,q1,_MM_SHUFFLE(1,3,2,0));
    ret.r[0] = q1;
    q1 = _mm_shuffle_ps(r0,v0,_MM_SHUFFLE(3,2,3,1));
    q1 = _mm_shuffle_ps(q1,q1,_MM_SHUFFLE(1,3,0,2));
    ret.r[1] = q1;
    q1 = _mm_shuffle_ps(v1,r0,_MM_SHUFFLE(3,2,1,0));
    ret.r[2] = q1;
    ret.r[3] = _mm_set_ss(1);
    return ret;
#else
    greal fTx  = q.f[0] + q.f[0];
    greal fTy  = q.f[1] + q.f[1];
    greal fTz  = q.f[2] + q.f[2];
    greal fTxx = fTx*q.f[0];
    greal fTyy = fTy*q.f[1];
    greal fTzz = fTz*q.f[2];
    greal fTwx = fTx*q.f[3];
    greal fTwy = fTy*q.f[3];
    greal fTwz = fTz*q.f[3];
    greal fTxy = fTy*q.f[0];
    greal fTxz = fTz*q.f[0];
    greal fTyz = fTz*q.f[1];
    gmat4 ret = {
        1.0f-(fTyy+fTzz), fTxy+fTwz,fTxz-fTwy,0,
        fTxy-fTwz, 1.0f-(fTxx+fTzz), fTyz+fTwx,0,
        fTxz+fTwy, fTyz-fTwx, 1.0f-(fTxx+fTyy),0,
        0,0,0,1
    };
    return ret;
#endif
}
X_INLINE_FUNC gmat4
g_mat4_trans        (const gmat4    *m)
{
#ifdef G_SSE_INTRINSICS
    gmat4 ret;
    gvec vTemp1, vTemp2, vTemp3, vTemp4;
    vTemp1 = _mm_shuffle_ps(m->r[0],m->r[1],_MM_SHUFFLE(1,0,1,0));
    vTemp3 = _mm_shuffle_ps(m->r[0],m->r[1],_MM_SHUFFLE(3,2,3,2));
    vTemp2 = _mm_shuffle_ps(m->r[2],m->r[3],_MM_SHUFFLE(1,0,1,0));
    vTemp4 = _mm_shuffle_ps(m->r[2],m->r[3],_MM_SHUFFLE(3,2,3,2));
    ret.r[0] = _mm_shuffle_ps(vTemp1, vTemp2,_MM_SHUFFLE(2,0,2,0));
    ret.r[1] = _mm_shuffle_ps(vTemp1, vTemp2,_MM_SHUFFLE(3,1,3,1));
    ret.r[2] = _mm_shuffle_ps(vTemp3, vTemp4,_MM_SHUFFLE(2,0,2,0));
    ret.r[3] = _mm_shuffle_ps(vTemp3, vTemp4,_MM_SHUFFLE(3,1,3,1));
    return ret;
#else
    gmat4 ret = {
        m->m[0][0], m->m[1][0], m->m[2][0], m->m[3][0],
        m->m[0][1], m->m[1][1], m->m[2][1], m->m[3][1],
        m->m[0][2], m->m[1][2], m->m[2][2], m->m[3][2],
        m->m[0][3], m->m[1][3], m->m[2][3], m->m[3][3]};
    return ret;
#endif
}
X_INLINE_FUNC gmat4
g_mat4_inv          (const gmat4    *m)
{
#ifdef G_SSE_INTRINSICS
    gmat4 MT, ret;
    gvec V00,V10,V01,V11,V02,V12,V13,V03;
    gvec D0, D1, D2;
    gvec C0,C1,C2,C3,C4,C5,C6,C7;
    gvec det, vTemp;

    MT = g_mat4_trans(m);
    V00 = _mm_shuffle_ps(MT.r[2], MT.r[2],_MM_SHUFFLE(1,1,0,0));
    V10 = _mm_shuffle_ps(MT.r[3], MT.r[3],_MM_SHUFFLE(3,2,3,2));
    V01 = _mm_shuffle_ps(MT.r[0], MT.r[0],_MM_SHUFFLE(1,1,0,0));
    V11 = _mm_shuffle_ps(MT.r[1], MT.r[1],_MM_SHUFFLE(3,2,3,2));
    V02 = _mm_shuffle_ps(MT.r[2], MT.r[0],_MM_SHUFFLE(2,0,2,0));
    V12 = _mm_shuffle_ps(MT.r[3], MT.r[1],_MM_SHUFFLE(3,1,3,1));

    D0 = _mm_mul_ps(V00,V10);
    D1 = _mm_mul_ps(V01,V11);
    D2 = _mm_mul_ps(V02,V12);

    V00 = _mm_shuffle_ps(MT.r[2],MT.r[2],_MM_SHUFFLE(3,2,3,2));
    V10 = _mm_shuffle_ps(MT.r[3],MT.r[3],_MM_SHUFFLE(1,1,0,0));
    V01 = _mm_shuffle_ps(MT.r[0],MT.r[0],_MM_SHUFFLE(3,2,3,2));
    V11 = _mm_shuffle_ps(MT.r[1],MT.r[1],_MM_SHUFFLE(1,1,0,0));
    V02 = _mm_shuffle_ps(MT.r[2],MT.r[0],_MM_SHUFFLE(3,1,3,1));
    V12 = _mm_shuffle_ps(MT.r[3],MT.r[1],_MM_SHUFFLE(2,0,2,0));

    V00 = _mm_mul_ps(V00,V10);
    V01 = _mm_mul_ps(V01,V11);
    V02 = _mm_mul_ps(V02,V12);
    D0 = _mm_sub_ps(D0,V00);
    D1 = _mm_sub_ps(D1,V01);
    D2 = _mm_sub_ps(D2,V02);
    // V11 = D0Y,D0W,D2Y,D2Y
    V11 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(1,1,3,1));
    V00 = _mm_shuffle_ps(MT.r[1], MT.r[1],_MM_SHUFFLE(1,0,2,1));
    V10 = _mm_shuffle_ps(V11,D0,_MM_SHUFFLE(0,3,0,2));
    V01 = _mm_shuffle_ps(MT.r[0], MT.r[0],_MM_SHUFFLE(0,1,0,2));
    V11 = _mm_shuffle_ps(V11,D0,_MM_SHUFFLE(2,1,2,1));
    // V13 = D1Y,D1W,D2W,D2W
    V13 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(3,3,3,1));
    V02 = _mm_shuffle_ps(MT.r[3], MT.r[3],_MM_SHUFFLE(1,0,2,1));
    V12 = _mm_shuffle_ps(V13,D1,_MM_SHUFFLE(0,3,0,2));
    V03 = _mm_shuffle_ps(MT.r[2], MT.r[2],_MM_SHUFFLE(0,1,0,2));
    V13 = _mm_shuffle_ps(V13,D1,_MM_SHUFFLE(2,1,2,1));

    C0 = _mm_mul_ps(V00,V10);
    C2 = _mm_mul_ps(V01,V11);
    C4 = _mm_mul_ps(V02,V12);
    C6 = _mm_mul_ps(V03,V13);

    // V11 = D0X,D0Y,D2X,D2X
    V11 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(0,0,1,0));
    V00 = _mm_shuffle_ps(MT.r[1], MT.r[1],_MM_SHUFFLE(2,1,3,2));
    V10 = _mm_shuffle_ps(D0,V11,_MM_SHUFFLE(2,1,0,3));
    V01 = _mm_shuffle_ps(MT.r[0], MT.r[0],_MM_SHUFFLE(1,3,2,3));
    V11 = _mm_shuffle_ps(D0,V11,_MM_SHUFFLE(0,2,1,2));
    // V13 = D1X,D1Y,D2Z,D2Z
    V13 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(2,2,1,0));
    V02 = _mm_shuffle_ps(MT.r[3], MT.r[3],_MM_SHUFFLE(2,1,3,2));
    V12 = _mm_shuffle_ps(D1,V13,_MM_SHUFFLE(2,1,0,3));
    V03 = _mm_shuffle_ps(MT.r[2], MT.r[2],_MM_SHUFFLE(1,3,2,3));
    V13 = _mm_shuffle_ps(D1,V13,_MM_SHUFFLE(0,2,1,2));

    V00 = _mm_mul_ps(V00,V10);
    V01 = _mm_mul_ps(V01,V11);
    V02 = _mm_mul_ps(V02,V12);
    V03 = _mm_mul_ps(V03,V13);
    C0 = _mm_sub_ps(C0,V00);
    C2 = _mm_sub_ps(C2,V01);
    C4 = _mm_sub_ps(C4,V02);
    C6 = _mm_sub_ps(C6,V03);

    V00 = _mm_shuffle_ps(MT.r[1],MT.r[1],_MM_SHUFFLE(0,3,0,3));
    // V10 = D0Z,D0Z,D2X,D2Y
    V10 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(1,0,2,2));
    V10 = _mm_shuffle_ps(V10,V10,_MM_SHUFFLE(0,2,3,0));
    V01 = _mm_shuffle_ps(MT.r[0],MT.r[0],_MM_SHUFFLE(2,0,3,1));
    // V11 = D0X,D0W,D2X,D2Y
    V11 = _mm_shuffle_ps(D0,D2,_MM_SHUFFLE(1,0,3,0));
    V11 = _mm_shuffle_ps(V11,V11,_MM_SHUFFLE(2,1,0,3));
    V02 = _mm_shuffle_ps(MT.r[3],MT.r[3],_MM_SHUFFLE(0,3,0,3));
    // V12 = D1Z,D1Z,D2Z,D2W
    V12 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(3,2,2,2));
    V12 = _mm_shuffle_ps(V12,V12,_MM_SHUFFLE(0,2,3,0));
    V03 = _mm_shuffle_ps(MT.r[2],MT.r[2],_MM_SHUFFLE(2,0,3,1));
    // V13 = D1X,D1W,D2Z,D2W
    V13 = _mm_shuffle_ps(D1,D2,_MM_SHUFFLE(3,2,3,0));
    V13 = _mm_shuffle_ps(V13,V13,_MM_SHUFFLE(2,1,0,3));

    V00 = _mm_mul_ps(V00,V10);
    V01 = _mm_mul_ps(V01,V11);
    V02 = _mm_mul_ps(V02,V12);
    V03 = _mm_mul_ps(V03,V13);
    C1 = _mm_sub_ps(C0,V00);
    C0 = _mm_add_ps(C0,V00);
    C3 = _mm_add_ps(C2,V01);
    C2 = _mm_sub_ps(C2,V01);
    C5 = _mm_sub_ps(C4,V02);
    C4 = _mm_add_ps(C4,V02);
    C7 = _mm_add_ps(C6,V03);
    C6 = _mm_sub_ps(C6,V03);

    C0 = _mm_shuffle_ps(C0,C1,_MM_SHUFFLE(3,1,2,0));
    C2 = _mm_shuffle_ps(C2,C3,_MM_SHUFFLE(3,1,2,0));
    C4 = _mm_shuffle_ps(C4,C5,_MM_SHUFFLE(3,1,2,0));
    C6 = _mm_shuffle_ps(C6,C7,_MM_SHUFFLE(3,1,2,0));
    C0 = _mm_shuffle_ps(C0,C0,_MM_SHUFFLE(3,1,2,0));
    C2 = _mm_shuffle_ps(C2,C2,_MM_SHUFFLE(3,1,2,0));
    C4 = _mm_shuffle_ps(C4,C4,_MM_SHUFFLE(3,1,2,0));
    C6 = _mm_shuffle_ps(C6,C6,_MM_SHUFFLE(3,1,2,0));
    /* Get the determinate */
    det = g_vec4_dot (C0,MT.r[0]);
    vTemp = _mm_div_ps(_mm_set_ss(1),det);
    ret.r[0] = _mm_mul_ps(C0,vTemp);
    ret.r[1] = _mm_mul_ps(C2,vTemp);
    ret.r[2] = _mm_mul_ps(C4,vTemp);
    ret.r[3] = _mm_mul_ps(C6,vTemp);
    return ret;
#else
    gmat4 ret;
    greal m00 = m->m[0][0], m01 = m->m[0][1], m02 = m->m[0][2], m03 = m->m[0][3];
    greal m10 = m->m[1][0], m11 = m->m[1][1], m12 = m->m[1][2], m13 = m->m[1][3];
    greal m20 = m->m[2][0], m21 = m->m[2][1], m22 = m->m[2][2], m23 = m->m[2][3];
    greal m30 = m->m[3][0], m31 = m->m[3][1], m32 = m->m[3][2], m33 = m->m[3][3];

    greal v0 = m20 * m31 - m21 * m30;
    greal v1 = m20 * m32 - m22 * m30;
    greal v2 = m20 * m33 - m23 * m30;
    greal v3 = m21 * m32 - m22 * m31;
    greal v4 = m21 * m33 - m23 * m31;
    greal v5 = m22 * m33 - m23 * m32;

    greal t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
    greal t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
    greal t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
    greal t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

    greal invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

    ret.m[0][0] = t00 * invDet;
    ret.m[1][0] = t10 * invDet;
    ret.m[2][0] = t20 * invDet;
    ret.m[3][0] = t30 * invDet;

    ret.m[0][1] = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    ret.m[1][1] = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    ret.m[2][1] = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    ret.m[3][1] = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m10 * m31 - m11 * m30;
    v1 = m10 * m32 - m12 * m30;
    v2 = m10 * m33 - m13 * m30;
    v3 = m11 * m32 - m12 * m31;
    v4 = m11 * m33 - m13 * m31;
    v5 = m12 * m33 - m13 * m32;

    ret.m[0][2] = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    ret.m[1][2] = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    ret.m[2][2] = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    ret.m[3][2] = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m21 * m10 - m20 * m11;
    v1 = m22 * m10 - m20 * m12;
    v2 = m23 * m10 - m20 * m13;
    v3 = m22 * m11 - m21 * m12;
    v4 = m23 * m11 - m21 * m13;
    v5 = m23 * m12 - m22 * m13;

    ret.m[0][3] = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    ret.m[1][3] = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    ret.m[2][3] = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    ret.m[3][3] = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;
    return ret;
#endif
}
X_INLINE_FUNC gmat4
g_mat4_mul        (const gmat4    *m1,
                   const gmat4    *m2)
{
#ifdef G_SSE_INTRINSICS
    gmat4 ret;
    gvec vX, vY, vZ, vW;
    /* r[0], splat x,y,z,w */
    vW = m1->r[0];
    vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    /* r[0] * colomn of m2 */
    vX = _mm_mul_ps(vX,m2->r[0]);
    vY = _mm_mul_ps(vY,m2->r[1]);
    vZ = _mm_mul_ps(vZ,m2->r[2]);
    vW = _mm_mul_ps(vW,m2->r[3]);
    /* sum them, finish ret.r[0] */
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    ret.r[0] = vX;
    /* repeat for other 3 rows */
    vW = m1->r[1];
    vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,m2->r[0]);
    vY = _mm_mul_ps(vY,m2->r[1]);
    vZ = _mm_mul_ps(vZ,m2->r[2]);
    vW = _mm_mul_ps(vW,m2->r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    ret.r[1] = vX;
    vW = m1->r[2];
    vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,m2->r[0]);
    vY = _mm_mul_ps(vY,m2->r[1]);
    vZ = _mm_mul_ps(vZ,m2->r[2]);
    vW = _mm_mul_ps(vW,m2->r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    ret.r[2] = vX;
    vW = m1->r[3];
    vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,m2->r[0]);
    vY = _mm_mul_ps(vY,m2->r[1]);
    vZ = _mm_mul_ps(vZ,m2->r[2]);
    vW = _mm_mul_ps(vW,m2->r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    ret.r[3] = vX;
    return ret;
#else
    gmat4 ret = {
        m1->m[0][0] * m2->m[0][0] + m1->m[0][1] * m2->m[1][0] + m1->m[0][2] * m2->m[2][0] + m1->m[0][3] * m2->m[3][0],
        m1->m[0][0] * m2->m[0][1] + m1->m[0][1] * m2->m[1][1] + m1->m[0][2] * m2->m[2][1] + m1->m[0][3] * m2->m[3][1],
        m1->m[0][0] * m2->m[0][2] + m1->m[0][1] * m2->m[1][2] + m1->m[0][2] * m2->m[2][2] + m1->m[0][3] * m2->m[3][2],
        m1->m[0][0] * m2->m[0][3] + m1->m[0][1] * m2->m[1][3] + m1->m[0][2] * m2->m[2][3] + m1->m[0][3] * m2->m[3][3],

        m1->m[1][0] * m2->m[0][0] + m1->m[1][1] * m2->m[1][0] + m1->m[1][2] * m2->m[2][0] + m1->m[1][3] * m2->m[3][0],
        m1->m[1][0] * m2->m[0][1] + m1->m[1][1] * m2->m[1][1] + m1->m[1][2] * m2->m[2][1] + m1->m[1][3] * m2->m[3][1],
        m1->m[1][0] * m2->m[0][2] + m1->m[1][1] * m2->m[1][2] + m1->m[1][2] * m2->m[2][2] + m1->m[1][3] * m2->m[3][2],
        m1->m[1][0] * m2->m[0][3] + m1->m[1][1] * m2->m[1][3] + m1->m[1][2] * m2->m[2][3] + m1->m[1][3] * m2->m[3][3],

        m1->m[2][0] * m2->m[0][0] + m1->m[2][1] * m2->m[1][0] + m1->m[2][2] * m2->m[2][0] + m1->m[2][3] * m2->m[3][0],
        m1->m[2][0] * m2->m[0][1] + m1->m[2][1] * m2->m[1][1] + m1->m[2][2] * m2->m[2][1] + m1->m[2][3] * m2->m[3][1],
        m1->m[2][0] * m2->m[0][2] + m1->m[2][1] * m2->m[1][2] + m1->m[2][2] * m2->m[2][2] + m1->m[2][3] * m2->m[3][2],
        m1->m[2][0] * m2->m[0][3] + m1->m[2][1] * m2->m[1][3] + m1->m[2][2] * m2->m[2][3] + m1->m[2][3] * m2->m[3][3],

        m1->m[3][0] * m2->m[0][0] + m1->m[3][1] * m2->m[1][0] + m1->m[3][2] * m2->m[2][0] + m1->m[3][3] * m2->m[3][0],
        m1->m[3][0] * m2->m[0][1] + m1->m[3][1] * m2->m[1][1] + m1->m[3][2] * m2->m[2][1] + m1->m[3][3] * m2->m[3][1],
        m1->m[3][0] * m2->m[0][2] + m1->m[3][1] * m2->m[1][2] + m1->m[3][2] * m2->m[2][2] + m1->m[3][3] * m2->m[3][2],
        m1->m[3][0] * m2->m[0][3] + m1->m[3][1] * m2->m[1][3] + m1->m[3][2] * m2->m[2][3] + m1->m[3][3] * m2->m[3][3]
    };
    return ret;
#endif
}
X_INLINE_FUNC gmat4
g_mat4_affine           (const gvec     scale,
                         const gquat    rotate,
                         const gvec     offset)
{
    gmat4 ret, rot;

    ret = g_mat4_scale (scale);
    rot = g_mat4_rotate (rotate);
    ret = g_mat4_mul(&ret, &rot);
    ret.r[3] = g_vec_mask_xyz1(offset);
    return ret;
}
X_INLINE_FUNC gmat4
g_mat4_xform            (const gxform   *f)
{
    return g_mat4_affine (f->scale, f->rotate, f->offset);
}
X_INLINE_FUNC gmat4
g_mat4_ortho            (const gbox     *box)
{
    gmat4 ret;
    greal inv_w = 1 / (box->right-box->left);
    greal inv_h = 1 / (box->top-box->bottom);
    greal inv_d = -1 / (box->back - box->front);
    greal qn = (box->back * box->front) * inv_d;
    gvec ABq = g_vec (inv_w*2, inv_h*2, inv_d*2, 0);

    ret = g_mat4_scale(ABq);
    ret.m[3][0] = - (box->right + box->left) * inv_w;
    ret.m[3][1] = - (box->top + box->bottom) * inv_h;
    ret.m[3][2] = qn;
    return ret;
}
X_INLINE_FUNC gmat4
g_mat4_persp      (const gbox     *box)
{
    gmat4 ret;
    greal nearZ2= box->front + box->front;
    greal inv_w = 1 / (box->right-box->left);
    greal inv_h = 1 / (box->top-box->bottom);
    greal inv_d = 1 / (box->back - box->front);
    greal q = - (box->back + box->front) * inv_d;
    gvec ABqn = g_vec(nearZ2 * inv_w,
                      nearZ2 * inv_h,
                      -2 * (box->back * box->front) * inv_d,
                      0);
    /* ret.r[0].f[0] = A */
    ret.r[0] = g_vec_mask_x(ABqn);
    /* ret.r[1].f[1] = B */
    ret.r[1] = g_vec_mask_y(ABqn);
    ret.r[3] = g_vec_mask_z(ABqn);

    ret.r[2] = g_vec((box->left + box->right) * inv_w,
                     (box->top + box->bottom) * inv_h,
                     q,
                     -1);
    return ret;
}
X_INLINE_FUNC gmat4
g_mat4_view             (const gvec     pos,
                         const gquat    rotate)
{
    gvec r3;
    gmat4 ret;
    ret = g_mat4_rotate (rotate);
    ret = g_mat4_trans (&ret);
    r3 = g_vec3_xform (pos, (gmat3*)&ret);
    r3 = g_vec_negate(r3);
    ret.r[3] = g_vec_mask_xyz1(r3);

    return ret;
}
#endif /* X_CAN_INLINE || __G_MAT4_C__ */

X_END_DECLS
#endif /* __G_MAT4_H__ */
