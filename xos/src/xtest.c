#include "config.h"
#include "xtest.h"
#include "xstr.h"
#include "xmem.h"
#include "xprintf.h"
#include "xutil.h"
#include "xslice.h"
#include "xslist.h"
#include "xatomic.h"
#include "xfile.h"
#include "xthread.h"
#include "xoption.h"
#include "xthread.h"
#include "xiconv.h"
#include "xwchar.h"
#include <stdlib.h>
#include <string.h>
#ifdef X_OS_WINDOWS
#include <crtdbg.h>
#define _WIN32_WINDOWS 0x0401 /* to get IsDebuggerPresent */
#include <Windows.h>
#endif
#include <setjmp.h>
static xPrivate _test_env;
static xPrivate _test_run_name;
static xbool    _test_opt_keep_going;
static xbool    _test_opt_list;
static xstrv    _test_opt_paths;
static xstrv    _test_opt_paths_skipped;
static xsize    _test_path_max_length;
static xTestSuite *_suite_root;
X_RWLOCK_DEFINE_STATIC(test_suites);
X_RWLOCK_DEFINE_STATIC(test_cases);
struct _xTestCase
{
    xstr        name;
    xstr        description;
    xint        ref_count;
    xsize       fixture_size;
    xTestFunc   fixture_setup;
    xTestFunc   fixture_test;
    xTestFunc   fixture_teardown;
    xcptr       test_data;
};
struct _xTestSuite
{
    xstr        name;
    xstr        description;
    xint        ref_count;
    xSList      *suites;
    xSList      *cases;
};
typedef struct _DestroyEntry    DestroyEntry;
struct _DestroyEntry {
    DestroyEntry      *next;
    xFreeFunc         free_func;
    xptr              destroy_data;
};

xstr __x_assert_msg = NULL;
void
x_assertion_expr        (xcstr          domain,
                         xcstr          file,
                         xint           line,
                         xcstr          func,
                         xcstr          expr)
{
    xstr s = x_strcat ("assertion failed: (", expr, ")", NULL);
    x_assertion_message (domain, file, line, func, s);
    x_free (s);
}
void
x_assertion_cmpstr      (xcstr          domain,
                         xcstr          file,
                         xint           line,
                         xcstr          func,
                         xcstr          expr,
                         xcstr          arg1,
                         xcstr          cmp,
                         xcstr          arg2)
{
    char *a1, *a2, *s, *t1 = NULL, *t2 = NULL;
    a1 = arg1 ? x_strcat ("\"", t1 = x_stresc (arg1, NULL), "\"", NULL) : x_strdup ("NULL");
    a2 = arg2 ? x_strcat ("\"", t2 = x_stresc (arg2, NULL), "\"", NULL) : x_strdup ("NULL");
    x_free (t1);
    x_free (t2);
    s = x_strdup_printf ("assertion failed (%s): (%s %s %s)", expr, a1, cmp, a2);
    x_free (a1);
    x_free (a2);
    x_assertion_message (domain, file, line, func, s);
    x_free (s);
}


void
x_assertion_cmpnum      (xcstr          domain,
                         xcstr          file,
                         xint           line,
                         xcstr          func,
                         xcstr          expr,
                         xdouble        arg1,
                         xcstr          cmp,
                         xdouble        arg2,
                         xchar          numtype)
{
    char *s = NULL;
    switch (numtype) {
    case 'i':
        s = x_strdup_printf ("assertion failed (%s): (%.0f %s %.0f)",
                             expr, arg1, cmp, arg2);
        break;
    case 'x':
        s = x_strdup_printf ("assertion failed (%s): (0x%08"
                             X_INT64_MODIFIER "x %s 0x%08"
                             X_INT64_MODIFIER "x)",
                             expr, (xuint64) arg1, cmp, (xuint64) arg2);
        break;
    case 'f':
        s = x_strdup_printf ("assertion failed (%s): (%.9g %s %.9g)",
                             expr, arg1, cmp, arg2);
        break;
        /* ideally use: floats=%.7g double=%.17g */
    }
    x_assertion_message (domain, file, line, func, s);
    x_free (s);
}

