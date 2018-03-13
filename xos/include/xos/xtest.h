#ifndef __X_TEST_H__
#define __X_TEST_H__
#include "xmsg.h"
X_BEGIN_DECLS
/* assertion API */
#define x_assert_str(s1, cmp, s2)    X_STMT_START {                     \
    const xchar *__s1 = (s1), *__s2 = (s2);                             \
    if (x_strcmp (__s1, __s2) cmp 0) ;else                              \
    x_assertion_cmpstr (X_LOG_DOMAIN, __FILE__, __LINE__,               \
                        X_STRFUNC, #s1 " " #cmp " " #s2,                \
                        __s1, #cmp, __s2);                              \
} X_STMT_END

#define x_assert_int(n1, cmp, n2)    X_STMT_START {                     \
    xint64 __n1 = (n1), __n2 = (n2);                                    \
    if (__n1 cmp __n2) ; else                                           \
    x_assertion_cmpnum (X_LOG_DOMAIN, __FILE__, __LINE__,               \
                        X_STRFUNC, #n1 " " #cmp " " #n2,                \
                        (xdouble)__n1, #cmp, (xdouble)__n2, 'i');       \
} X_STMT_END

#define x_assert_uint(n1, cmp, n2)   X_STMT_START {                     \
    xuint64 __n1 = (n1), __n2 = (n2);                                   \
    if (__n1 cmp __n2) ; else                                           \
    x_assertion_cmpnum (X_LOG_DOMAIN, __FILE__, __LINE__,               \
                        X_STRFUNC, #n1 " " #cmp " " #n2,                \
                        (xdouble)__n1, #cmp, (xdouble)__n2, 'i');       \
} X_STMT_END

#define x_assert_hex(n1, cmp, n2)    X_STMT_START {                     \
    xuint64 __n1 = (n1), __n2 = (n2);                                   \
    if (__n1 cmp __n2) ; else                                           \
    x_assertion_cmpnum (X_LOG_DOMAIN, __FILE__, __LINE__,               \
                        X_STRFUNC, #n1 " " #cmp " " #n2,                \
                        (xdouble)__n1, #cmp, (xdouble)__n2, 'x');       \
} X_STMT_END

#define x_assert_float(n1,cmp,n2)    X_STMT_START {                     \
    xdouble __n1 = (n1), __n2 = (n2);                                   \
    if (__n1 cmp __n2) ; else                                           \
    x_assertion_cmpnum (X_LOG_DOMAIN, __FILE__, __LINE__,               \
                        X_STRFUNC, #n1 " " #cmp " " #n2,                \
                        __n1, #cmp, __n2, 'f');                         \
} X_STMT_END

#define x_assert_no_error(err)      X_STMT_START {                      \
    if (err)                                                            \
    x_assertion_message_error (X_LOG_DOMAIN, __FILE__, __LINE__,        \
                               X_STRFUNC, #err, err, 0, 0);             \
}X_STMT_END

#define x_assert_error(err, dom, c)	X_STMT_START {                      \
    if (!err || (err)->domain != dom || (err)->code != c)               \
    x_assertion_message_error (X_LOG_DOMAIN, __FILE__, __LINE__,        \
                               X_STRFUNC,  #err, err, dom, c);          \
} X_STMT_END


#ifdef X_DISABLE_ASSERT
#define x_assert_not_reached()          X_STMT_START { (void) 0; } X_STMT_END
#define x_assert(expr)                  X_STMT_START { (void) 0; } X_STMT_END

#else /* !X_DISABLE_ASSERT */
#define x_assert_not_reached()          X_STMT_START {                  \
    x_assertion_message (X_LOG_DOMAIN, __FILE__, __LINE__,              \
                         X_STRFUNC, NULL);                              \
} X_STMT_END

#define x_assert(expr)                  X_STMT_START {                  \
    if X_LIKELY (expr) ; else                                           \
    x_assertion_expr (X_LOG_DOMAIN, __FILE__, __LINE__,                 \
                      X_STRFUNC, #expr);                                \
} X_STMT_END
#endif /* !X_DISABLE_ASSERT */

void        x_assertion_expr            (xcstr          domain,
                                         xcstr          file,
                                         xint           line,
                                         xcstr          func,
                                         xcstr          expr) X_NORETURN;

void        x_assertion_cmpstr          (xcstr          domain,
                                         xcstr          file,
                                         xint           line,
                                         xcstr          func,
                                         xcstr          expr,
                                         xcstr          arg1,
                                         xcstr          cmp,
                                         xcstr          arg2) X_NORETURN;


void        x_assertion_cmpnum          (xcstr          domain,
                                         xcstr          file,
                                         xint           line,
                                         xcstr          func,
                                         xcstr          expr,
                                         xdouble        arg1,
                                         xcstr          cmp,
                                         xdouble        arg2,
                                         xchar          numtype) X_NORETURN;

void        x_assertion_message         (xcstr          domain,
                                         xcstr          file,
                                         xint           line,
                                         xcstr          func,
                                         xcstr          expr) X_NORETURN;

/* unit test API */
typedef xbool   (*xTestLogFatalFunc)    (xcstr          log_domain,
                                         xLogLevelFlags log_level,
                                         xcstr          message,
                                         xptr           user_data);

/* void (*)(xptr fixture, xcptr user_data) */
typedef xVoidFunc   xTestFunc;

#define     x_test_add_func(test_path, test_func)                       \
    x_test_add_vtable (test_path, NULL, 0, NULL,                        \
                       NULL, X_VOID_FUNC(test_func), NULL)

#define     x_test_add_data_func(test_path, test_data, test_func)       \
    x_test_add_vtable (test_path, NULL, 0, test_data,                   \
                       NULL, X_VOID_FUNC(test_func), NULL)

#define     x_test_run()                                                \
    x_test_run_suite (x_test_root ())

xTestSuite* x_test_root                 (void);

/* return the number of failed case */ 
xsize       x_test_run_suite            (xTestSuite     *suite);

void        x_test_add_vtable           (xcstr          test_path,
                                         xcstr          description,
                                         xsize          data_size,
                                         xcptr          test_data,
                                         xTestFunc      test_setup,
                                         xTestFunc      test_func,
                                         xTestFunc      test_teardown);

void        x_test_suite_new            (xTestSuite     *parent,
                                         xcstr          suite_name,
                                         xcstr          description);

xTestSuite* x_test_suite_ref            (xTestSuite     *parent,
                                         xcstr          suite_path);

int         x_test_suite_unref          (xTestSuite     *suite);

void        x_test_case_new             (xTestSuite     *parent,
                                         xcstr          test_name,
                                         xcstr          description,
                                         xsize          data_size,
                                         xcptr          test_data,
                                         xTestFunc      test_setup,
                                         xTestFunc      test_func,
                                         xTestFunc      test_teardown);

xTestCase*  x_test_case_ref             (xTestSuite     *parent,
                                         xcstr          case_path);

int         x_test_case_unref           (xTestCase      *test_case);

xOptionGroup* x_test_options            (void);

X_END_DECLS
#endif /* __X_TEST_H__ */
