#include "config.h"
#include "xmsg.h"
#include "xbit.h"
#include "xmem.h"
#include "xstr.h"
#include "xprintf.h"
#include "xprintf-prv.h"
#include "xthread.h"
#include "xinit-prv.h"
#include "xtest.h"
#include "xiconv.h"
#include "xstring.h"
#include "xutil.h"
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

typedef struct _LogDomain	LogDomain;
typedef struct _LogHandler	LogHandler;
struct _LogDomain
{
    xchar		    *log_domain;
    xLogLevelFlags  fatal_mask;
    LogHandler	    *handlers;
    LogDomain	    *next;
};
struct _LogHandler
{
    xuint		    id;
    xLogLevelFlags  log_level;
    xLogFunc	    log_func;
    xptr	        data;
    LogHandler	    *next;
};


typedef struct {
    xchar           *log_domain;
    xLogLevelFlags  log_level;
    xchar           *pattern;
} TestExpectedMessage;

X_LOCK_DEFINE_STATIC(messages);
static LogDomain        *log_domains = NULL;
static xSList           *expected_messages = NULL;
static xPrivate         log_depth;
static xTestLogFatalFunc fatal_log_func = NULL;
static xptr             fatal_log_data;
static xLogFunc         default_log_func = x_log_default_handler;
static xptr             default_log_data = NULL;

static LogDomain*
log_find_domain_L       (xcstr          log_domain)
{
    LogDomain *domain;

    domain = log_domains;
    while (domain){
        if (strcmp (domain->log_domain, log_domain) == 0)
            return domain;
        domain = domain->next;
    }
    return NULL;
}
static xLogFunc
log_domain_get_handler_L(LogDomain	    *domain,
                         xLogLevelFlags log_level,
                         xptr	        *data)
{
    if (domain && log_level)
    {
        register LogHandler *handler;

        handler = domain->handlers;
        while (handler)
        {
            if ((handler->log_level & log_level) == log_level)
            {
                *data = handler->data;
                return handler->log_func;
            }
            handler = handler->next;
        }
    }

    *data = default_log_data;
    return default_log_func;
}
#ifdef X_OS_WINDOWS
#include <process.h>		/* For getpid() */
#include <crtdbg.h>
#include <io.h>
#define _WIN32_WINDOWS 0x0401 /* to get IsDebuggerPresent */
#include <windows.h>
#define getpid _getpid
static xbool win32_keep_fatal_message = FALSE;

/* This default message will usually be overwritten. */
/* Yes, a fixed size buffer is bad. So sue me. But g_error() is never
 * called with huge strings, is it?
 */
static xchar  fatal_msg_buf[1000] = "Unspecified fatal error encountered, aborting.";
static xchar *fatal_msg_ptr = fatal_msg_buf;

#undef write
static inline int
dowrite (int fd, const void  *buf, unsigned len)
{
    if (win32_keep_fatal_message)
    {
        memcpy (fatal_msg_ptr, buf, len);
        fatal_msg_ptr += len;
        *fatal_msg_ptr = 0;
        return len;
    }

    _write (fd, buf, len);

    return len;
}
#define write(fd, buf, len) dowrite(fd, buf, len)

#endif

static void
write_string (int          fd,
              const xchar *string)
{
    write (fd, string, (unsigned)strlen (string));
}
#define FORMAT_UNSIGNED_BUFSIZE ((X_SIZEOF_LONG * 3) + 3)

static void
format_unsigned (xchar  *buf,
                 xulong  num,
                 xuint   radix)
{
    xulong tmp;
    xchar c;
    xint i, n;

    /* we may not call _any_ GLib functions here (or macros like g_return_if_fail()) */

    if (radix != 8 && radix != 10 && radix != 16)
    {
        *buf = '\000';
        return;
    }

    if (!num)
    {
        *buf++ = '0';
        *buf = '\000';
        return;
    } 

    if (radix == 16)
    {
        *buf++ = '0';
        *buf++ = 'x';
    }
    else if (radix == 8)
    {
        *buf++ = '0';
    }

    n = 0;
    tmp = num;
    while (tmp)
    {
        tmp /= radix;
        n++;
    }

    i = n;

    /* Again we can't use g_assert; actually this check should _never_ fail. */
    if (n > FORMAT_UNSIGNED_BUFSIZE - 3)
    {
        *buf = '\000';
        return;
    }

    while (num)
    {
        i--;
        c = (xchar)(num % radix);
        if (c < 10)
            buf[i] = c + '0';
        else
            buf[i] = c + 'a' - 10;
        num /= radix;
    }

    buf[n] = '\000';
}
/* string size big enough to hold level prefix */
#define	STRING_BUFFER_SIZE	(FORMAT_UNSIGNED_BUFSIZE + 32)

#define	ALERT_LEVELS		(X_LOG_LEVEL_ERROR | X_LOG_LEVEL_CRITICAL | X_LOG_LEVEL_WARNING)

