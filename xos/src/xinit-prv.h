#ifndef __X_INIT_PRV_H__
#define __X_INIT_PRV_H__
#include "xmsg.h"
#include "xinit.h"
X_BEGIN_DECLS

#ifdef X_OS_WINDOWS

X_INTERN_FUNC
void        _x_thread_win32_detach  (void);

X_INTERN_FUNC
void        _x_thread_win32_init    (void);

X_INTERN_FUNC
void        _x_clock_win32_init     (void);

X_INTERN_VAR
xptr        _x_os_dll;
#endif

X_INTERN_VAR
xLogLevelFlags x_log_always_fatal;

X_INTERN_VAR
xLogLevelFlags x_log_msg_prefix;

X_INTERN_FUNC
xsize       _x_nearest_power        (xsize          base,
                                     xsize          num);

X_INTERN_VAR
xbool           x_mem_gc_friendly;

X_END_DECLS
#endif /* __X_INIT_PRV_H__ */
