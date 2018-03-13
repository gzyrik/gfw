#include "add-test.h"
static void alloca_test ()
{
    xptr p = x_alloca(64);
    x_assert(p != NULL);
}
ADD_TEST("/xos/alloca", alloca_test);
