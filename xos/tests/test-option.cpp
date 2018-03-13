#include "add-test.h"
#define OPT(n)              "arg"#n
#define ARG(n)              "--arg"#n
#define ARGV(n, v)          "--arg"#n"="#v

static void test_none ()
{
    xOptionContext *context = NULL;  
    xbool opt[3]={0,0,1};
    xOption entries[] =  {
        {OPT(0), 'a', 0, X_OPTION_NONE, &opt[0], NULL, NULL},
        {OPT(1), 'b', 0, X_OPTION_NONE, &opt[1], NULL, NULL},
        {OPT(2), 'c', X_OPTION_REVERSE_VALUE, X_OPTION_NONE, &opt[2], NULL, NULL},
        {NULL},
    };
    const char* argv0[]={"test", ARG(0),"-b", ARG(1),ARG(2), "-c"};
    xsize argc = X_N_ELEMENTS(argv0);
    xstrv argv = (xstrv)argv0;

    context = x_option_context_new(NULL,NULL,NULL,NULL);
    x_option_context_add_entries(context,entries);
    x_option_context_parse(context, &argc, &argv, NULL);
    x_option_context_delete (context);  

    x_assert(opt[0]);
    x_assert(opt[1]);
    x_assert(!opt[2]);
}

static void test_simple ()
{
    xOptionContext *context = NULL;  
    xstr  opt_str = NULL;
    xint  opt_int = 0;
    xdouble opt_double = 0;
    xint64  opt_i64 = 0;
    xstr opt_path = NULL;
    xOption entries[] =  {
        {OPT(0), 'a', 0, X_OPTION_STR, &opt_str, NULL, NULL},
        {OPT(1), 'b', 0, X_OPTION_INT, &opt_int, NULL, NULL},
        {OPT(2), 'c', 0, X_OPTION_DOUBLE, &opt_double, NULL, NULL},
        {OPT(3), 'd', 0, X_OPTION_INT64, &opt_i64, NULL, NULL},
        {OPT(4), 'e', 0, X_OPTION_PATH, &opt_path, NULL, NULL},
        {NULL},
    };
    const char* argv0[]={"",
        ARGV(0,arga),
        "-a", "argb",
        "-b", "3",
        ARGV(1,1),
        ARGV(2, 1.2343),
        ARGV(3, 0x12345678910),

    };
    xsize argc = X_N_ELEMENTS(argv0);
    xstrv argv = (xstrv)argv0;

    context = x_option_context_new(NULL,NULL,NULL,NULL);
    x_option_context_add_entries(context,entries);
    x_option_context_parse(context, &argc, &argv, NULL);
    x_option_context_delete (context);
    
    x_assert_str(opt_str, == , "argb");
    x_assert_int(opt_int, ==, 1);
    x_assert_float(opt_double, ==, 1.2343);
    x_assert_hex(opt_i64, ==, 0x12345678910);
}
ADD_TEST ("/xos/option/none", test_none, "General test for none  option");
ADD_TEST ("/xos/option/simple", test_simple, "General test for simple options");
