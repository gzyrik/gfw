#include "config.h"
#include "xwin.h"
#ifdef X_OS_WINDOWS
#include "xstr.h"
#include "xiconv.h"
#include <windows.h>
xcstr
x_win_getlocale         (void)
{
}
xstr
x_win_error_msg         (xint           error)
{
    xchar *retval;
    wchar_t *msg = NULL;
    xsize nchars;

    FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER
                    |FORMAT_MESSAGE_IGNORE_INSERTS
                    |FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, error, 0,
                    (LPWSTR) &msg, 0, NULL);
    if (msg != NULL) {
        nchars = wcslen (msg);

        if (nchars > 2 && msg[nchars-1] == '\n' && msg[nchars-2] == '\r')
            msg[nchars-2] = '\0';

        retval = x_utf16_to_str (msg, -1, NULL, NULL, NULL);

        LocalFree (msg);
    }
    else
        retval = x_strdup ("");

    return retval;
}
#endif
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