/* these are emitted by the default log handler */
#define DEFAULT_LEVELS (X_LOG_LEVEL_ERROR | X_LOG_LEVEL_CRITICAL | X_LOG_LEVEL_WARNING | X_LOG_LEVEL_MESSAGE)
/* these are filtered by G_MESSAGES_DEBUG by the default log handler */
#define INFO_LEVELS (X_LOG_LEVEL_INFO | X_LOG_LEVEL_DEBUG)
static int
mklevel_prefix          (xstr           level_prefix,
                         xLogLevelFlags log_level)
{
    xbool to_stdout = TRUE;

    /* we may not call _any_ GLib functions here */

    switch (log_level & X_LOG_LEVEL_MASK) {
    case X_LOG_LEVEL_ERROR:
        strcpy (level_prefix, "ERROR");
        to_stdout = FALSE;
        break;
    case X_LOG_LEVEL_CRITICAL:
        strcpy (level_prefix, "CRITICAL");
        to_stdout = FALSE;
        break;
    case X_LOG_LEVEL_WARNING:
        strcpy (level_prefix, "WARNING");
        to_stdout = FALSE;
        break;
    case X_LOG_LEVEL_MESSAGE:
        strcpy (level_prefix, "Message");
        to_stdout = FALSE;
        break;
    case X_LOG_LEVEL_INFO:
        strcpy (level_prefix, "INFO");
        break;
    case X_LOG_LEVEL_DEBUG:
        strcpy (level_prefix, "DEBUG");
        break;
    default:
        if (log_level) {
            strcpy (level_prefix, "LOG-");
            format_unsigned (level_prefix + 4, log_level & X_LOG_LEVEL_MASK, 16);
        }
        else
            strcpy (level_prefix, "LOG");
        break;
    }
    if (log_level & X_LOG_FLAG_RECURSION)
        strcat (level_prefix, " (recursed)");
    if (log_level & ALERT_LEVELS)
        strcat (level_prefix, " **");

#ifdef X_OS_WIN32
    win32_keep_fatal_message = (log_level & X_LOG_FLAG_FATAL) != 0;
#endif
    return to_stdout ? 1 : 2;
}
static void
log_fallback_handler    (xcstr          log_domain,
                         xLogLevelFlags log_level,
                         xcstr          message,
                         xptr           user_data)
{
    xchar level_prefix[STRING_BUFFER_SIZE];
#ifndef X_OS_WIN32
    xchar pid_string[FORMAT_UNSIGNED_BUFSIZE];
#endif
    int fd;

    /* we cannot call _any_ GLib functions in this fallback handler,
     * which is why we skip UTF-8 conversion, etc.
     * since we either recursed or ran out of memory, we're in a pretty
     * pathologic situation anyways, what we can do is giving the
     * the process ID unconditionally however.
     */

    fd = mklevel_prefix (level_prefix, log_level);
    if (!message)
        message = "(NULL) message";

#ifndef X_OS_WIN32
    format_unsigned (pid_string, getpid (), 10);
#endif

    if (log_domain)
        write_string (fd, "\n");
    else
        write_string (fd, "\n** ");

#ifndef X_OS_WIN32
    write_string (fd, "(process:");
    write_string (fd, pid_string);
    write_string (fd, "): ");
#endif

    if (log_domain) {
        write_string (fd, log_domain);
        write_string (fd, "-");
    }
    write_string (fd, level_prefix);
    write_string (fd, ": ");
    write_string (fd, message);
}

void
x_log                   (xcstr          log_domain,
                         xLogLevelFlags log_level,
                         xcstr          format,
                         ...)
{
    va_list args;
    va_start (args, format);
    x_logv (log_domain, log_level, format, args);
    va_end (args);
}

