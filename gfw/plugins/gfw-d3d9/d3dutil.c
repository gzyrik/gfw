#include "gfw-d3d9.h"
HINSTANCE
_module_instance        (void)
{
    static HINSTANCE hInst=0;
    if(!hInst) {
        if(!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                              (LPCTSTR)&hInst, &hInst))
            x_error("GetModuleHandleEx:failed");
    }
    return hInst;
}
