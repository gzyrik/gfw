#ifndef __X_CFG_IPHONEOS_H__
#define __X_CFG_IPHONEOS_H__

#define X_OS_IPPHONEOS          1
#define X_SIZE_FORMAT           "zu"
#define X_INT64_MODIFIER        "ll"
#define X_SIZEOF_LONG           4
#define X_MININT                0
#define X_MAXINT                0xFFFFFFFF
#define X_MININT64              0
#define X_MAXINT64              0
#define X_MAXUINT64             0
#define X_MAXFLOAT              0
#define X_FLOAT_EPSILON         0
#define X_MAXDOUBLE             0
#define X_DOUBLE_EPSILON        0

#define X_MODULE_SUFFIX         "so"
#define	X_MODULE_EXPORT
#define	X_MODULE_IMPORT

#define X_DIR_SEPARATOR         "/"
#define X_IS_DIR_SEPARATOR(c)   (c == '/')

#undef  X_HAVE_ALLOCA_H
#define X_HAVE_ISO_VARARGS
#define X_CAN_INLINE
#define X_HAVE___INLINE__

#define X_BREAK_POINT()
#ifndef X_BYTE_ORDER
#define X_BYTE_ORDER            X_LITTLE_ENDIAN
#endif

#endif /* __X_CFG_IPHONEOS_H__ */

