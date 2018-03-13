#include "config.h"
#include "xunix.h"
#ifndef X_OS_WINDOWS
#include "xstr.h"
#include "xerror.h"
#include "xquark.h"
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_UNISTD_H */

xQuark
x_unix_error_quark      (void)
{
    static xQuark q;
    if X_UNLIKELY (q == 0)
        q = x_quark_from_static("Unix");
    return q;
}

xbool
x_unix_pipe             (int        *fds,
                         xbool      close_on_exec,
                         xError     **error)
{
    int ecode;
    const int flags = 0;/* close_on_exec ? FD_CLOEXEC : 0; */
#ifdef HAVE_PIPE2
    const int pipe2_flags = close_on_exec ? O_CLOEXEC : 0;
    /* Atomic */
    ecode = pipe2 (fds, pipe2_flags);
    if (ecode == -1 && errno != ENOSYS)
        return x_unix_error_from_errno (error, errno);
    else if (ecode == 0)
        return TRUE;
    /* Fall through on -ENOSYS, we must be running on an old kernel */
#endif
    ecode = pipe (fds);
    if (ecode == -1)
        return x_unix_error_from_errno (error, errno);
#ifdef F_GETFL
    ecode = fcntl (fds[0], flags);
    if (ecode == -1)
    {
        int saved_errno = errno;
        close (fds[0]);
        close (fds[1]);
        return x_unix_error_from_errno (error, saved_errno);
    }
    ecode = fcntl (fds[1], flags);
    if (ecode == -1)
    {
        int saved_errno = errno;
        close (fds[0]);
        close (fds[1]);
        return x_unix_error_from_errno (error, saved_errno);
    }
#endif
    return TRUE;
}

xbool
x_unix_nonblocking      (xint       fd,
                         xbool      nonblock,
                         xError     **error)
{
#ifdef F_GETFL
    xlong fcntl_flags;
    fcntl_flags = fcntl (fd, F_GETFL);

    if (fcntl_flags == -1)
        return x_unix_error_from_errno (error, errno);

    if (nonblock)
    {
#ifdef O_NONBLOCK
        fcntl_flags |= O_NONBLOCK;
#else
        fcntl_flags |= O_NDELAY;
#endif
    }
    else
    {
#ifdef O_NONBLOCK
        fcntl_flags &= ~O_NONBLOCK;
#else
        fcntl_flags &= ~O_NDELAY;
#endif
    }

    if (fcntl (fd, F_SETFL, fcntl_flags) == -1)
        return x_unix_error_from_errno (error, errno);
    return TRUE;
#else
    return x_unix_error_from_errno (error, EINVAL);
#endif
}

xbool
x_unix_error_from_errno (xError     **error,
                         xint       saved_errno)
{
    x_set_error (error,
                 X_UNIX_ERROR,
                 saved_errno,
                 "%s",
                 strerror (saved_errno));
    return FALSE;
}
#endif
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
