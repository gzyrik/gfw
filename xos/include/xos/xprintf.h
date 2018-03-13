#ifndef __X_PRINTF_H__
#define __X_PRINTF_H__
#include <stdarg.h>
#include <stdio.h>
#include "xtype.h"
X_BEGIN_DECLS

xint        x_printf                (xcstr          format,
                                     ...) X_PRINTF (1, 2);

xint        x_fprintf               (FILE           *file,
                                     xcstr          format,
                                     ...) X_PRINTF (2, 3);

xint        x_sprintf               (xstr           str,
                                     xcstr          format,
                                     ...) X_PRINTF (2, 3);

xint        x_snprintf              (xstr           str,
                                     xsize          n,
                                     xcstr          format,
                                     ...) X_PRINTF (3, 4);

xint        x_vprintf               (xcstr          format,
                                     va_list        args);

xint        x_vfprintf              (FILE           *file,
                                     xcstr          format,
                                     va_list        args);

xint        x_vsprintf              (xstr           str,
                                     xcstr          format,
                                     va_list        args);

xint        x_vsnprintf             (xstr           str,
                                     xsize          n,
                                     xcstr          format,
                                     va_list        args);

xint        x_vasprintf             (xstr           *str,
                                     xcstr          format,
                                     va_list        args);

xsize       x_printf_bound          (xcstr          format,
                                     va_list        args);
X_END_DECLS

#endif /* __X_PRINTF_H__ */