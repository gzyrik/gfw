#ifndef __G_XFORM_H__
#define __G_XFORM_H__
#include "gtype.h"

X_BEGIN_DECLS

X_INLINE_FUNC
gxform      g_xform_eye             (void);
X_INLINE_FUNC
gxform      g_xform_inv             (const gxform   *xf);
X_INLINE_FUNC
gxform      g_xform_mul             (const gxform   *xf1,
                                     const gxform   *xf2);
X_INLINE_FUNC
gxform      g_xform_lerp            (const greal    t,
                                     const gxform   *xf1,
                                     const gxform   *xf2,
                                     const xuint    flags);
#if defined (X_CAN_INLINE) || defined (__G_XFORM_C__)

X_INLINE_FUNC gxform
g_xform_eye        (void)
{
    gxform xform = {
        {1,1,1,1},
        {0,0,0,1},
        {0,0,0,0}
    };
    return xform;
}
X_INLINE_FUNC gxform
g_xform_inv             (const gxform   *xf)
{
    gxform ret;
    ret.offset = g_vec_negate (xf->offset);
    ret.scale = g_vec_inv (xf->scale);
    ret.rotate = g_quat_inv (xf->rotate);
    return ret;
}
X_INLINE_FUNC gxform
g_xform_mul             (const gxform   *xf1,
                         const gxform   *xf2)
{
    gxform ret;
    ret.scale = g_vec_mul (xf1->scale, xf2->scale);
    ret.rotate= g_quat_mul (xf1->rotate, xf2->rotate);
    ret.offset= g_vec_mul (ret.scale, xf2->offset);
    ret.offset= g_vec3_rotate(ret.offset,ret.rotate);
    ret.offset=g_vec_add (ret.offset, xf1->offset);
    return ret;
}
X_INLINE_FUNC gxform
g_xform_lerp            (const greal    t,
                         const gxform   *xf1,
                         const gxform   *xf2,
                         const xuint    flags)
{
    gxform ret;
    if (flags & G_LERP_SPLINE) {
    }
    else {
        if (flags & G_LERP_SPHERICAL)
            ret.rotate = g_quat_slerp (t,
                                       xf1->rotate,
                                       xf2->rotate,
                                       G_LERP_SHORT_ROTATE & flags);
        else
            ret.rotate = g_quat_nlerp (t,
                                       xf1->rotate,
                                       xf2->rotate,
                                       G_LERP_SHORT_ROTATE & flags);
        ret.offset = g_vec_lerp (g_vec(t,t,t,t), xf1->offset, xf2->offset);
        ret.scale = g_vec_lerp (g_vec(t,t,t,t), xf1->scale, xf2->scale);
    }
    return ret;
}

#endif /* X_CAN_INLINE || __G_XFORM_C__ */

X_END_DECLS
#endif /* __G_XFORM_H__ */
