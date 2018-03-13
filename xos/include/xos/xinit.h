#ifndef __X_INIT_H__
#define __X_INIT_H__

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define X_HAS_CONSTRUCTORS 1

#define X_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define X_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER) && (_MSC_VER >= 1500)
/* Visual studio 2008 and later has _Pragma */

#define X_HAS_CONSTRUCTORS 1

#define X_DEFINE_CONSTRUCTOR(_func) \
    static void _func(void); \
static int _func ## _wrapper(void) { _func(); return 0; } \
__pragma(section(".CRT$XCU",read)) \
__declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _wrapper;

#define X_DEFINE_DESTRUCTOR(_func) \
    static void _func(void); \
static int _func ## _constructor(void) { atexit (_func); return 0; } \
__pragma(section(".CRT$XCU",read)) \
__declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#elif defined (_MSC_VER)

#define X_HAS_CONSTRUCTORS 1

/* Pre Visual studio 2008 must use #pragma section */
#define X_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define X_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define X_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
    section(".CRT$XCU",read)
#define X_DEFINE_CONSTRUCTOR(_func) \
    static void _func(void); \
static int _func ## _wrapper(void) { _func(); return 0; } \
__declspec(allocate(".CRT$XCU")) static int (*p)(void) = _func ## _wrapper;

#define X_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
    section(".CRT$XCU",read)
#define X_DEFINE_DESTRUCTOR(_func) \
    static void _func(void); \
static int _func ## _constructor(void) { atexit (_func); return 0; } \
__declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define X_HAS_CONSTRUCTORS 1

#define X_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define X_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define X_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
    init(_func)
#define X_DEFINE_CONSTRUCTOR(_func) \
    static void _func(void);

#define X_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
    fini(_func)
#define X_DEFINE_DESTRUCTOR(_func) \
    static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#endif /* __X_INIT_H__ */
