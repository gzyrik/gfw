#ifndef __X_STR_H__
#define __X_STR_H__
#include "xtype.h"
#include <stdarg.h>
X_BEGIN_DECLS
xstr        x_stpcpy                (xstr           dest,
                                     xcstr          src);

xint        x_strcmp                (xcstr          str0,
                                     xcstr          str1);

xstr        x_strndup               (xcstr          str,
                                     xssize         length) X_MALLOC;

#define    x_strdup(str)            x_strndup(str, -1)

xstr        x_strcat                (xcstr          string1,
                                     ...) X_MALLOC X_NULL_TERMINATED;

xstr        x_strcatv               (xcstr          string1,
                                     va_list        args) X_MALLOC;


xstr        x_stresc                (xcstr          string1,
                                     ...) X_MALLOC X_NULL_TERMINATED;

xstr        x_strdup_printf         (xcstr          format,
                                     ...) X_PRINTF (1, 2) X_MALLOC;

xstr        x_strdup_vprintf        (xcstr          format,
                                     va_list        args) X_FORMAT(1) X_MALLOC;

xstr        x_strchug               (xstr           str);

xstr        x_strchomp              (xstr           str);

#define     x_strstrip( str )	x_strchomp (x_strchug (str))

xstr        x_str_suffix            (xcstr          str,
                                     xcstr          suffix);

xstr        x_str_prefix            (xcstr          str,
                                     xcstr          prefix);

xstr        x_str_ntoupper           (xcstr          str,
                                      xssize         length) X_MALLOC;

#define     x_str_toupper( str ) x_str_ntoupper (str, -1)

xstr        x_strdelimit            (xstr           str,
                                     xcstr          delimiters,
                                     xchar	        new_delim);

xstrv       x_strsplit              (xcstr          str,
                                     xcstr          delimiter,
                                     xssize         max_tokens) X_MALLOC;

xstrv       x_strv_new              (xcstr          str,
                                     ...) X_MALLOC X_NULL_TERMINATED;

xstrv       x_strv_newv             (xcstr          str,
                                     va_list        argv) X_MALLOC;

xint        x_strv_cmp              (xcstrv         strv0,
                                     xcstrv         strv1);

void        x_strv_free             (xstrv          str_array);

xstrv       x_strv_dup              (xcstrv         str_array);

xint        x_strv_find             (xcstrv         str_array,
                                     xcstr          str);

xstr        x_casefold              (xcstr          str,
                                     xssize         len) X_MALLOC;

xcstr       x_stristr               (xcstr          str,
                                     xcstr          sub);

xcstr       x_strrstr               (xcstr          str,
                                     xcstr          sub);

xdouble     x_strtod                (xcstr          nptr,
                                     xcstr          *endptr);

xint64      x_strtoll               (xcstr          nptr,
                                     xcstr          *endptr,
                                     xint           base);

int         x_xdigit_value          (xchar          c);

int         x_digit_value           (xchar          c);

X_END_DECLS
#endif /* __X_STR_H__ */