void
x_logv                  (xcstr          log_domain,
                         xLogLevelFlags log_level,
                         xcstr          format,
                         va_list        args)
{
    int i;
    xbool was_fatal = (log_level & X_LOG_FLAG_FATAL) != 0;
    xbool was_recursion = (log_level & X_LOG_FLAG_RECURSION) != 0;
    xchar buffer[1025], *msg, *msg_alloc = NULL;

    log_level &= X_LOG_LEVEL_MASK;
    if (!log_level)
        return;

    if (was_recursion) {
        /* we use a stack buffer of fixed size, since we're likely
         * in an out-of-memory situation
         */
        xsize size;

        size = _x_vsnprintf (buffer, 1024, format, args);
        msg = buffer;
    } else {
        msg = msg_alloc = x_strdup_vprintf (format, args);
    }
    /*
       if (expected_messages)
       {
       GTestExpectedMessage *expected = expected_messages->data;

       expected_messages = g_slist_delete_link (expected_messages,
       expected_messages);
       if (strcmp (expected->log_domain, log_domain) == 0 &&
       ((log_level & expected->log_level) == expected->log_level) &&
       g_pattern_match_simple (expected->pattern, msg))
       {
       g_free (expected->log_domain);
       g_free (expected->pattern);
       g_free (expected);
       g_free (msg_alloc);
       return;
       }
       else
       {
       gchar level_prefix[STRING_BUFFER_SIZE];
       gchar *expected_message;

       mklevel_prefix (level_prefix, expected->log_level);
       expected_message = g_strdup_printf ("Did not see expected message %s: %s",
       level_prefix, expected->pattern);
       g_log_default_handler (log_domain, log_level, expected_message, NULL);
       g_free (expected_message);

       log_level |= G_LOG_FLAG_FATAL;
       }
       }
       */
    for (i = x_bit_nth_msf (log_level, -1);
         i >= 0; i = x_bit_nth_msf (log_level, i)) {

        register xLogLevelFlags test_level;
        test_level = 1 << i;
        if (log_level & test_level) {
            LogDomain *domain;
            xLogFunc log_func;
            xLogLevelFlags domain_fatal_mask;
            xptr data = NULL;
            xbool masquerade_fatal = FALSE;
            xsize depth;

            if (was_fatal)
                test_level |= X_LOG_FLAG_FATAL;
            if (was_recursion)
                test_level |= X_LOG_FLAG_RECURSION;

            X_LOCK (messages);
            depth = X_PTR_TO_SIZE (x_private_get (&log_depth));
            domain = log_find_domain_L (log_domain ? log_domain : "");
            if (depth)
                test_level |= X_LOG_FLAG_RECURSION;
            depth++;
            domain_fatal_mask = domain ? domain->fatal_mask : X_LOG_FATAL_MASK;
            if ((domain_fatal_mask | x_log_always_fatal) & test_level)
                test_level |= X_LOG_FLAG_FATAL;
            if (test_level & X_LOG_FLAG_RECURSION)
                log_func = log_fallback_handler;
            else
                log_func = log_domain_get_handler_L (domain, test_level, &data);
            domain = NULL;
            X_UNLOCK (messages);

            x_private_set (&log_depth, X_SIZE_TO_PTR (depth));

            log_func (log_domain, test_level, msg, data);

            if ((test_level & X_LOG_FLAG_FATAL)
                && !(test_level & X_LOG_LEVEL_ERROR)) {
                masquerade_fatal = fatal_log_func
                    && !fatal_log_func (log_domain, test_level, msg, fatal_log_data);
            }

            if ((test_level & X_LOG_FLAG_FATAL) && !masquerade_fatal) {
#ifdef X_OS_WINDOWS
                xutf16 locale_msg = x_str_to_utf16 (fatal_msg_buf, -1,NULL,NULL,NULL);
                MessageBoxW (NULL, locale_msg, NULL, MB_ICONERROR|MB_SETFOREGROUND);
                if (IsDebuggerPresent () && !(test_level & X_LOG_FLAG_RECURSION))
                    X_BREAK_POINT ();
                else
                    abort ();
                x_free (locale_msg);
#else
                if (!(test_level & X_LOG_FLAG_RECURSION))
                    X_BREAK_POINT ();
                else
                    abort ();
#endif
            }

            depth--;
            x_private_set (&log_depth, X_SIZE_TO_PTR (depth));
        }
    }
    x_free (msg_alloc);
}

void
x_log_default_handler   (xcstr          log_domain,
                         xLogLevelFlags log_level,
                         xcstr          message,
                         xptr	        user_data)
{
    xchar level_prefix[STRING_BUFFER_SIZE],  *str;
    xString *string;
    int fd;
    static xcstr domains;

    if (!domains) domains = x_getenv ("X_MESSAGES_DEBUG");

    if ((log_level & DEFAULT_LEVELS) || (log_level >> X_LOG_LEVEL_USER_SHIFT))
        goto emit;

    if (((log_level & INFO_LEVELS) == 0) ||
        domains == NULL ||
        (strcmp (domains, "all") != 0 &&
         (!log_domain || !strstr (domains, log_domain))))
        ;//return;

emit:
    /* we can be called externally with recursion for whatever reason */
    if (log_level & X_LOG_FLAG_RECURSION) {
        log_fallback_handler (log_domain, log_level, message, user_data);
        return;
    }

    fd = mklevel_prefix (level_prefix, log_level);

    string = x_string_new (NULL,0);
    if (log_level & ALERT_LEVELS)
        x_string_append (string, "\n");
    if (!log_domain)
        x_string_append (string, "** ");

    if ((x_log_msg_prefix & (log_level & X_LOG_LEVEL_MASK))
        == (log_level & X_LOG_LEVEL_MASK)) {
        xcstr prg_name = x_get_prgname ();

        if (!prg_name)
            x_string_append_printf (string,
                                    "(process:%lu): ", (xulong)getpid ());
        else
            x_string_append_printf (string,
                                    "(%s:%lu): ", prg_name, (xulong)getpid ());
    }

    if (log_domain) {
        x_string_append (string, log_domain);
        x_string_append_char (string, '-');
    }
    x_string_append (string, level_prefix);

    x_string_append (string, ": ");
    if (!message)
        x_string_append (string, "(NULL) message");
    else {
        xString *msg = x_string_new (message, -1);
        x_string_escape_invalid (msg);
        x_string_append (string, msg->str);
        x_string_delete (msg, TRUE);
    }
    x_string_append (string, "\n");

    str = x_string_delete (string, FALSE);

    write_string (fd, str);
    x_free (str);
}