void
x_assertion_message     (xcstr          domain,
                         xcstr          file,
                         xint           line,
                         xcstr          func,
                         xcstr          message)
{
    char lstr[32];
    char *s;

    if (!message)
        message = "code should not be reached";
    x_snprintf (lstr, 32, "%d", line);
    s = x_strcat (domain ? domain : "", domain && domain[0] ? ":" : "",
                  "ERROR:", file, ":", lstr, ":",
                  func, func[0] ? ":" : "",
                  " ", message, NULL);
    x_printf ("**\n%s\n", s);

    /* store assertion message in global variable, so that it can be found in a
     * core dump */
    if (__x_assert_msg != NULL)
        /* free the old one */
        free (__x_assert_msg);
    __x_assert_msg = (char*) malloc (strlen (s) + 1);
    strcpy (__x_assert_msg, s);

    //x_test_log (X_TEST_LOG_ERROR, s, NULL, 0, NULL);
#ifdef X_OS_WINDOWS
    OutputDebugStringA(s);
#endif
    x_free (s);
    if(_test_opt_keep_going) {
        jmp_buf *env = x_private_get(&_test_env);
        if (env) longjmp(*env, 1);
    }
#ifdef X_OS_WINDOWS
    MessageBoxW (NULL, x_str_to_utf16 (__x_assert_msg, -1,NULL,NULL,NULL),
                 NULL, MB_ICONERROR|MB_SETFOREGROUND);
    if (IsDebuggerPresent ()) {
        X_BREAK_POINT();
        return;
    }
#endif
    abort();
}


static xstr         _test_uri_base;
static xbool        _test_run_result;
static xSList       *_test_destroy_queue;
typedef enum {
    TEST_NONE,
    TEST_ERROR,             /* s:msg */
    TEST_START_BINARY,      /* s:binaryname s:seed */
    TEST_LIST_CASE,         /* s:testpath */
    TEST_SKIP_CASE,         /* s:testpath */
    TEST_START_CASE,        /* s:testpath */
    TEST_STOP_CASE,         /* d:status d:nforks d:elapsed */
    TEST_MIN_RESULT,        /* s:blurb d:result */
    TEST_MAX_RESULT,        /* s:blurb d:result */
    TEST_MESSAGE            /* s:blurb */
} TestFlags;

static void
test_log                (TestFlags          lbit,
                         xcstr              string1,
                         xcstr              string2,
                         ...)
{

}
xOptionGroup*
x_test_options      (void)
{
    static xOptionGroup *test_opts;
    if (!test_opts) {
        xOption entries[] =  {
            {"keep-going", 'k', 0, X_OPTION_NONE,
                &_test_opt_keep_going,
                NULL, NULL},
            {"list", 'l', 0, X_OPTION_NONE,
                &_test_opt_list,
                "List test cases available in a test executable", NULL},
            {"path", 'p', 0, X_OPTION_STR_ARRAY,
                &_test_opt_paths,
                "Only start test cases matching TESTPATH", "TESTPATH"},
            {NULL}
        };
        test_opts = x_option_group_new("test", "Test Help Options:", "Show unit test help options", X_I18N_DOMAIN);
        x_option_group_add_entries(test_opts,entries);
    }
    return test_opts;
}

