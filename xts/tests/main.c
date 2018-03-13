#include <xos.h>
void figure_test();

int main(int argc, char **argv)
{
    x_test_init(argc,argv);
    x_test_add_func("/xts/type-module", figure_test);
    x_test_run();
    return 0;
}