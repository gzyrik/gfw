#include <xos/xtest.h>
void script_test();
int main(int argc, char **argv)
{
    x_test_init(argc,argv);
    x_test_add_func("/xrt/scirpt", script_test);
    x_test_run();
    return 0;
}