static xbool
test_case_run           (xTestCase      *test_case)
{
    xstr    old_run_name, new_run_name;
    xbool   success = TRUE;
    jmp_buf env;


    old_run_name = x_private_get(&_test_run_name);
    new_run_name = x_strcat (old_run_name, "/", test_case->name, NULL);
    x_private_set(&_test_run_name, new_run_name);
    if (_test_opt_list) {
        x_printf ("%-*s%s\n", (xint)_test_path_max_length+4, new_run_name, test_case->description);
        test_log (TEST_LIST_CASE, new_run_name, NULL, 0, NULL);
    }
    else {
        DestroyEntry    *destroy_queue;
        long double     largs[3];
        xTimer run_timer;
        void *fixture;
        int ret;

        test_log (TEST_START_CASE, new_run_name, NULL, 0, NULL);
        _test_run_result = FALSE;

        fixture = test_case->fixture_size ? 
            x_malloc0 (test_case->fixture_size) : (xptr)test_case->test_data;
        x_private_set(&_test_env, &env);
        ret = setjmp (env);
        if (0==ret) {
            x_timer_start (&run_timer);
            if (test_case->fixture_setup)
                test_case->fixture_setup (fixture, test_case->test_data);
            test_case->fixture_test (fixture, test_case->test_data);

            if (test_case->fixture_teardown)
                test_case->fixture_teardown (fixture, test_case->test_data);
            if (test_case->fixture_size)
                x_free (fixture);
            x_timer_stop (&run_timer);
        }
        x_private_set(&_test_env, NULL);

        /* 清理延时销毁的数据 */
        destroy_queue = (DestroyEntry*)_test_destroy_queue;
        _test_destroy_queue = NULL;
        while (destroy_queue) {
            DestroyEntry *dentry = destroy_queue;
            destroy_queue = dentry->next;
            dentry->free_func (dentry->destroy_data);
            x_slice_free (DestroyEntry, dentry);
        }

        success = _test_run_result;
        largs[0] = success ? 0 : 1; /* OK */
        largs[1] = 0;
        largs[2] = (long double)x_timer_elapsed (&run_timer);
        test_log (TEST_STOP_CASE, NULL, NULL, X_N_ELEMENTS (largs), largs);
    }
    x_private_set(&_test_run_name, old_run_name);
    x_free (new_run_name);

    return success;
}
static xsize
test_suite_run          (xTestSuite *suite,
                         xcstr      path)
{
    xsize n_bad = 0, l;
    xchar *rest, *old_name = x_private_get(&_test_run_name);
    xSList *slist, *reversed;

    x_return_val_if_fail (suite != NULL, -1);

    while (path[0] == '/')
        path++;
    l = strlen (path);
    rest = strchr (path, '/');
    l = rest ? MIN (l, (xsize)(rest - path)) : l;

    if (!suite->name || !suite->name[0])
        x_private_set(&_test_run_name, x_strdup(old_name));
    else
        x_private_set(&_test_run_name, x_strcat (old_name, "/", suite->name, NULL));

    X_READ_LOCK(test_cases);
    reversed = x_slist_copy (suite->cases);
    X_READ_UNLOCK(test_cases);

    reversed = x_slist_reverse (reversed);
    for (slist = reversed; slist; slist = slist->next) {
        xTestCase *tc = slist->data;
        xsize n = l ? strlen (tc->name) : 0;
        if (l == n && strncmp (path, tc->name, n) == 0) {
            if (!test_case_run (tc))
                n_bad++;
        }
    }
    x_slist_free (reversed);

    X_READ_LOCK(test_suites);
    reversed = x_slist_copy (suite->suites);
    X_READ_UNLOCK(test_suites);

    reversed = x_slist_reverse (reversed);
    for (slist = reversed; slist; slist = slist->next) {
        xTestSuite *ts = slist->data;
        xsize n = l ? strlen (ts->name) : 0;
        if (l == n && strncmp (path, ts->name, n) == 0)
            n_bad += test_suite_run (ts, rest ? rest : "");
    }
    x_slist_free (reversed);

    x_free (x_private_get(&_test_run_name));
    x_private_set(&_test_run_name, old_name);
    return n_bad;
}

xTestSuite*
x_test_root             (void)
{
    if (!x_atomic_ptr_get (&_suite_root)
        && x_once_init_enter (&_suite_root) ){
        _suite_root = x_slice_new0 (xTestSuite);
        _suite_root->name = "";
        _suite_root->ref_count = 1;
        _suite_root->description = "root";
        x_once_init_leave(&_suite_root);
    }
    return _suite_root;
}
xsize
x_test_run_suite        (xTestSuite     *suite)
{
    xsize n_bad = 0;
    xstr old_name;
    x_return_val_if_fail (suite != NULL, -1);

    if (!_test_opt_paths)
        _test_opt_paths = x_strv_new("",NULL);
    
    x_assert(x_private_get(&_test_run_name) == NULL);
    old_name = x_strdup("");
    x_private_set(&_test_run_name, old_name);
    while (*_test_opt_paths) {
        xsize l,n;
        xcstr rest;
        xstr p = *_test_opt_paths++;
        xcstr path = p;
        n = strlen (suite->name);
        while (path[0] == '/')
            path++;
        if (!n) {/* root suite, run unconditionally */
            n_bad += test_suite_run (suite, path);
            x_free(p);
            continue;
        }
        /* regular suite, match path */
        rest = strchr (path, '/');
        l = strlen (path);
        l = rest ? MIN (l, (xsize)(rest - path)) : l;
        if ((!l || l == n) && strncmp (path, suite->name, n) == 0)
            n_bad += test_suite_run (suite, rest ? rest : "");
        x_free(p);
    }
    x_assert(x_private_get(&_test_run_name) == old_name);
    x_private_set(&_test_run_name, NULL);
    x_free(old_name);
    return n_bad;

}

