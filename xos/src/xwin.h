#ifndef __X_WIN_H__
#define __X_WIN_H__
#include "xtype.h"
#if X_OS_WINDOWS
X_BEGIN_DECLS

xstr    x_win_error_msg (xint           error);

xcstr   x_win_getlocale (void);

X_END_DECLS
#endif
#endif /* __X_WIN32_H__ */
