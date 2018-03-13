#ifndef __G_REAL_H__
#define __G_REAL_H__
#include <math.h>
#include <stdlib.h>
#include "gtype.h"
X_BEGIN_DECLS
#define g_real_aeq(x, y) (ABS(x,y) < G_EPSILON)
#define g_real_aeq0(x)  (ABS(x,0) < G_EPSILON)

greal       g_real_sin              (const grad     r);

greal       g_real_tan              (const grad     r);

X_INLINE_FUNC
greal       g_real_rsqrt            (const greal    r);
X_INLINE_FUNC
greal       g_real_sqrt             (const greal    r);
X_INLINE_FUNC
greal       g_real_cos              (const grad     r);
X_INLINE_FUNC
greal       g_real_mod              (const greal    x,
                                     const greal    f);
X_INLINE_FUNC
greal       g_real_rand             (const greal    start,
                                     const greal    end);
X_INLINE_FUNC
greal       g_real_lerp             (const greal    t,
                                     const greal    r1,
                                     const greal    r2);
X_INLINE_FUNC
greal       g_real_rlerp            (const greal    r1,
                                     const greal    r2);

#if defined (X_CAN_INLINE) || defined (__G_REAL_C__)
X_INLINE_FUNC greal
g_real_rsqrt            (const greal    r)
{
    return 1/sqrtf(r);
}
X_INLINE_FUNC greal
g_real_sqrt             (const greal    r)
{
    return sqrtf(r);
}
X_INLINE_FUNC greal
g_real_cos              (const grad     r)
{
    return g_real_sin(G_PI_2-r);
}

X_INLINE_FUNC greal
g_real_mod              (const greal    x,
                         const greal    y)
{
    return fmodf(x,y);
}
X_INLINE_FUNC greal
g_real_rand             (const greal    start,
                         const greal    end)
{
    return rand()*(end - start)/RAND_MAX + start;
}
X_INLINE_FUNC greal
g_real_lerp             (const greal    t,
                         const greal    r1,
                         const greal    r2)
{
    return r1 + t*(r2-r1);
}
X_INLINE_FUNC greal
g_real_rlerp            (const greal    r1,
                         const greal    r2)
{
    return g_real_lerp (g_real_rand(0, 1), r1, r2);
}
#endif /* X_CAN_INLINE || __G_REAL_C__ */

X_END_DECLS
#endif /* __G_REAL_H__ */
