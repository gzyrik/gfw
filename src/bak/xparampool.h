#ifndef __X_PARAM_POOL_H__
#define __X_PARAM_POOL_H__
#include "xtype.h"
X_BEGIN_DECLS
struct _xParamPool
{
    xMutex      mutex;
    xHashTbl    *hashtbl;
};
void        x_param_pool_insert     (xParamPool     *pool,
                                     xType          owner_type,
                                     xuint          n_params,
                                     xParam         *params);

void        x_param_pool_remove     (xParamPool     *pool,
                                     xuint          n_params,
                                     xParam         *params);

xParam*     x_param_pool_lookup     (xParamPool     *pool,
                                     xType          owner_type,
                                     xcstr          param_name,
                                     xbool          walk_ancestors);

X_END_DECLS
#endif /* __X_PARAM_PRV_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
