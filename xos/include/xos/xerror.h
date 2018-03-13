#ifndef __X_ERROR_H__
#define __X_ERROR_H__
#include "xtype.h"
#include <stdarg.h>
X_BEGIN_DECLS

xError*     x_error_new             (xQuark         domain,
                                     xint           code,
                                     xcstr          format,
                                     ...) X_PRINTF(3, 4);

xError*     x_error_new_valist      (xQuark         domain,
                                     xint           code,
                                     xcstr          format,
                                     va_list        argv);

void        x_error_delete          (xError         *error);

xError*     x_error_dup             (xError         *error);

void        x_set_error             (xError         **error,
                                     xQuark         domain,
                                     xint           code,
                                     xcstr          format,
                                     ...) X_PRINTF(4, 5);

void        x_propagate_error       (xError         **dst,
                                     xError         *src);

struct _xError
{
    xQuark      domain;
    xint        code;
    xstr        message;
};

X_END_DECLS
#endif /* __X_ERROR_H__ */

