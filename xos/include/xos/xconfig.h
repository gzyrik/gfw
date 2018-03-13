#ifndef __X_CONFIG_H__
#define __X_CONFIG_H__
/* We include stddef.h to get the system's definition of NULL
 *
 */
#include <stddef.h>

/* Here we provide X_EXTENSION as an alias for __extension__,
 * where this is valid. This allows for warningless compilation of
 * "long long" types even in the presence of '-ansi -pedantic'. 
 */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
#  define X_EXTENSION __extension__
#else
#  define X_EXTENSION
#endif

/* Provide macros to feature the GCC function attribute.
 *
 */
#if    __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define X_PURE                              \
    __attribute__((__pure__))
#define X_MALLOC                            \
    __attribute__((__malloc__))
#else
#define X_PURE
#define X_MALLOC
#endif

#if     __GNUC__ >= 4
#define X_NULL_TERMINATED __attribute__((__sentinel__))
#else
#define X_NULL_TERMINATED
#endif

#if     (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define X_ALLOC_SIZE(x) __attribute__((__alloc_size__(x)))
#define X_ALLOC_SIZE2(x,y) __attribute__((__alloc_size__(x,y)))
#else
#define X_ALLOC_SIZE(x)
#define X_ALLOC_SIZE2(x,y)
#endif

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define X_PRINTF( format_idx, arg_idx )     \
    __attribute__((__format__ (__printf__, format_idx, arg_idx)))
#define X_SCANF( format_idx, arg_idx )      \
    __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
#define X_FORMAT( arg_idx )                 \
    __attribute__((__format_arg__ (arg_idx)))
#define X_NORETURN                          \
    __attribute__((__noreturn__))
#define X_CONST                             \
    __attribute__((__const__))
#define X_UNUSED                            \
    __attribute__((__unused__))
#define X_NO_INSTRUMENT                     \
    __attribute__((__no_instrument_function__))
#else   /* !__GNUC__ */
#define X_PRINTF( format_idx, arg_idx )
#define X_SCANF( format_idx, arg_idx )
#define X_FORMAT( arg_idx )
#define X_NORETURN
#define X_CONST
#define X_UNUSED
#define X_NO_INSTRUMENT
#endif  /* !__GNUC__ */

#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#  define X_MAY_ALIAS __attribute__((may_alias))
#else
#  define X_MAY_ALIAS
#endif

#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define X_WARN_UNUSED_RET                \
    __attribute__((warn_unused_result))
#else
#define X_WARN_UNUSED_RET
#endif /* __GNUC__ */

#define X_STRINGIFY(macro_or_string)    X_STRINGIFY_ARG (macro_or_string)
#define X_STRINGIFY_ARG(contents)   #contents

#ifndef __GI_SCANNER__ /* The static assert macro really confuses the introspection parser */
#define X_PASTE_ARGS(identifier1,identifier2) identifier1 ## identifier2
#define X_PASTE(identifier1,identifier2)      X_PASTE_ARGS (identifier1, identifier2)
#ifdef __COUNTER__
#define X_STATIC_ASSERT(expr) typedef char X_PASTE (_XStaticAssertCompileTimeAssertion_, __COUNTER__)[(expr) ? 1 : -1]
#else
#define X_STATIC_ASSERT(expr) typedef char X_PASTE (_XStaticAssertCompileTimeAssertion_, __LINE__)[(expr) ? 1 : -1]
#endif
#define X_STATIC_ASSERT_EXPR(expr) ((void) sizeof (char[(expr) ? 1 : -1]))
#endif

/* Provide a string identifying the current code position */
#if defined(__GNUC__) && (__GNUC__ < 3) && !defined(__cplusplus)
#  define X_STRLOC  __FILE__ ":" X_STRINGIFY (__LINE__) ":" __PRETTY_FUNCTION__ "()"
#else
#  define X_STRLOC  __FILE__ ":" X_STRINGIFY (__LINE__)
#endif

