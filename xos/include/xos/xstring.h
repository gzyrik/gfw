#ifndef __X_STRING_H__
#define __X_STRING_H__
#include "xtype.h"
#include <stdarg.h>
X_BEGIN_DECLS

xString*    x_string_new            (xcstr          init,
                                     xssize         len);

xstr        x_string_delete         (xString        *string,
                                     xbool          free_str);

void        x_string_erase          (xString        *string,
                                     xsize          pos,
                                     xssize         len);

void        x_string_insert         (xString        *string,
                                     xssize         pos,
                                     xcstr          str,
                                     xssize         len);

void        x_string_insert_char    (xString*       string,
                                     xssize         pos,
                                     xchar          ch);

void        x_string_escape_invalid (xString*       string);

#define     x_string_append(string, str)                                \
    x_string_insert (string, -1, str, -1)

#define     x_string_append_len(string, str, len)                       \
    x_string_insert (string, -1, str, len)

void        x_string_append_printf  (xString        *string,
                                     xcstr          format,
                                     ...) X_PRINTF (2, 3);


void        x_string_append_vprintf (xString        *string,
                                     xcstr          format,
                                     va_list        args);

void        x_string_append_uri_escaped (xString    *string,
                                         xcstr      unescaped,
                                         xcstr      reserved_chars_allowed,
                                         xbool      allow_utf8);

#define     x_string_append_char(string, ch) X_STMT_START {             \
    if (string->len + 1 < string->allocated_len) {                      \
        string->str[string->len++] = ch;                                \
        string->str[string->len] = 0;                                   \
    } else  x_string_insert_char (string, -1, ch);                      \
} X_STMT_END

struct _xString
{
    xstr    str;
    xsize   len;
    xsize   allocated_len;
};

X_END_DECLS
#endif /* __X_STRING_H__ */
