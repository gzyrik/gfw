#include "gfwcg.h"
xbool X_MODULE_EXPORT
type_module_main        (xTypeModule    *module)
{
    _cg_program_register (module);
    return TRUE;
}
