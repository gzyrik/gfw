#include <gfw.h>

void script_test ()
{
    gvec a={1,2,3};
    gvec b={5,6,7};
    gvec c;

    c = g_vec3_cross(a, b);

    x_res_mgr_mount ("test","file:.");
    x_res_mgr_parse ("test");
    x_object_unref (g_material_get ("Example"));
}
