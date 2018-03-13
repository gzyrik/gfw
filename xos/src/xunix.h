#ifndef __X_UNIX_H__
#define __X_UNIX_H__
#include "xtype.h"
#ifndef X_OS_WIN32
X_BEGIN_DECLS

xbool       x_unix_pipe             (int        *fds,
                                     xbool      close_on_exec,
                                     xError     **error);

xbool       x_unix_nonblocking      (xint       fd,
                                     xbool      nonblock,
                                     xError     **error);

xbool       x_unix_error_from_errno (xError     **error,
                                     xint       saved_errno);

#define X_UNIX_ERROR                (x_unix_error_quark())
xQuark      x_unix_error_quark      (void);


X_END_DECLS
#endif
#endif /* __X_UNIX_H__ */
