#include "config.h"
#include "xwakeup.h"
#include "xmsg.h"
#if defined X_OS_WINDOWS
#include "xwin.h"
#include <windows.h>
xWakeup*
x_wakeup_new            (void)
{
    HANDLE wakeup;
    wakeup = CreateEvent (NULL, TRUE, FALSE, NULL);
    if (!wakeup)
        x_error ("Cannot create event for xWakeup: %s",
                 x_win_error_msg (GetLastError ()));

    return (xWakeup *) wakeup;
}

void
x_wakeup_signal         (xWakeup        *wakeup)
{
    SetEvent ((HANDLE) wakeup);
}

void
x_wakeup_ack            (xWakeup        *wakeup)
{
    ResetEvent ((HANDLE) wakeup);
}

void
x_wakeup_delete         (xWakeup        *wakeup)
{
    CloseHandle ((HANDLE) wakeup);
}

#else

#include "xslice.h"
#include "xerror.h"
#include "xunix.h"
#include <fcntl.h>
#if defined (HAVE_EVENTFD)
#include <sys/eventfd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

struct _xWakeup
{
  xint fds[2];
};

xWakeup*
x_wakeup_new            (void)
{
    xError *error = NULL;
    xWakeup *wakeup;

    wakeup = x_slice_new (xWakeup);

    /* try eventfd first, if we think we can */
#if defined (HAVE_EVENTFD)
#ifndef TEST_EVENTFD_FALLBACK
    wakeup->fds[0] = eventfd (0, EFD_CLOEXEC | EFD_NONBLOCK);
#else
    wakeup->fds[0] = -1;
#endif

    if (wakeup->fds[0] != -1)
    {
        wakeup->fds[1] = -1;
        return wakeup;
    }

    /* for any failure, try a pipe instead */
#endif

    if (!x_unix_pipe (wakeup->fds, FD_CLOEXEC, &error))
        x_error ("Creating pipes for xWakeup: %s\n", error->message);

    if (!x_unix_nonblocking (wakeup->fds[0], TRUE, &error) ||
        !x_unix_nonblocking (wakeup->fds[1], TRUE, &error))
        x_error ("Set pipes non-blocking for xWakeup: %s\n", error->message);

    return wakeup;
}

void
x_wakeup_signal         (xWakeup        *wakeup)
{
    xuint64 one = 1;

    if (wakeup->fds[1] == -1)
        write (wakeup->fds[0], &one, sizeof one);
    else
        write (wakeup->fds[1], &one, 1);
}

void
x_wakeup_ack            (xWakeup        *wakeup)
{
    char buffer[16];

    /* read until it is empty */
    while (read (wakeup->fds[0], buffer, sizeof buffer) == sizeof buffer);
}

void
x_wakeup_delete         (xWakeup        *wakeup)
{
    close (wakeup->fds[0]);

    if (wakeup->fds[1] != -1)
        close (wakeup->fds[1]);

    x_slice_free (xWakeup, wakeup);
}

#endif
