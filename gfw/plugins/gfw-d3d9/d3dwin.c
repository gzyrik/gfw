#include "gfw-d3d9.h"
X_DEFINE_DYNAMIC_TYPE (D3dWnd, d3d_wnd, G_TYPE_TARGET);
xType
_d3d_wnd_register       (xTypeModule    *module)
{
    return d3d_wnd_register (module);
}
enum {
    PROP_0,
    PROP_HWND,
    PROP_PHWND,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_FULLSCREEN,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    D3dWnd *wnd = (D3dWnd*)object;
}
static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    D3dWnd *wnd = (D3dWnd*)object;
}
static LRESULT CALLBACK d3d_wnd_proc (HWND, UINT, WPARAM, LPARAM);
static void
d3d_wnd_swap            (gTarget        *target)
{
    D3dWnd *wnd = (D3dWnd*) target;
    if (wnd->swap_chain) {
        TRACE_CALL (IDirect3DSwapChain9_Present,
                    "%p",
                    (wnd->swap_chain, NULL, NULL, NULL, NULL, 0));
    }
    else {
        TRACE_CALL (IDirect3DDevice9_Present,
                    "%p, %p, %p, %d, %p",
                    (_d3d9_render->device, NULL, NULL, 0, NULL));
    }
}
static void
d3d_wnd_init            (D3dWnd         *wnd)
{

}
static xptr
constructor             (xType          type,
                         xsize          n_properties,
                         xConstructParam *params)
{
    RECT rt;
    xsize i;
    HWND  hwnd, phwnd;
    xuint  w, h;
    xbool fullscreen;
    D3dWnd  *wnd;

    wnd = (D3dWnd*) x_instance_new (type);
    for (i=0; i< n_properties; ++i) {
        switch (params[i].pspec->param_id) {
        case PROP_HWND:
            hwnd = x_value_get_ptr (params[i].value);
            break;
        case PROP_PHWND:
            phwnd = x_value_get_ptr (params[i].value);
            break;
        case PROP_WIDTH:
            w = x_value_get_uint (params[i].value);
            break;
        case PROP_HEIGHT:
            h =  x_value_get_uint (params[i].value);
            break;
        case PROP_FULLSCREEN:
            fullscreen = x_value_get_bool (params[i].value);
        }
    }

    if (!hwnd) {
        static ATOM wndId = 0;
        DWORD dwStyle = WS_VISIBLE | WS_CLIPCHILDREN;
        DWORD dwStyleEx = 0;
        HINSTANCE instance = _module_instance ();

        if (!fullscreen) {
            if (phwnd != 0)
                dwStyle |= WS_CHILD;
            else
                dwStyle |= WS_OVERLAPPEDWINDOW;
            rt.left = rt.top = 0;
            rt.right = w;
            rt.bottom = h;
            AdjustWindowRect(&rt, dwStyle, FALSE);
            w = rt.right - rt.left;
            h = rt.bottom - rt.top;
        }
        else {
            dwStyle |= WS_POPUP;
            dwStyleEx |= WS_EX_TOPMOST;
        }

        if (!wndId) {
            WNDCLASSW wc = {
                CS_OWNDC, d3d_wnd_proc, 0,0, instance,
                LoadIcon(0, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW),
                (HBRUSH)GetStockObject(BLACK_BRUSH), NULL, L"D3DWnd"
            };
            wndId = RegisterClassW (&wc);
            if (wndId == 0)
                x_error ("RegisterClass failed");
        }
        wnd->hwnd = CreateWindowExW (dwStyleEx, L"D3DWnd", L"D3DWnd", dwStyle,
                                     CW_USEDEFAULT, CW_USEDEFAULT, w, h,
                                     phwnd, NULL, instance, wnd);
        wnd->external = FALSE;
    }
    else {
        wnd->hwnd = hwnd;
        wnd->external = TRUE;
    }
    ShowWindow(wnd->hwnd, SW_NORMAL);
    GetClientRect(wnd->hwnd, &rt);
    g_target_resize(G_TARGET(wnd), rt.right, rt.bottom);
    return wnd;
}
static void
finalize                (xObject        *object)
{
}
static void
d3d_wnd_class_init      (D3dWndClass    *klass)
{
    gTargetClass *tclass;
    xObjectClass *oclass;
    xParam *param;

    oclass = X_OBJECT_CLASS (klass);
    oclass->constructor  = constructor;
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = finalize;

    tclass = (gTargetClass*)klass;
    tclass->swap  = d3d_wnd_swap;

    param = x_param_ptr_new ("hwnd","hwnd","hwnd",
                             X_PARAM_STATIC_STR | X_PARAM_WRITABLE
                             | X_PARAM_CONSTRUCT_ONLY);
    x_oclass_install_param(oclass, PROP_HWND, param);

    param = x_param_ptr_new ("parent","parent","parent",
                             X_PARAM_STATIC_STR | X_PARAM_WRITABLE
                             | X_PARAM_CONSTRUCT_ONLY);
    x_oclass_install_param(oclass, PROP_PHWND, param);

    param = x_param_uint_new ("width","width","width",
                              0, X_MAXUINT32, 400,
                              X_PARAM_STATIC_STR | X_PARAM_WRITABLE
                              | X_PARAM_CONSTRUCT_ONLY);
    x_oclass_install_param(oclass, PROP_WIDTH, param);

    param = x_param_uint_new ("height","height","height",
                              0, X_MAXUINT32, 300,
                              X_PARAM_STATIC_STR | X_PARAM_WRITABLE
                              | X_PARAM_CONSTRUCT_ONLY);
    x_oclass_install_param(oclass, PROP_HEIGHT, param);

    param = x_param_bool_new ("fullscreen","fullscreen","fullscreen", FALSE,
                              X_PARAM_STATIC_STR | X_PARAM_WRITABLE
                              | X_PARAM_CONSTRUCT_ONLY);
    x_oclass_install_param(oclass, PROP_FULLSCREEN, param);
}
static void
d3d_wnd_class_finalize  (D3dWndClass    *klass)
{

}

static LRESULT CALLBACK 
d3d_wnd_proc (HWND hwnd, UINT Msg, WPARAM wparam, LPARAM lparam)
{
    switch (Msg) {
    case WM_CLOSE:
        DestroyWindow (hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW (hwnd, Msg, wparam, lparam);
}
