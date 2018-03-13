#ifndef __X_THREAD_POOL_H__
#define __X_THREAD_POOL_H__
#include "xtype.h"
X_BEGIN_DECLS

#define x_thread_pool_new(func, user_data)\
    x_thread_pool_new_full(X_VOID_FUNC(func), user_data, -1, FALSE, NULL)

xThreadPool*    x_thread_pool_new_full  (xWorkFunc      func,
                                         xptr           user_data,
                                         xssize         max_threads,
                                         xbool          exclusive,
                                         xError         **error);

void            x_thread_pool_delete    (xThreadPool    *pool,
                                         xbool          immediate,
                                         xbool          wait_);

xbool           x_thread_pool_push      (xThreadPool    *pool,
                                         xptr           data,
                                         xError         **error);

X_END_DECLS
#endif /* __X_THREAD_POOL_H__ */