/* Provide a string identifying the current function, non-concatenatable */
#if defined (__GNUC__)
#  define X_STRFUNC     ((const char*) (__PRETTY_FUNCTION__))
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 19901L
#  define X_STRFUNC     ((const char*) (__func__))
#elif defined(_MSC_VER) && (_MSC_VER > 1300)
#  define X_STRFUNC     ((const char*) (__FUNCTION__))
#else
#  define X_STRFUNC     ((const char*) ("???"))
#endif

/* Guard C code in headers, while including them from C++ */
#ifdef  __cplusplus
# define X_BEGIN_DECLS  extern "C" {
# define X_END_DECLS    }
#else
# define X_BEGIN_DECLS
# define X_END_DECLS
#endif

/* Provide definitions for some commonly used macros.
 *  Some of them are only provided if they haven't already
 *  been defined. It is assumed that if they are already
 *  defined then the current definition is correct.
 */
#ifndef NULL
#  ifdef __cplusplus
#    define NULL        (0L)
#  else /* !__cplusplus */
#    define NULL        ((void*) 0)
#  endif /* !__cplusplus */
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef  ABS
#define ABS(a, b)  (((a) < (b)) ? (b)-(a) : (a)-(b))

#undef  CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* Count the number of elements in an array. The array must be defined
 * as such; using this with a dynamically allocated array will give
 * incorrect results.
 */
#define X_N_ELEMENTS(arr)       (sizeof (arr) / sizeof ((arr)[0]))

#define X_PTR_TO_SIZE(p) ((xsize) (p))
#define X_SIZE_TO_PTR(s) ((xptr) (xsize) (s))


#define X_LITTLE_ENDIAN 1234
#define X_BIG_ENDIAN    4321
#define X_PDP_ENDIAN    3412		/* unused, need specific PDP check */

/* Provide convenience macros for handling structure
 * fields through their offsets.
 */

#if defined(__GNUC__)  && __GNUC__ >= 4
#  define X_STRUCT_OFFSET(struct_type, member)  \
    ((xlong) offsetof (struct_type, member))
#else
#  define X_STRUCT_OFFSET(struct_type, member)  \
    ((xlong) ((xuint8*) &((struct_type*) 0)->member))
#endif

#define X_STRUCT_MEMBER_P(struct_p, struct_offset)   \
    ((xptr) ((xuint8*) (struct_p) + (xlong) (struct_offset)))
#define X_STRUCT_MEMBER(member_type, struct_p, struct_offset)   \
    (*(member_type*) X_STRUCT_MEMBER_P ((struct_p), (struct_offset)))

/* Provide simple macro statement wrappers:
 *   X_STMT_START { statements; } X_STMT_END;
 * This can be used as a single statement, like:
 *   if (x) X_STMT_START { ... } X_STMT_END; else ...
 * This intentionally does not use compiler extensions like GCC's '({...})' to
 * avoid portability issue or side effects when compiled with different compilers.
 */
#if !(defined (X_STMT_START) && defined (X_STMT_END))
#  define X_STMT_START  do
#  define X_STMT_END    while (0)
#endif

/*
 * The X_LIKELY and X_UNLIKELY macros let the programmer give hints to 
 * the compiler about the expected result of an expression. Some compilers
 * can use this information for optimizations.
 *
 * The _X_BOOLEAN_EXPR macro is intended to trigger a gcc warning when
 * putting assignments in x_return_if_fail ().  
 */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _X_BOOLEAN_EXPR(expr)               \
    X_EXTENSION ({                     \
                 int _x_boolean_var_;  \
                 if (expr)             \
                 _x_boolean_var_ = 1;  \
                 else                  \
                 _x_boolean_var_ = 0;  \
                 _x_boolean_var_;      \
                 })
