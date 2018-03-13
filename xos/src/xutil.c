#include "config.h"
#include "xutil.h"
#include "xmsg.h"
#include "xstr.h"
#include "xmem.h"
#include "xiconv.h"
#include "xatomic.h"
#include "xthread.h"
#include "xfile.h"
#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef X_OS_WINDOWS
#include <windows.h>
#include <io.h>
#define putenv _putenv
#endif /* X_OS_WINDOWS */

X_LOCK_DEFINE_STATIC    (app);
static xstr             _prgname = NULL;
static xstr             _charset = NULL;
#ifdef X_OS_WINDOWS
#pragma comment(lib,"winmm.lib")
static ULONGLONG (*_GetTickCount64) (void) = NULL;
static xuint32 _win32_tick_epoch = 0;
static LARGE_INTEGER _win32_frequency;
void
_x_clock_win32_init     (void)
{
    HMODULE kernel32;

    _GetTickCount64 = NULL;
    kernel32 = GetModuleHandleW (L"KERNEL32.DLL");
    if (kernel32 != NULL)
        _GetTickCount64 = (void *) GetProcAddress (kernel32, "GetTickCount64");

    if (QueryPerformanceFrequency(&_win32_frequency))
        _win32_frequency.QuadPart /= 1000;
    else
        _win32_frequency.QuadPart = 0;
    _win32_tick_epoch = ((xuint32)GetTickCount()) >> 31;
}
#endif

void
x_ctime                 (xTimeVal       *result)
{
#ifndef X_OS_WINDOWS
    struct timeval r;

    x_return_if_fail (result != NULL);

    /*this is required on alpha, there the timeval structs are int's
      not longs and a cast only would fail horribly*/
    gettimeofday (&r, NULL);
    result->sec = r.tv_sec;
    result->usec = r.tv_usec;
#else
    FILETIME ft;
    xuint64 time64;

    x_return_if_fail (result != NULL);

    GetSystemTimeAsFileTime (&ft);
    memmove (&time64, &ft, sizeof (FILETIME));

    /* Convert from 100s of nanoseconds since 1601-01-01
     * to Unix epoch. Yes, this is Y2038 unsafe.
     */
    time64 -= 116444736000000000;
    time64 /= 10;

    result->sec = (xlong)(time64 / 1000000);
    result->usec = (xlong)(time64 % 1000000);
#endif
}
xint64
x_rtime                 (void)
{
    xTimeVal tv;

    x_ctime (&tv);

    return (((xint64) tv.sec) * 1000000) + tv.usec;
}
xint64
x_mtime                 (void)
{
#ifdef HAVE_CLOCK_GETTIME
    /* librt clock_gettime() is our first choice */
    struct timespec ts;

#ifdef CLOCK_MONOTONIC
    clock_gettime (CLOCK_MONOTONIC, &ts);
#else
    clock_gettime (CLOCK_REALTIME, &ts);
#endif
    return (((xint64) ts.tv_sec) * 1000000) + (ts.tv_nsec / 1000);

#elif defined (X_OS_WINDOWS)
    if (_win32_frequency.QuadPart) {
        LARGE_INTEGER qpcnt;
        QueryPerformanceCounter(&qpcnt);
        return qpcnt.QuadPart*1000/_win32_frequency.QuadPart;
    }
    else {
        xuint64 ticks;
        xuint32 ticks32;
        if (_GetTickCount64 != NULL) {
            xuint32 ticks_as_32bit;

            ticks = _GetTickCount64 ();
            ticks32 = timeGetTime();
            ticks_as_32bit = (xuint32)ticks;
            /* We could do some 2's complement hack, but we play it safe */
            if (ticks32 - ticks_as_32bit <= X_MAXINT32)
                ticks += ticks32 - ticks_as_32bit;
            else
                ticks -= ticks_as_32bit - ticks32;
        }
        else {
            xuint32 epoch;

            epoch = x_atomic_int_get (&_win32_tick_epoch);
            ticks32 = timeGetTime();    
            if ((ticks32 >> 31) != (epoch & 1)) {
                epoch++;
                x_atomic_int_set (&_win32_tick_epoch, epoch);
            }

            ticks = (xint64)ticks32 | ((xint64)epoch) << 31;
        }
        return ticks * 1000;  
    }
#else /* !HAVE_CLOCK_GETTIME && ! G_OS_WIN32*/

    xTimeVal tv;

    x_ctime (&tv);

    return (((xint64) tv.sec) * 1000000) + tv.usec;
#endif

}
xcstr
x_getenv                (xcstr          name)
{
    x_return_val_if_fail (name != NULL, NULL);
    return (xcstr)getenv (name);
}

void
x_putenv                (xcstr          exp)
{
    x_return_if_fail (exp != NULL);
    putenv ((char*)exp);
}
void
x_usleep                (xulong         microseconds)
{
#ifdef X_OS_WINDOWS
    Sleep (microseconds / 1000);
#else
    struct timespec request, remaining;
    request.tv_sec = microseconds / 1000000;
    request.tv_nsec = 1000 * (microseconds % 1000000);
    while (nanosleep (&request, &remaining) == -1 && errno == EINTR)
        request = remaining;
#endif
}

xcstr
x_get_prgname           (void)
{
    xcstr retval;

    X_LOCK (app);
#ifdef X_OS_WINDOWS
    if (_prgname == NULL) {
        static xbool beenhere = FALSE;

        if (!beenhere)
        {
            xstr utf8_buf = NULL;
            wchar_t buf[MAX_PATH+1];

            beenhere = TRUE;
            if (GetModuleFileNameW (GetModuleHandle (NULL),
                                    buf, X_N_ELEMENTS (buf)) > 0)
                utf8_buf = x_utf16_to_str (buf, -1, NULL, NULL, NULL);

            if (utf8_buf)
            {
                _prgname = x_path_get_basename (utf8_buf);
                x_free (utf8_buf);
            }
        }
    }
#endif
    retval = _prgname;
    X_UNLOCK (app);

    return retval;
}

void
x_set_prgname           (xcstr          prgname)
{
    X_LOCK (app);
    x_free (_prgname);
    _prgname = x_strdup (prgname);
    X_UNLOCK (app);
}

xcstr
x_get_charset           (void)
{
    xcstr retval;

    X_LOCK (app);
    retval = _charset;
    X_UNLOCK (app);

    return retval;
}
void
x_set_charset           (xcstr          charset)
{
    X_LOCK (app);
    x_free (_charset);
    _charset = x_strdup (charset);
    X_UNLOCK (app);
}