void
x_test_add_vtable       (xcstr          test_path,
                         xcstr          description,
                         xsize          data_size,
                         xcptr          test_data,
                         xTestFunc      data_setup,
                         xTestFunc      test_func,
                         xTestFunc      data_clean)
{
    xstrv segments;
    xuint i;
    xTestSuite *suite;
    xsize path_length=0;

    x_return_if_fail (test_path != NULL);
    x_return_if_fail (test_func != NULL);
    x_return_if_fail (x_path_is_absolute (test_path));

    while (test_path[0] == '/') test_path++;
    x_return_if_fail (test_path[0] != 0);

    if (x_strv_find ((xcstrv)_test_opt_paths_skipped, test_path) > 0)
        return;

    suite = x_test_root();
    segments = x_strsplit (test_path, "/", -1);
    for (i = 0; segments[i] != NULL; i++) {
        xcstr seg = segments[i];
        xbool islast = segments[i + 1] == NULL;

        if (islast && !seg[0])
            x_error ("invalid test case path: %s", test_path);
        else if (!seg[0])
            continue;       /* initial or duplicate slash */
        else if (!islast) {
            xTestSuite *ts = x_test_suite_ref(suite, seg);
            if (!ts) {
                x_test_suite_new(suite, seg, NULL);
                ts = x_test_suite_ref(suite, seg);
            }
            x_test_suite_unref(suite);
            suite = ts;
            path_length += x_str_width(seg) + 1;
        }
        else {/* islast */
            x_test_case_new (suite, seg, description,
                             data_size, test_data,
                             data_setup, test_func, data_clean);
            x_test_suite_unref(suite);
            path_length += x_str_width(seg);
        }
    }
    x_strv_free (segments);
    if (path_length > _test_path_max_length)
        _test_path_max_length = path_length;
}
static xint suite_cmp(xTestSuite *ts, xcstr suite_name)
{
    return strcmp(ts->name, suite_name);
}
static xint case_cmp(xTestCase *tc, xcstr test_name)
{
    return strcmp(tc->name, test_name);
}
void
x_test_suite_new            (xTestSuite     *parent,
                             xcstr          suite_name,
                             xcstr          description)
{

    x_return_if_fail (parent != NULL);
    x_return_if_fail (suite_name != NULL);
    x_return_if_fail (strchr (suite_name, '/') == NULL);
    x_return_if_fail (suite_name[0] != 0);

    X_WRITE_LOCK(test_suites);
    if (!x_slist_find_full(parent->suites, suite_name, X_INT_FUNC(suite_cmp))) {
        xTestSuite *ts = x_slice_new0 (xTestSuite);
        ts->ref_count = 1;
        ts->name = x_strdup(suite_name);
        ts->description = x_strdup(description);
        parent->suites = x_slist_prepend (parent->suites, ts);
    }
    X_WRITE_UNLOCK(test_suites);
}
xTestSuite*
x_test_suite_ref            (xTestSuite     *parent,
                             xcstr          suite_path)
{
    xTestSuite *ts;
    xSList *node;
    xcstr p;

    x_return_val_if_fail (parent != NULL, NULL);
    x_return_val_if_fail (suite_path != NULL, NULL);
    x_return_val_if_fail (suite_path[0] != 0, NULL);

    p = strrchr(suite_path, '/');
    if (p != NULL) {
        xstr suite_name = x_strndup(suite_path, p-suite_path);
        parent = x_test_suite_ref (parent, suite_name);
        x_free(suite_name);
        if (!parent) return NULL;
        suite_path = p+1;
    }

    X_READ_LOCK(test_suites);
    node = x_slist_find_full(parent->suites, suite_path, X_INT_FUNC(suite_cmp));
    if (node) {
        ts = node->data;
        if (x_atomic_int_get (&ts->ref_count) > 0)
            x_atomic_int_inc (&ts->ref_count);
        else
            ts = NULL;
    }
    else
        ts = NULL;
    X_READ_UNLOCK(test_suites);
    if (p != NULL)
        x_test_suite_unref(parent);
    return ts;
}
int
x_test_suite_unref      (xTestSuite     *test_suite)
{
    xint ref_count;

    if (test_suite == _suite_root) return 1; 
    x_return_val_if_fail (test_suite != NULL, -1);
    x_return_val_if_fail (x_atomic_int_get (&test_suite->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&test_suite->ref_count);
    if X_LIKELY(ref_count != 0)
        return ref_count;

    x_free(test_suite->name);
    x_free(test_suite->description);
    x_slist_free_full(test_suite->cases, X_VOID_FUNC(x_test_case_unref));
    x_slice_free(xTestSuite, test_suite);

    return ref_count;
}
void
x_test_case_new         (xTestSuite     *parent,
                         xcstr          test_name,
                         xcstr          description,
                         xsize          data_size,
                         xcptr          test_data,
                         xTestFunc      data_setup,
                         xTestFunc      test_func,
                         xTestFunc      data_clean)
{
    x_return_if_fail (parent != NULL);
    x_return_if_fail (test_name != NULL);
    x_return_if_fail (strchr (test_name, '/') == NULL);
    x_return_if_fail (test_name[0] != 0);
    x_return_if_fail (test_func != NULL);

    X_WRITE_LOCK(test_cases);
    if (!x_slist_find_full(parent->cases, test_name, X_INT_FUNC(case_cmp))) {
        xTestCase *tc = x_slice_new0 (xTestCase);
        tc->ref_count = 1;
        tc->name = x_strdup(test_name);
        tc->description = x_strdup(description);
        tc->test_data = (xptr) test_data;
        tc->fixture_size = data_size;
        tc->fixture_setup = (void*) data_setup;
        tc->fixture_test = (void*) test_func;
        tc->fixture_teardown = (void*) data_clean;
        parent->cases = x_slist_prepend (parent->cases, tc);
    }
    X_WRITE_UNLOCK(test_cases);
}
xTestCase*
x_test_case_ref         (xTestSuite     *parent,
                         xcstr          case_path)
{
    xTestCase *tc;
    xcstr p;
    xSList *node;

    x_return_val_if_fail (parent != NULL, NULL);
    x_return_val_if_fail (case_path != NULL, NULL);
    x_return_val_if_fail (case_path[0] != 0, NULL);

    p = strrchr(case_path, '/');
    if (p != NULL) {
        xstr suite_name = x_strndup(case_path, p-case_path);
        xTestSuite* ts = x_test_suite_ref (parent, suite_name);
        if (ts) {
            tc = x_test_case_ref (ts, p+1);
            x_test_suite_unref (ts);
        }
        else
            tc = NULL;
        x_free(suite_name);
        return tc;
    }

    X_READ_LOCK(test_cases);
    node = x_slist_find_full(parent->cases, case_path, X_INT_FUNC(case_cmp));
    if (node) {
        tc = node->data;
        if (x_atomic_int_get (&tc->ref_count) > 0)
            x_atomic_int_inc (&tc->ref_count);
        else
            tc = NULL;
    }
    else
        tc = NULL;
    X_READ_UNLOCK(test_cases);
    return tc;
}

int
x_test_case_unref       (xTestCase      *test_case)
{
    xint ref_count;
    x_return_val_if_fail (test_case != NULL, -1);
    x_return_val_if_fail (x_atomic_int_get (&test_case->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&test_case->ref_count);
    if X_LIKELY(ref_count != 0)
        return ref_count;

    x_free(test_case->name);
    x_free(test_case->description);
    x_slice_free(xTestCase, test_case);

    return ref_count;
}
