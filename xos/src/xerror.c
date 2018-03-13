#include "config.h"
#include "xerror.h"
#include "xmsg.h"
#include "xslice.h"
#include "xstr.h"
#include "xmem.h"
#define ERROR_OVERWRITTEN_WARNING \
    "xError set over the top of a previous xError or uninitialized memory.\n" \
"This indicates a bug in someone's code. You must ensure an error is NULL "   \
"before it's set.\nThe overwriting error message was: %s"

xError*
x_error_new             (xQuark         domain,
                         xint           code,
                         xcstr          format,
                         ...)
{
    xError* error;
    va_list args;

    x_warn_if_fail (domain != 0);
    x_warn_if_fail (format != NULL);

    va_start (args, format);
    error = x_error_new_valist (domain, code, format, args);
    va_end (args);

    return error;
}

xError*
x_error_new_valist      (xQuark         domain,
                         xint           code,
                         xcstr          format,
                         va_list        argv)
{
    xError *error;

    x_warn_if_fail (domain != 0);
    x_warn_if_fail (format != NULL);

    error = x_slice_new (xError);

    error->domain = domain;
    error->code = code;
    error->message = x_strdup_vprintf (format, argv);

    return error; 
}

void
x_error_delete          (xError         *error)
{
    if (error) {
        x_free (error->message);
        x_slice_free (xError, error);
    }
}

xError*
x_error_dup             (xError         *error)
{
    xError *copy;

    x_return_val_if_fail (error != NULL, NULL);
    /* See g_error_new_valist for why these don't return */
    x_warn_if_fail (error->domain != 0);
    x_warn_if_fail (error->message != NULL);

    copy = x_slice_new (xError);

    *copy = *error;

    copy->message = x_strdup (error->message);

    return copy;
}
void
x_set_error             (xError         **error,
                         xQuark         domain,
                         xint           code,
                         xcstr          format,
                         ...)
{
    xError *new_error;

    va_list args;

    if (error == NULL)
        return;

    va_start (args, format);
    new_error = x_error_new_valist (domain, code, format, args);
    va_end (args);

    if (*error == NULL)
        *error = new_error;
    else {
        x_warning (ERROR_OVERWRITTEN_WARNING, new_error->message); 
        x_error_delete (new_error);
    }
}
void
x_propagate_error       (xError         **dst,
                         xError         *src)
{
    x_return_if_fail (src != NULL);

    if (dst == NULL) {
        x_error_delete (src);
    }
    else {
        if (*dst != NULL)
            x_warning (ERROR_OVERWRITTEN_WARNING, src->message);
        else
            *dst = src;
    }
}
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */

