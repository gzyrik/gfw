#ifndef __X_MSG_H__
#define __X_MSG_H__
#include <stdarg.h>
#include "xtype.h"
X_BEGIN_DECLS

#define x_goto_clean_if_fail(expr) X_STMT_START {                       \
    if X_LIKELY (expr) { } else  {                                      \
        x_log (X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL,                      \
               "file %s: line %d: assertion `%s' failed",               \
               __FILE__, __LINE__, #expr);                              \
        goto clean;                                                     \
    }                                                                   \
} X_STMT_END

#define x_return_val_if_fail(expr, val) X_STMT_START {                  \
    if X_LIKELY (expr) { } else  {                                      \
        x_log (X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL,                      \
               "file %s: line %d: assertion `%s' failed",               \
               __FILE__, __LINE__, #expr);                              \
        return (val);                                                   \
    }                                                                   \
} X_STMT_END

#define x_return_if_fail(expr) X_STMT_START {                           \
    if X_LIKELY (expr) { } else  {                                      \
        x_log (X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL,                      \
               "file %s: line %d: assertion `%s' failed",               \
               __FILE__, __LINE__, #expr);                              \
        return;                                                         \
    }                                                                   \
} X_STMT_END

#define x_warn_if_fail(expr) X_STMT_START {                             \
    if X_LIKELY (expr) { } else  {                                      \
        x_log (X_LOG_DOMAIN, X_LOG_LEVEL_WARNING,                       \
               "file %s: line %d: check `%s' failed",                   \
               __FILE__, __LINE__, #expr);                              \
    }                                                                   \
} X_STMT_END

#define x_warn_if_reached() X_STMT_START {                              \
    x_log (X_LOG_DOMAIN, X_LOG_LEVEL_WARNING,                           \
           "file %s: line %d: reached failed",                          \
           __FILE__, __LINE__);                                         \
} X_STMT_END

#define x_error_if_reached() X_STMT_START {                             \
    x_log (X_LOG_DOMAIN, X_LOG_LEVEL_ERROR,                             \
           "file %s: line %d: reached failed",                          \
           __FILE__, __LINE__);                                         \
} X_STMT_END
typedef enum
{
    /* log flags */
    X_LOG_FLAG_RECURSION            = 1 << 0,
    X_LOG_FLAG_FATAL                = 1 << 1,

    /* log levels */
    X_LOG_LEVEL_ERROR               = 1 << 2,       /* always fatal */
    X_LOG_LEVEL_CRITICAL            = 1 << 3,
    X_LOG_LEVEL_WARNING             = 1 << 4,
    X_LOG_LEVEL_MESSAGE             = 1 << 5,
    X_LOG_LEVEL_INFO                = 1 << 6,
    X_LOG_LEVEL_DEBUG               = 1 << 7,

    X_LOG_LEVEL_MASK                = ~(X_LOG_FLAG_RECURSION | X_LOG_FLAG_FATAL),
    X_LOG_FATAL_MASK                = (X_LOG_FLAG_RECURSION | X_LOG_LEVEL_ERROR)
} xLogLevelFlags;

#define X_LOG_LEVEL_USER_SHIFT      8

typedef void (*xLogFunc)            (xcstr          log_domain,
                                     xLogLevelFlags log_level,
                                     xcstr          message,
                                     xptr           user_data);

void        x_log                   (xcstr          log_domain,
                                     xLogLevelFlags flags,
                                     xcstr          format,
                                     ...) X_PRINTF (3, 4);

void        x_logv                  (xcstr          log_domain,
                                     xLogLevelFlags flags,
                                     xcstr          format,
                                     va_list        argv);

void        x_log_default_handler   (xcstr          log_domain,
                                     xLogLevelFlags log_level,
                                     xcstr          message,
                                     xptr	        user_data);

#ifndef X_LOG_DOMAIN
#define X_LOG_DOMAIN    ((xchar*) 0)
#endif  /* X_LOG_DOMAIN */

#ifdef X_HAVE_ISO_VARARGS
#define x_error(...)  X_STMT_START {                                    \
    x_log (X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, __VA_ARGS__);               \
    for (;;) ;                                                          \
} X_STMT_END
#define x_message(...)              x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_MESSAGE,         \
                                           __VA_ARGS__)
#define x_critical(...)             x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_CRITICAL,        \
                                           __VA_ARGS__)
#define x_warning(...)              x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_WARNING,         \
                                           __VA_ARGS__)
#define x_debug(...)                x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_DEBUG,           \
                                           __VA_ARGS__)

#elif defined(X_HAVE_GNUC_VARARGS)
#define x_error(format...) X_STMT_START {                               \
    x_log (X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, format);                    \
    for (;;) ;                                                          \
} X_STMT_END
#define x_message(format...)        x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_MESSAGE,         \
                                           format)
#define x_critical(format...)       x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_CRITICAL,        \
                                           format)
#define x_warning(format...)        x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_WARNING,         \
                                           format)
#define x_debug(format...)          x_log (X_LOG_DOMAIN,                \
                                           X_LOG_LEVEL_DEBUG,           \
                                           format)
#else   /* no varargs macros */
static void
x_error                 (xcstr          format,
                         ...)
{
    va_list args;
    va_start (args, format);
    x_logv (X_LOG_DOMAIN, X_LOG_LEVEL_ERROR, format, args);
    va_end (args);

    for(;;) ;
}
static void
x_message               (xcstr          format,
                         ...)
{
    va_list args;
    va_start (args, format);
    x_logv (X_LOG_DOMAIN, X_LOG_LEVEL_MESSAGE, format, args);
    va_end (args);
}
static void
x_critical              (xcstr          format,
                         ...)
{
    va_list args;
    va_start (args, format);
    x_logv (X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL, format, args);
    va_end (args);
}
static void
x_warning               (xcstr          format,
                         ...)
{
    va_list args;
    va_start (args, format);
    x_logv (X_LOG_DOMAIN, X_LOG_LEVEL_WARNING, format, args);
    va_end (args);
}
static void
x_debug                 (xcstr          format,
                         ...)
{
    va_list args;
    va_start (args, format);
    x_logv (X_LOG_DOMAIN, X_LOG_LEVEL_DEBUG, format, args);
    va_end (args);
}
#endif  /* !X_HAVE_ISO_VARARGS */


X_END_DECLS
#endif /* __X_MSG_H__ */
