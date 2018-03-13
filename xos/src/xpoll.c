#include "config.h"
#include "xpoll.h"
#include "xprintf.h"
#include "xmsg.h"
#include "xmem.h"
xbool _x_poll_debug;
#ifdef HAVE_POLL
xint
x_poll                  (xPollFd        *fds,
                         xsize          nfds,
                         xint           timeout)
{
    return poll ((struct pollfd *)fds, nfds, timeout);
}

#elif defined (X_OS_WINDOWS)
#include "xwin.h"
#include <windows.h>
static int
poll_rest (xbool poll_msgs, HANDLE *handles, xsize nhandles,
           xPollFd *fds, xsize nfds, xuint timeout)
{
    DWORD ready;
    xPollFd *f;
    int recursed_result;

    if (poll_msgs) {
        /* Wait for either messages or handles
         * -> Use MsgWaitForMultipleObjectsEx
         */
        if (_x_poll_debug)
            x_printf ("  MsgWaitForMultipleObjectsEx(%d, %d)\n",
                      nhandles, timeout);

        ready = MsgWaitForMultipleObjectsEx ((DWORD)nhandles, handles, timeout,
                                             QS_ALLINPUT, MWMO_ALERTABLE);

        if (ready == WAIT_FAILED) {
            xchar *emsg = x_win_error_msg (GetLastError ());
            x_warning ("MsgWaitForMultipleObjectsEx failed: %s", emsg);
            x_free (emsg);
        }
    }
    else if (nhandles == 0) {
        /* No handles to wait for, just the timeout */
        if (timeout == INFINITE)
            ready = WAIT_FAILED;
        else
        {
            SleepEx (timeout, TRUE);
            ready = WAIT_TIMEOUT;
        }
    }
    else {
        /* Wait for just handles
         * -> Use WaitForMultipleObjectsEx
         */
        if (_x_poll_debug)
            x_printf ("  WaitForMultipleObjectsEx(%d, %d)\n", nhandles, timeout);

        ready = WaitForMultipleObjectsEx ((DWORD)nhandles, handles, FALSE, timeout, TRUE);
        if (ready == WAIT_FAILED) {
            xchar *emsg = x_win_error_msg (GetLastError ());
            x_warning ("WaitForMultipleObjectsEx failed: %s", emsg);
            x_free (emsg);
        }
    }

    if (_x_poll_debug) {
        x_printf ("  wait returns %ld%s\n", ready,
                  (ready == WAIT_FAILED ? " (WAIT_FAILED)" :
                   (ready == WAIT_TIMEOUT ? " (WAIT_TIMEOUT)" :
                    (poll_msgs && ready == WAIT_OBJECT_0 + nhandles
                     ? " (msg)" : ""))));
    }

    if (ready == WAIT_FAILED)
        return -1;
    else if (ready == WAIT_TIMEOUT ||
             ready == WAIT_IO_COMPLETION)
        return 0;
    else if (poll_msgs && ready == WAIT_OBJECT_0 + nhandles) {
        for (f = fds; f < &fds[nfds]; ++f) {
            if (f->fd == X_WIN_MSG_HANDLE && f->events & X_IO_IN)
                f->revents |= X_IO_IN;
        }

        /* If we have a timeout, or no handles to poll, be satisfied
         * with just noticing we have messages waiting.
         */
        if (timeout != 0 || nhandles == 0)
            return 1;

        /* If no timeout and handles to poll, recurse to poll them,
         * too.
         */
        recursed_result = poll_rest (FALSE, handles, nhandles, fds, nfds, 0);
        return (recursed_result == -1) ? -1 : 1 + recursed_result;
    }
    else if (ready >= WAIT_OBJECT_0 && ready < WAIT_OBJECT_0 + nhandles) {
        for (f = fds; f < &fds[nfds]; ++f) {
            if ((HANDLE) f->fd == handles[ready - WAIT_OBJECT_0]) {
                f->revents = f->events;
                if (_x_poll_debug)
                    x_printf ("  got event %p\n", (HANDLE) f->fd);
            }
        }

        /* If no timeout and polling several handles, recurse to poll
         * the rest of them.
         */
        if (timeout == 0 && nhandles > 1) {
            /* Remove the handle that fired */
            xsize i;
            if (ready < nhandles - 1)
                for (i = ready - WAIT_OBJECT_0 + 1; i < nhandles; i++)
                    handles[i-1] = handles[i];
            nhandles--;
            recursed_result = poll_rest (FALSE, handles, nhandles, fds, nfds, 0);
            return (recursed_result == -1) ? -1 : 1 + recursed_result;
        }
        return 1;
    }

    return 0;
}

