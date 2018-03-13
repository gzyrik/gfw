#include "gfw-d3d9.h"
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dxerr.lib")
#pragma comment(lib, "dxguid.lib")
xbool X_MODULE_EXPORT
type_module_main        (xTypeModule    *module)
{
    _d3d9_render_register (module);
    _d3d_wnd_register (module);
    _d3d9_tex_register (module);
    _d3d9_pixelbuf_register (module);
    _d3d9_vertexbuf_register (module);
    _d3d9_indexbuf_register (module);
    _d3d9_hlsl_register (module);
    return TRUE;
}
