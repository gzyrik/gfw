#include "gfw-octree.h"
xbool X_MODULE_EXPORT
type_module_main        (xTypeModule    *module)
{
    _oct_node_register (module);
    _oct_ray_query_register (module);
    _oct_scene_register (module);
    return TRUE;
}