xint
x_poll                  (xPollFd        *fds,
                         xsize          nfds,
                         xint           timeout)
{
    HANDLE handles[MAXIMUM_WAIT_OBJETS];
    xbool poll_msgs = FALSE;
    xPollFd *f;
    xsize nhandles = 0;
    int retval;


    if (_x_poll_debug)
        x_printf ("x_poll: waiting for");
    for (f = fds; f < &fds[nfds]; ++f) {
        if (f->fd == X_WIN_MSG_HANDLE && (f->events & X_IO_IN)) {
            if (_x_poll_debug && !poll_msgs)
                x_printf (" MSG");
            poll_msgs = TRUE;
        }
        else if (f->fd > 0) {
            /* Don't add the same handle several times into the array, as
             * docs say that is not allowed, even if it actually does seem
             * to work.
             */
            xsize i;

            for (i = 0; i < nhandles; i++) {
                if (handles[i] == (HANDLE) f->fd)
                    break;
            }

            if (i == nhandles)
            {
                if (nhandles == MAXIMUM_WAIT_OBJECTS)
                {
                    x_warning ("Too many handles to wait for!\n");
                    break;
                }
                else
                {
                    if (_x_poll_debug)
                        x_printf (" %p", (HANDLE) f->fd);
                    handles[nhandles++] = (HANDLE) f->fd;
                }
            }
        }
    }

    if (_x_poll_debug)
        x_printf ("\n");

    for (f = fds; f < &fds[nfds]; ++f)
        f->revents = 0;

    if (timeout == (xuint32)-1)
        timeout = INFINITE;

    /* Polling for several things? */
    if (nhandles > 1 || (nhandles > 0 && poll_msgs)) {
        /* First check if one or several of them are immediately
         * available
         */
        retval = poll_rest (poll_msgs, handles, nhandles, fds, nfds, 0);

        /* If not, and we have a significant timeout, poll again with
         * timeout then. Note that this will return indication for only
         * one event, or only for messages. We ignore timeouts less than
         * ten milliseconds as they are mostly pointless on Windows, the
         * MsgWaitForMultipleObjectsEx() call will timeout right away
         * anyway.
         */
        if (retval == 0 && (timeout == INFINITE || timeout >= 10))
            retval = poll_rest (poll_msgs, handles, nhandles,
                                fds, nfds, timeout);
    }
    else {
        /* Just polling for one thing, so no need to check first if
         * available immediately
         */
        retval = poll_rest (poll_msgs, handles, nhandles, fds, nfds, timeout);
    }

    if (retval == -1) {
        for (f = fds; f < &fds[nfds]; ++f)
            f->revents = 0;
    }

    return retval;
}
#else
#define HAVE_SYS_SELECT_H

#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef GLIB_HAVE_SYS_POLL_H
#  include <sys/poll.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#ifdef X_OS_BEOS
#undef NO_FD_SET
#endif /* G_OS_BEOS */

#ifndef NO_FD_SET
#  define SELECT_MASK fd_set
#else /* !NO_FD_SET */
#  ifndef _AIX
typedef long fd_mask;
#  endif /* _AIX */
#  ifdef _IBMR2
#    define SELECT_MASK void
#  else /* !_IBMR2 */
#    define SELECT_MASK int
#  endif /* !_IBMR2 */
#endif /* !NO_FD_SET */

xint
x_poll                  (xPollFd        *fds,
                         xsize          nfds,
                         xint           timeout)
{
    struct timeval tv;
    SELECT_MASK rset, wset, xset;
    xPollFd *f;
    int ready;
    int maxfd = 0;

    FD_ZERO (&rset);
    FD_ZERO (&wset);
    FD_ZERO (&xset);

    for (f = fds; f < &fds[nfds]; ++f)
    {
        if (f->fd >= 0)
        {
            if (f->events & X_IO_IN)
                FD_SET (f->fd, &rset);
            if (f->events & X_IO_OUT)
                FD_SET (f->fd, &wset);
            if (f->events & X_IO_PRI)
                FD_SET (f->fd, &xset);
            if (f->fd > maxfd && (f->events & (X_IO_IN|X_IO_OUT|X_IO_PRI)))
                maxfd = f->fd;
        }
    }

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    ready = select (maxfd + 1, &rset, &wset, &xset,
                    timeout == -1 ? NULL : &tv);
    if (ready > 0)
        for (f = fds; f < &fds[nfds]; ++f)
        {
            f->revents = 0;
            if (f->fd >= 0)
            {
                if (FD_ISSET (f->fd, &rset))
                    f->revents |= X_IO_IN;
                if (FD_ISSET (f->fd, &wset))
                    f->revents |= X_IO_OUT;
                if (FD_ISSET (f->fd, &xset))
                    f->revents |= X_IO_PRI;
            }
        }

    return ready;
}
#endif
