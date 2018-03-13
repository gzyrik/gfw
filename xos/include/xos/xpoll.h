#ifndef __X_POLL_H__
#define __X_POLL_H__
#include "xtype.h"
X_BEGIN_DECLS
#define X_WIN_MSG_HANDLE 19981206

enum {
    X_IO_IN,
    X_IO_OUT,
    X_IO_PRI,
    X_IO_ERR,
    X_IO_HUP,
    X_IO_NVAL
};
struct _xPollFd
{
#ifdef X_OS_WIN64
    xint64	    fd;
#else
    xint		fd;
#endif
    xushort 	events;
    xushort 	revents;
};
xint        x_poll                  (xPollFd        *fds,
                                     xsize          nfds,
                                     xint           timeoutMS);

X_END_DECLS
#endif /*__X_POLL_H__ */
