#include "add-test.h"
struct test {
    xstr                path;
    xstr                description;
    xsize               data_size;
    xcptr               test_data;
    xTestFunc           setup_func;
    xTestFunc           test_func;
    xTestFunc           clear_func;
};
static  xSList *_tests;
AddTest::AddTest(const char* path, void (*func)(), const char* description)
{
    struct test *t;
    t = x_slice_new0 (struct test);
    t->path = x_strdup(path);
    t->description = x_strdup(description);
    t->test_func = (xTestFunc)func;
    _tests = x_slist_prepend(_tests, t);
}
static void setup(struct test *t)
{
    x_test_add_vtable(t->path, t->description,
                      t->data_size, t->test_data,
                      t->setup_func, t->test_func,t->clear_func);
}
int main(int _argc, char *argv[])
{
    xOptionContext *context = NULL;  
    xsize argc = _argc;
    context = x_option_context_new("xos self test", "test xos", "my-add-summary",NULL);  
    x_option_context_add_group(context, x_test_options());
    x_option_context_add_group(context, x_i18n_options());
    x_option_context_parse(context, &argc, &argv, NULL);
    x_option_context_delete (context);
    
    _tests = x_slist_reverse(_tests);
    x_slist_foreach(_tests, X_VOID_FUNC(setup), NULL);
    x_test_run();
    
    return 0;
}
