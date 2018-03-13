#ifndef __X_THREAD_PRV_H__
#define __X_THREAD_PRV_H__
#include "xtype.h"
X_BEGIN_DECLS
typedef enum {

} xThreadPriority;
struct  _xThread
{
    /*< private >*/
    xThreadFunc func;
    xptr data;
    xbool joinable;
    xThreadPriority priority;
    xint ref_count;
    xbool ours;
    xchar *name;
    xptr retval;
};

/* system thread implementation (xthread-posix.c, xthread-win32.c) */
X_INTERNAL_FUNC
void        x_system_thread_wait    (xThread        *thread);
X_INTERNAL_FUNC
xThread*    x_system_thread_new     (xThreadFunc    func,
                                     xulong         stack_size,
                                     xError         **error);
X_INTERNAL_FUNC
void        x_system_thread_free    (xThread        *thread);
X_INTERNAL_FUNC
void        x_system_thread_exit    (void);
X_INTERNAL_FUNC
void        x_system_thread_set_name(const xchar    *name);

/* gthread.c */
X_INTERNAL_FUNC
xThread*    x_thread_new_internal   (const xchar    *name,
                                      xThreadFunc   proxy,
                                      xThreadFunc   func,
                                      xptr          data,
                                      xsize         stack_size,
                                      xError        **error);

X_INTERNAL_FUNC
xptr        x_thread_proxy          (xptr           data);

X_END_DECLS
#endif /* __X_THREAD_PRV_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
