#include "gfw-d3d9.h"
D3d9Render *_d3d9_render;
X_DEFINE_DYNAMIC_TYPE (D3d9Render, d3d9_render, G_TYPE_RENDER);
xType
_d3d9_render_register    (xTypeModule    *module)
{
    return d3d9_render_register (module);
}
static void
d3d9_set_render         (D3DRENDERSTATETYPE type,
                         DWORD          value)
{
    DWORD oldval;
    TRACE_CALL (IDirect3DDevice9_GetRenderState,
                "%p,%d,%p",
                (_d3d9_render->device, type, &oldval));
    if (oldval == value)
        return;
    TRACE_CALL (IDirect3DDevice9_SetRenderState,
                "%p,%d,%d",
                (_d3d9_render->device, type, value));
}
static void
d3d9_set_sampler        (DWORD          sampler,
                         D3DSAMPLERSTATETYPE type,
                         DWORD          value)
{
    DWORD oldval;
    TRACE_CALL (IDirect3DDevice9_GetSamplerState,
                "%p,%d,%d,%p",
                (_d3d9_render->device, sampler, type, &oldval));
    if (oldval == value)
        return;
    TRACE_CALL (IDirect3DDevice9_SetSamplerState,
                "%p,%d,%d,%d",
                (_d3d9_render->device, sampler, type, value));
}
enum {
    PROP_0,
    PROP_DEVICE,
    PROP_HWND,
    PROP_PHWND,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_FULLSCREEN,
    PROP_VSYNC,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    D3d9Render *d3d = (D3d9Render*)object;
}
static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    D3d9Render *d3d = (D3d9Render*)object;
    switch (property_id) {
    case PROP_DEVICE:
        x_value_set_ptr (value, d3d->device);
        break;
    }
}

