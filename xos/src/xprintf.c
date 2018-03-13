#include "config.h"
#include "xprintf.h"
#include "xprintf-prv.h"
#include "xmsg.h"
#include "xmem.h"
#include "xatomic.h"
#include <string.h>
xint
x_printf                (xcstr          format,
                         ...)
{
    va_list args;
    xint retval;

    va_start (args, format);
    retval = x_vprintf (format, args);
    va_end (args);

    return retval;
}

xint
x_fprintf               (FILE           *file,
                         xcstr          format,
                         ...)
{
    va_list args;
    xint retval;

    va_start (args, format);
    retval = x_vfprintf (file, format, args);
    va_end (args);

    return retval;
}

xint
x_sprintf               (xstr           str,
                         xcstr          format,
                         ...)
{
  va_list args;
  xint retval;

  va_start (args, format);
  retval = x_vsprintf (str, format, args);
  va_end (args);
  
  return retval;
}

xint
x_snprintf              (xstr           str,
                         xsize          n,
                         xcstr          format,
                         ...) 
{
    va_list args;
    xint retval;

    va_start (args, format);
    retval = x_vsnprintf (str, n, format, args);
    va_end (args);

    return retval;

}

xint
x_vprintf               (xcstr          format,
                         va_list        args)
{
    x_return_val_if_fail (format != NULL, -1);

    return _x_vprintf (format, args);
}    

xint
x_vfprintf              (FILE           *file,
                         xcstr          format,
                         va_list        args)
{
    x_return_val_if_fail (format != NULL, -1);

    return _x_vfprintf (file, format, args);
}  

xint
x_vsprintf              (xstr           str,
                         xcstr          format,
                         va_list        args)
{
  x_return_val_if_fail (str != NULL, -1);
  x_return_val_if_fail (format != NULL, -1);

  return _x_vsprintf (str, format, args);
}  

xint
x_vsnprintf             (xstr           str,
                         xsize          n,
                         xcstr          format,
                         va_list        args)
{
    x_return_val_if_fail (n == 0 || str != NULL, -1);
    x_return_val_if_fail (format != NULL, -1);

    return _x_vsnprintf (str, n, format, args);
}

xint
x_vasprintf             (xstr           *str,
                         xcstr          format,
                         va_list        args)
{
    xint len;
    x_return_val_if_fail (str != NULL, -1);

#if !defined(HAVE_GOOD_PRINTF)

    len = _x_gnulib_vasprintf (str, format, args);
    if (len < 0)
        *str = NULL;

#elif defined (HAVE_VASPRINTF)

    len = vasprintf (str, format, args);
    if (len < 0)
        *str = NULL;
    else if (!g_mem_is_system_malloc ()) 
    {
        /* vasprintf returns malloc-allocated memory */
        xstr str1 = x_strdup (*str, len);
        free (*str);
        *str = str1;
    }

#else

    {
        va_list args2;

        va_copy (args2, args);

        *str = x_new (xchar, x_printf_bound (format, args2));

        len = _x_vsprintf (*str, format, args);
        va_end (args2);
    }
#endif

    return len;
}

xsize
x_printf_bound          (xcstr          format,
                         va_list        args)
{
    static FILE *dev_null = NULL;
    if (!x_atomic_ptr_get (&dev_null)
        && x_once_init_enter (&dev_null) ){
#ifdef X_OS_WIN32
        dev_null = fopen ("nul", "w");
#else
        dev_null = fopen ("/dev/null", "w");
#endif
        x_once_init_leave (&dev_null);
    }
    return x_vfprintf (dev_null, format, args) + 1;
}
