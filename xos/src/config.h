#define X_LOG_DOMAIN            "xos"
#define X_I18N_DOMAIN           "xos"
#include "xconfig.h"
//#define X_DISABLE_ASSERT

#define HAVE_GOOD_PRINTF
#undef  HAVE_STPCPY

#define MIN_ARRAY_SIZE          4
#define MAXIMUM_WAIT_OBJETS     128

#ifdef _WIN32
#define HAVE_WIN_ICONV_H        1
#define HAVE_MALLOC_H           1
#define HAVE_ALIGNED_MALLOC     1
#define HAVE_XLOCALE            1
#define locale_t                _locale_t
#define strtod_l                _strtod_l
#define strtoll_l               _strtoi64_l
#define newlocale(mask, locale, base) _create_locale(LC_ALL,locale)
#define SIZEOF_VOID_P           8
#elif defined X_OS_ANDROID
#define HAVE_SYS_TIME_H         1
#define HAVE_UNISTD_H           1
#define HAVE_CLOCK_GETTIME      1

#elif defined X_OS_DARWIN
#define HAVE_SYS_TIME_H         1
#define HAVE_UNISTD_H           1
#define HAVE_ICONV_H            1
#define HAVE_FCNTL_H            1
#define HAVE_XLOCALE_H          1
#define HAVE_DLFCN_H            1

#elif defined X_OS_IPPHONEOS
#define HAVE_SYS_TIME_H         1
#define HAVE_UNISTD_H           1
#define HAVE_ICONV_H            1

#endif