static void
d3d9_render_begin       (gRender        *render)
{
    D3d9Render* d3d9 = (D3d9Render*)render;
    TRACE_CALL (IDirect3DDevice9_BeginScene,
                "%p",
                (d3d9->device));
}
static void
d3d9_render_clear       (gRender        *render,
                         gViewport      *viewport)
{
    D3d9Render* d3d9 = (D3d9Render*)render;
    DWORD flags;
    D3DCOLOR color;

    flags = 0;
    color = g_vec_color(viewport->color);
    if (viewport->buffers & G_FRAME_COLOR)
        flags |= D3DCLEAR_TARGET;
    if (viewport->buffers  & G_FRAME_DEPTH)
        flags |= D3DCLEAR_ZBUFFER;
    if (viewport->buffers  & G_FRAME_STENCIL)
        flags |= D3DCLEAR_STENCIL;
    if (d3d9->viewport != viewport) {
        xsize i;
        D3DVIEWPORT9 d3dvp;
        gTarget *target;
        LPDIRECT3DSURFACE9 pBack[64] = {0,};

        d3d9->viewport = viewport;
        target = viewport->target;
        pBack[0] = ((D3dWnd*)target)->back_surface;
        for (i=0; i<1; ++i) {
            TRACE_CALL (IDirect3DDevice9_SetRenderTarget,
                        "%p, %d, %p",
                        (d3d9->device, i, pBack[i]));
        }
         TRACE_CALL (IDirect3DDevice9_SetDepthStencilSurface,
                        "%p, %d, %p",
                        (d3d9->device, ((D3dWnd*)target)->z_buffer));
        d3dvp.X = viewport->x;
        d3dvp.Y = viewport->y;
        d3dvp.Width = viewport->w;
        d3dvp.Height = viewport->h;
        d3dvp.MinZ = 0;
        d3dvp.MaxZ = 1;
        TRACE_CALL (IDirect3DDevice9_SetViewport,
                    "%p, %p",
                    (d3d9->device, &d3dvp));
    }
    TRACE_CALL (IDirect3DDevice9_Clear,
                "%p, %d, %p,%d, %d, %f, %d)",
                (d3d9->device, 0, NULL, flags, color,
                 viewport->depth, viewport->stencil));
}
static void
d3d9_render_setup       (gRender        *render,
                         gPass          *pass)
{
    //TODO
    xbool flip = FALSE;
    d3d9_set_render (D3DRS_CULLMODE, _d3d_map_cull_mode(pass->cull_mode, flip));
    d3d9_set_render (D3DRS_FILLMODE, _d3d_map_fill_mode(pass->fill_mode));
}
static void
d3d9_render_end         (gRender        *render)
{
    D3d9Render* d3d9 = (D3d9Render*)render;
    TRACE_CALL (IDirect3DDevice9_EndScene,
                "%p",
                (d3d9->device));
}
static void
d3d9_render_vdeclares   (D3d9Render     *d3d9,
                         gVertexData    *vdata)
{
    LPDIRECT3DVERTEXDECLARATION9 decl;

    if (!vdata->elem_changed && vdata->vdeclare)
        decl = vdata->vdeclare;
    else {
        xArray *elements = vdata->elements;
        decl = x_hash_table_lookup (d3d9->vdeclares, elements);
        if (!decl) {
            xsize i;
            LPD3DVERTEXELEMENT9 d3delems = x_new(D3DVERTEXELEMENT9, elements->len+1);
            for (i=0; i< elements->len; ++i) {
                gVertexElem *elem = &x_array_index (elements, gVertexElem, i);
                d3delems[i].Method = D3DDECLMETHOD_DEFAULT;
                d3delems[i].Offset = elem->offset;
                d3delems[i].Stream = elem->source;
                d3delems[i].Type   = _d3d_map_vertex_type (elem->type);
                d3delems[i].Usage  = _d3d_map_vertex_semantic(elem->semantic);
                if (elem->semantic == G_VERTEX_SPECULAR)
                    d3delems[i].UsageIndex = 1;
                else if (elem->semantic  == G_VERTEX_DIFFUSE)
                    d3delems[i].UsageIndex = 0;
                else
                    d3delems[i].UsageIndex = elem->index;
            }
            d3delems[i].Stream = 0xff;
            d3delems[i].Offset = 0;
            d3delems[i].Type = D3DDECLTYPE_UNUSED;
            d3delems[i].Method = 0;
            d3delems[i].Usage = 0;
            d3delems[i].UsageIndex = 0;

            TRACE_CALL (IDirect3DDevice9_CreateVertexDeclaration,
                        "%p, %p, %p",
                        (d3d9->device, d3delems, &decl));
            x_free (d3delems);
            x_hash_table_insert (d3d9->vdeclares, x_array_dup(elements), decl);
        }
        vdata->vdeclare = decl;
        vdata->elem_changed = FALSE;
    }
    TRACE_CALL (IDirect3DDevice9_SetVertexDeclaration,
                "%p, %p",
                (d3d9->device, decl));
}
static void
d3d9_render_binding     (D3d9Render     *d3d9,
                         xPtrArray      *binding)
{
    xsize i,j;
    for (i=0; i<binding->len;++i) {
        D3d9VertexBuf *vbuf = binding->data[i];
        if (!vbuf) {
            TRACE_CALL (IDirect3DDevice9_SetStreamSource,
                        "%p,%d,%p,%d,%d",
                        (d3d9->device, i,NULL,0, 0));
        }
        else {
            g_buffer_upload ((gBuffer*)vbuf);
            TRACE_CALL (IDirect3DDevice9_SetStreamSource,
                        "%p,%d,%p,%d,%d",
                        (d3d9->device, i,vbuf->handle,0, vbuf->parent.vertex_size));

        }
    }
    for (j=i;j<d3d9->last_source; ++j) {
        TRACE_CALL (IDirect3DDevice9_SetStreamSource,
                    "%p,%d,%p,%d,%d",
                    (d3d9->device, j,NULL,0, 0));
    }
    d3d9->last_source = i;
}
static void
d3d9_render_draw        (gRender        *render,
                         gRenderOp      *op)
{
    D3DPRIMITIVETYPE primType;
    DWORD          primCount;
    D3d9Render* d3d9 = (D3d9Render*)render;

    x_return_if_fail (op->vdata != NULL);

    _d3d_map_primitive (op, &primType, &primCount);
    d3d9_render_vdeclares (d3d9, op->vdata);
    d3d9_render_binding (d3d9, op->vdata->binding);
    if (op->idata) {
        D3d9IndexBuf *ibuf = g_index_data_get (op->idata);
        g_buffer_upload ((gBuffer*)ibuf);
        TRACE_CALL (IDirect3DDevice9_SetIndices,
                    "%p,%p",
                    (d3d9->device, ibuf->handle));

        TRACE_CALL (IDirect3DDevice9_DrawIndexedPrimitive,
                    "%p, %d, %d, %d,%d, %d, %d",
                    (d3d9->device, primType, op->vdata->start,0, op->vdata->count, op->idata->start,primCount));
    }
    else {
        TRACE_CALL (IDirect3DDevice9_DrawPrimitive,
                    "%p,%d,%d,%d",
                    (d3d9->device, primType,op->vdata->start, primCount));

    }
}
static xptr
d3d9_render_constructor (xType          type,
                         xsize          n_properties,
                         xConstructParam *params)
{
    gRenderClass *klass;
    D3d9Render *d3d9;
    xsize i;
    HWND  hwnd, phwnd;
    xuint  w, h;
    xbool fullscreen;
    DWORD flags;
    xbool v_sync;
    D3DDISPLAYMODE d3ddm;

    _d3d9_render = d3d9 = (D3d9Render*) x_instance_new (type);
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
            break;
        case PROP_VSYNC:
            v_sync = x_value_get_bool (params[i].value);
            break;
        }
    }
    TRACE_CALL (IDirect3D9_GetAdapterDisplayMode,
                "%p, %d, %p",
                (d3d9->d3d9, D3DADAPTER_DEFAULT, &d3ddm));

    if (!h) h = d3ddm.Height;
    if (!w) w = d3ddm.Width;

    d3d9->params.BackBufferFormat = d3ddm.Format;
    d3d9->params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3d9->params.EnableAutoDepthStencil = TRUE;
    d3d9->params.BackBufferCount = v_sync ? 2 : 1;
    d3d9->params.hDeviceWindow = hwnd;
    d3d9->params.BackBufferFormat = d3ddm.Format;

    if (!fullscreen) {
        if (hwnd) {
            RECT rt;
            GetWindowRect(hwnd, &rt);
            w = rt.right - rt.left;
            h = rt.bottom - rt.top; 
        }
        d3d9->params.Windowed = TRUE;
        d3d9->params.BackBufferWidth = w;
        d3d9->params.BackBufferHeight = h;
        d3d9->params.FullScreen_RefreshRateInHz = 0;
    }
    else {
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);
        d3d9->params.Windowed = FALSE;
        d3d9->params.BackBufferWidth = w;
        d3d9->params.BackBufferHeight = h;
        d3d9->params.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
    }
    if (v_sync) {
        d3d9->params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
        d3d9->params.BackBufferCount = 2;
    }
    else {
        d3d9->params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        d3d9->params.BackBufferCount = 1;
    }

    /* Try to create a 32-bit depth, 8-bit stencil */
    if( FAILED( IDirect3D9_CheckDeviceFormat (d3d9->d3d9, D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,d3d9->params.BackBufferFormat,D3DUSAGE_DEPTHSTENCIL, 
        D3DRTYPE_SURFACE, D3DFMT_D24S8 ))){
        /* Bugger, no 8-bit hardware stencil, just try 32-bit zbuffer */
        if( FAILED( IDirect3D9_CheckDeviceFormat(d3d9->d3d9, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,  d3d9->params.BackBufferFormat,  D3DUSAGE_DEPTHSTENCIL, 
            D3DRTYPE_SURFACE, D3DFMT_D32)))
            d3d9->params.AutoDepthStencilFormat = D3DFMT_D16;
        else
            d3d9->params.AutoDepthStencilFormat = D3DFMT_D32;
    }
    else {
        if( SUCCEEDED( IDirect3D9_CheckDepthStencilMatch(d3d9->d3d9, D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
            d3d9->params.BackBufferFormat, d3d9->params.BackBufferFormat, D3DFMT_D24S8 ) ) )
            d3d9->params.AutoDepthStencilFormat = D3DFMT_D24S8;  
        else 
            d3d9->params.AutoDepthStencilFormat = D3DFMT_D24X8; 
    }

    klass = (gRenderClass*)X_INSTANCE_CLASS (d3d9);
    d3d9->window = x_object_new (klass->window,
                                 "hwnd", hwnd,
                                 "fullscreen", fullscreen,
                                 "width", w,
                                 "height", h,
                                 "parent", phwnd,
                                 NULL);
    g_render_attach (G_TARGET (d3d9->window));

    if ((d3d9->caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        == D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        flags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE;
    else
        flags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE;

    TRACE_CALL (IDirect3D9_CreateDevice,
                "%p, %d, %d, %p, %d, %p, %p",
                (d3d9->d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                 d3d9->window->hwnd, flags,
                 &d3d9->params, &d3d9->device));
    TRACE_CALL (IDirect3DDevice9_GetRenderTarget,
                "%p, %d, %p",
                (d3d9->device, 0, &d3d9->window->back_surface));
    TRACE_CALL (IDirect3DDevice9_GetDepthStencilSurface,
                "%p, %p",
                (d3d9->device, &d3d9->window->z_buffer));
    return d3d9;
}
static xuint
d3d9_decl_hash          (xArray         *elems)
{
    xsize i;
    xuint32 h = 5381;
    for (i=0; i<elems->len; ++i) {
        gVertexElem e = x_array_index (elems,gVertexElem, i);
        xuint u;
        u = e.offset | (e.source<<8) | (e.type<<16) | (e.semantic<<20)| (e.index<<24);
        h += (h<<5) + u;
    }
    return h;
}
static xint
d3d9_decl_cmp           (xArray         *e1,
                         xArray         *e2)
{
    if (e1->len < e2->len)
        return -1;
    else if (e1->len > e2->len)
        return 1;
    return memcmp (e1->data,e2->data, e1->len*sizeof(gVertexElem));
}
static void
d3d9_decl_free          (xptr           p)
{
    LPDIRECT3DVERTEXDECLARATION9 decl = p;
    TRACE_CALL (IDirect3DVertexDeclaration9_Release,
                "%p",
                (decl));
}
static void
d3d9_render_init        (D3d9Render     *d3d9)
{
    d3d9->d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    x_return_if_fail (d3d9->d3d9 != NULL); 

    TRACE_CALL (IDirect3D9_GetDeviceCaps,
                "%p, %d, %d, %p",
                (d3d9->d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3d9->caps));

    d3d9->vdeclares = x_hash_table_new_full ((xHashFunc)d3d9_decl_hash, (xCmpFunc)d3d9_decl_cmp,
                                             x_free, d3d9_decl_free);
}
static void
d3d9_render_finalize    (xObject        *object)
{
}
static void
d3d9_render_class_init  (D3d9RenderClass *klass)
{
    gRenderClass *rclass;
    xObjectClass *oclass;
    xParam *param;

    oclass = X_OBJECT_CLASS (klass);
    oclass->constructor  = d3d9_render_constructor;
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = d3d9_render_finalize;

    rclass = G_RENDER_CLASS (klass);
    rclass->window  = x_type_from_mime (G_TYPE_TARGET, "D3dWnd");
    rclass->texture = x_type_from_mime (G_TYPE_TEXTURE, "D3d9Tex");
    rclass->pbuf    = x_type_from_mime (G_TYPE_PIXEL_BUFFER, "D3d9PixelBuf");
    rclass->vbuf    = x_type_from_mime (G_TYPE_VERTEX_BUFFER, "D3d9VertexBuf");
    rclass->ibuf    = x_type_from_mime (G_TYPE_INDEX_BUFFER, "D3d9IndexBuf");
    rclass->begin   = d3d9_render_begin;
    rclass->clear   = d3d9_render_clear;
    rclass->end      = d3d9_render_end;
    rclass->setup = d3d9_render_setup;
    rclass->draw  = d3d9_render_draw;

    param = x_param_ptr_new ("device","device","device",
                             X_PARAM_STATIC_STR | X_PARAM_READABLE);
    x_oclass_install_param(oclass, PROP_DEVICE, param);

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

    param = x_param_bool_new ("vsync","vsync","vsync", FALSE,
                              X_PARAM_STATIC_STR | X_PARAM_WRITABLE
                              | X_PARAM_CONSTRUCT_ONLY);
    x_oclass_install_param(oclass, PROP_VSYNC, param);
}
static void
d3d9_render_class_finalize  (D3d9RenderClass *klass)
{

}