#define X_LIKELY(expr) (__builtin_expect (_X_BOOLEAN_EXPR(expr), 1))
#define X_UNLIKELY(expr) (__builtin_expect (_X_BOOLEAN_EXPR(expr), 0))
#else
#define X_LIKELY(expr) (expr)
#define X_UNLIKELY(expr) (expr)
#endif

#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define X_DEPRECATED __attribute__((__deprecated__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#define X_DEPRECATED __declspec(deprecated)
#else
#define X_DEPRECATED
#endif

#if    __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define X_DEPRECATED_FOR(f) __attribute__((__deprecated__("Use '" #f "' instead")))
#elif defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define X_DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define X_DEPRECATED_FOR(f) X_DEPRECATED
#endif

#if    __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define X_ALIGN(n)  __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#define X_ALIGN(n)  __declspec(align(n))
#else
#define X_ALIGN(n)
#endif

#if    __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define X_UNAVAILABLE(maj,min) __attribute__((deprecated("Not available before " #maj "." #min)))
#elif defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define X_UNAVAILABLE(maj,min) __declspec(deprecated("is not available before " #maj "." #min))
#else
#define X_UNAVAILABLE(maj,min)
#endif

#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define X_BYTE_ORDER X_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define X_BYTE_ORDER X_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
#define X_BYTE_ORDER X_PDP_ENDIAN
#endif
#endif /* __BYTE_ORDER__ */

/* platform config */
#if defined (_WIN32) || defined(_WIN64)
#include "xcfg-win.h"
#elif defined (ANDROID)
#include "xcfg-android.h"
#elif defined (IPHONEOS)
#include "xcfg-ios.h"
#elif defined (DARWIN)
#include "xcfg-darwin.h"
#else
#error unknown platform
#endif

#ifndef X_BYTE_ORDER
#error undefined X_BYTE_ORDER
#endif

/* inlining hassle. for compilers that don't allow the `inline' keyword,
 * mostly because of strict ANSI C compliance or dumbness, we try to fall
 * back to either `__inline__' or `__inline'.
 * X_CAN_INLINE is defined in platform config. if the compiler seems to be 
 * actually *capable* to do function inlining, in which case inline 
 * function bodies do make sense. we also define X_INLINE_FUNC to properly 
 * export the function prototypes if no inlining can be performed.
 * inline function bodies have to be special cased with X_CAN_INLINE and a
 * .c file specific macro to allow one compiled instance with extern linkage
 * of the functions by defining X_IMPLEMENT_INLINES and the .c file macro.
 */
#if defined (__GNUC__) && defined (__STRICT_ANSI__)
#  undef inline
#  define inline __inline__
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#  undef inline
#  define inline __forceinline
#elif !defined (X_HAVE_INLINE)
#  undef inline
#  if defined (X_HAVE___INLINE__)
#    define inline __inline__
#  elif defined (X_HAVE___INLINE)
#    define inline __inline
#  else /* !inline && !__inline__ && !__inline */
#    define inline  /* don't inline, then */
#  endif
#endif

#ifdef X_IMPLEMENT_INLINES
#  define X_INLINE_FUNC
#  undef  X_CAN_INLINE
#elif defined (__GNUC__) 
#  define X_INLINE_FUNC static __inline __attribute__ ((unused))
#elif defined (X_CAN_INLINE) 
#  define X_INLINE_FUNC static inline
#else /* can't inline */
#  define X_INLINE_FUNC
#endif /* !X_INLINE_FUNC */

#if defined (__GNUC__) 
#define X_INTERN_FUNC __attribute__ ((visibility ("hidden")))
#define X_INTERN_VAR __attribute__ ((visibility ("hidden")))
#else
#define X_INTERN_FUNC
#define X_INTERN_VAR
#endif

#define X_MININT32	((xint32)  0x80000000)
#define X_MAXINT32	((xint32)  0x7fffffff)
#define X_MAXUINT32 ((xuint32) 0xFFFFFFFF)

#endif /* __X_CONFIG_H__ */
