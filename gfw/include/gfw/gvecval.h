#ifndef __G_VALUE_H__
#define __G_VALUE_H__
#include "gtype.h"
X_BEGIN_DECLS


#define G_TYPE_VEC3  X_TYPE_MAKE_FUNDAMENTAL (X_TYPE_FUNDAMENTAL_USER)
#define G_TYPE_VEC4  X_TYPE_MAKE_FUNDAMENTAL (X_TYPE_FUNDAMENTAL_USER+1)
#define G_VALUE_HOLDS_VEC3(val)     X_VALUE_HOLDS(val, G_TYPE_VEC3)
#define G_VALUE_HOLDS_VEC4(val)     X_VALUE_HOLDS(val, G_TYPE_VEC4)

#define G_TYPE_PARAM_VEC3           (g_param_types()[0])
#define G_IS_PARAM_VEC3(pspec)      (X_INSTANCE_IS_TYPE ((pspec), G_TYPE_PARAM_VEC3))
#define G_PARAM_VEC3(pspec)         (X_INSTANCE_CAST((pspec), G_TYPE_PARAM_VEC3, gParamVec))

#define G_TYPE_PARAM_VEC4           (g_param_types()[1])
#define G_IS_PARAM_VEC4(pspec)      (X_INSTANCE_IS_TYPE ((pspec), G_TYPE_PARAM_VEC4))
#define G_PARAM_VEC4(pspec)         (X_INSTANCE_CAST((pspec), G_TYPE_PARAM_VEC4, gParamVec))

const xType*g_param_types           (void);

gvec        g_value_get_vec         (const xValue   *val);

void        g_value_set_vec         (xValue         *val,
                                     const gvec     vec);

xParam*     g_param_vec3_new        (xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     gvec           minimum,
                                     gvec           maximum,
                                     gvec           default_value,
                                     xuint          flags);

xParam*     g_param_vec4_new        (xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     gvec           minimum,
                                     gvec           maximum,
                                     gvec           default_value,
                                     xuint          flags);
struct _gParamVec 
{
    /*< private >*/
    xParam                  parent;
    xfloat                  minimum[4];
    xfloat                  maximum[4];
    xfloat                  default_value[4];
};

X_END_DECLS
#endif /* __G_VALUE_H__ */
