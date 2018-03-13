#ifndef __GML_TYPE_H__
#define __GML_TYPE_H__
#include <xos/xtype.h>

#ifdef X_HAVE_SSE_INTRINSICS
#include <xmmintrin.h>
#include <emmintrin.h>
#define G_SSE_INTRINSICS
#endif

#define G_VEC_ASEERT_ALIGN(p)   x_assert(((unsigned)p&0xF) == 0)
#define G_VEC_ALIGN             16
#define G_PI_2                  (1.570796327f)
#define G_PI                    (3.141592654f)
#define G_2PI                   (6.283185307f)
#define G_INV_PI                (1.0f / G_PI )
#define G_INV_2PI               (1.0f / G_2PI )
#define G_EPSILON               (1.192092896e-7f)
#define G_IS_NAN(x)             ((*(guint*)&(x) & 0x7F800000) == 0x7F800000 \
                                 && (*(guint*)&(x) & 0x7FFFFF) != 0)
#define G_IS_INF(x)             ((*(guint*)&(x) & 0x7FFFFFFF) == 0x7F800000)

#define G_DEG2RAD(degree)       ((degree) * (G_PI / 180.0))
#define G_RAD2DEG(radian)       ((radian) * (180.0 / G_PI))


X_BEGIN_DECLS

typedef xfloat                      greal;
typedef greal                       grad; /* radian */
typedef greal                       gdeg; /* degree */
typedef unsigned                    guint;

#ifdef G_SSE_INTRINSICS
typedef __m128                      gvec;
#else
typedef union {
    greal           f[4];
    guint           u[4];
} gvec;
#endif

typedef gvec                        gquat;
typedef gvec                        gcolor;
typedef gvec                        gplane;

typedef union _gmat3                gmat3;
typedef union _gmat4                gmat4;
typedef union _gveci                gveci;
typedef union _gvecf                gvecf;
typedef struct _gxform              gxform;
typedef struct _gray                gray;
typedef struct _gaabb               gaabb;
typedef struct _gsphere             gsphere;
typedef struct _gbox                gbox;
typedef struct _grect               grect;
typedef enum {
    G_LERP_SHORT_ROTATE = 1,
    G_LERP_SPHERICAL    = 2,
    G_LERP_SPLINE       = 4,
} gLerpFlags;

typedef enum {
    G_NO_SIDE,
    G_NEGATIVE_SIDE,
    G_POSITIVE_SIDE,
    G_BOTH_SIDE,
} gPlaneSide;

typedef enum {
    G_BOX_FRONT,
    G_BOX_BACK,
    G_BOX_LEFT,
    G_BOX_RIGHT,
    G_BOX_UP,
    G_BOX_DOWN,
    G_BOX_FACES,
    G_BOX_OUTSIDE = G_BOX_FACES,
    G_BOX_PARTIAL,
    G_BOX_INSIDE,
} gBoxSide;

union X_ALIGN(16) _gveci{
    guint           u[4];
    gvec            v;
};

union X_ALIGN(16) _gvecf{
    greal           f[4];
    gvec            v;
};

union X_ALIGN(16) _gmat3
{
    gvec            r[3];
    greal           m[3][4];
};

union X_ALIGN(16) _gmat4
{
    gvec            r[4];
    greal           m[4][4];
};

struct X_ALIGN(16) _gxform
{
    gvec            scale;
    gquat           rotate;
    gvec            offset;
};
struct X_ALIGN(16) _gray
{
    gvec    origin;
    gvec    direction;
};
struct X_ALIGN(16) _gaabb
{
    gvec    maximum;
    gvec    minimum;
    xint    extent;    /* 0 is null, <0 infinite, >0 finite */
};
struct X_ALIGN(16) _gsphere
{
    gvec    center;
    gvec    radius;
};
struct _gbox
{
    greal   left, right;
    greal   top, bottom;
    greal   front, back;
};

struct _grect
{
    greal   left, right;
    greal   top, bottom;
};

X_END_DECLS
#endif /* __GML_TYPE_H__ */
