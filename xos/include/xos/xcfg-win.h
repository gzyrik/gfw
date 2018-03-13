#ifndef __X_CFG_WINDOWS_H__
#define __X_CFG_WINDOWS_H__
#include <limits.h>
#include <float.h>

#ifdef _WIN64
#define X_OS_WIN64              1
#else
#define X_OS_WIN32              1
#endif

#define X_OS_WINDOWS            1
#define X_SIZE_FORMAT           "d"
#define X_INT64_MODIFIER        "I64"
#define X_INT64_FORMAT          "I64i"
#define X_UINT64_FORMAT         "I64u"
#define X_SIZEOF_LONG           4
#define X_MININT                0
#define X_MAXINT                0xFFFFFFFF
#define X_MININT64              0
#define X_MAXINT64              0
#define X_MAXUINT64             0
#define X_MAXFLOAT              FLT_MAX
#define X_FLOAT_EPSILON         0
#define X_MAXDOUBLE             DBL_MAX
#define X_DOUBLE_EPSILON        0
#define va_copy(dst, src)       memcpy((&dst), (&src), sizeof(va_list)) 


#define X_MODULE_SUFFIX         "dll"
#define	X_MODULE_EXPORT		    __declspec(dllexport)
#define	X_MODULE_IMPORT		    __declspec(dllimport)

#define X_DIR_SEPARATOR         "\\"
#define X_IS_DIR_SEPARATOR(c)   (c == '\\' || c == '/')

#undef  X_HAVE_ALLOCA_H
#define X_HAVE_ISO_VARARGS
#define X_CAN_INLINE
#define X_HAVE_INLINE

#ifndef X_BYTE_ORDER
#define X_BYTE_ORDER            X_LITTLE_ENDIAN
#endif

#define X_BREAK_POINT()         _CrtDbgBreak()

#undef  X_HAVE_SSE_INTRINSICS


#endif /* __X_CFG_WINDOWS_H__ */

