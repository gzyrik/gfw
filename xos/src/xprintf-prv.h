#ifndef __X_PRINTF_PRV_H__
#define __X_PRINTF_PRV_H__

#ifdef HAVE_GOOD_PRINTF

#define _x_printf    printf
#define _x_fprintf   fprintf
#define _x_sprintf   sprintf
#define _x_snprintf  snprintf

#define _x_vprintf   vprintf
#define _x_vfprintf  vfprintf
#define _x_vsprintf  vsprintf
#define _x_vsnprintf vsnprintf

#else

#include "gnulib/printf.h"

#define _x_printf    _x_gnulib_printf
#define _x_fprintf   _x_gnulib_fprintf
#define _x_sprintf   _x_gnulib_sprintf
#define _x_snprintf  _x_gnulib_snprintf

#define _x_vprintf   _x_gnulib_vprintf
#define _x_vfprintf  _x_gnulib_vfprintf
#define _x_vsprintf  _x_gnulib_vsprintf
#define _x_vsnprintf _x_gnulib_vsnprintf

#endif

#endif /* __X_PRINTF_PRV_H__ */
