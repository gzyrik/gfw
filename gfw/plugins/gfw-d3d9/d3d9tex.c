#include "gfw-d3d9.h"
X_DEFINE_DYNAMIC_TYPE (D3d9Tex, d3d9_tex, G_TYPE_TEXTURE);
xType
_d3d9_tex_register       (xTypeModule    *module)
{
    return d3d9_tex_register (module);
}
static void
d3d9_set_stage          (D3d9Render     *d3d9,
                         DWORD          stage,
                         D3DSAMPLERSTATETYPE type,
                         DWORD          value)
{
    DWORD oldval;
    TRACE_CALL (IDirect3DDevice9_GetTextureStageState,
                "%p,%d,%d,%p",
                (d3d9->device, stage, type, &oldval));
    if (oldval == value)
        return;
    TRACE_CALL (IDirect3DDevice9_SetTextureStageState,
                "%p,%d,%d,%d",
                (d3d9->device, stage, type, value));
}
static void
d3d9_tex_bind           (gTexture       *tex,
                         xsize          index)
{
    D3d9Tex *d3d9tex = (D3d9Tex*)tex;
    TRACE_CALL(IDirect3DDevice9_SetTexture,
               "%p, %d, %p",
               (_d3d9_render->device, index, d3d9tex->tex));
    d3d9_set_stage (_d3d9_render, index, D3DTSS_TEXCOORDINDEX, index);
}
static void
d3d9_tex_unbind         (gTexture       *tex,
                         xsize          index)
{
    TRACE_CALL(IDirect3DDevice9_SetTexture,
               "%p, %d, %p",
               (_d3d9_render->device, index, NULL));
}
static void
create_surface_list     (D3d9Tex        *d3d9tex)
{
    xsize mip,face;
    IDirect3DSurface9   *surface;
    D3d9PixelBuf *pixel_buffer;
    gTexture *tex = (gTexture*)d3d9tex;
    gRenderClass *render_class;

    render_class = G_RENDER_GET_CLASS (_d3d9_render);
    x_assert (!tex->buffers);
    tex->mipmaps = IDirect3DBaseTexture9_GetLevelCount (d3d9tex->tex) - 1;

    switch(tex->type) {
    case G_TEXTURE_2D:
    case G_TEXTURE_1D:
        x_assert (d3d9tex->norm_tex);
        tex->faces = 1;
        tex->buffers = x_ptr_array_new_full (tex->mipmaps + 1, NULL);
        for(mip=0; mip<=tex->mipmaps; ++mip) {
            TRACE_CALL (IDirect3DTexture9_GetSurfaceLevel,
                        "%p, %d, %p",
                        (d3d9tex->norm_tex, mip, &surface));
            /* decrement reference count, the GetSurfaceLevel call increments this
               this is safe because the texture keeps a reference as well */
            IDirect3DSurface9_Release (surface);

            pixel_buffer = x_object_new (render_class->pbuf,
                                         "surface", surface,NULL);
            x_ptr_array_append1 (tex->buffers, pixel_buffer);
        }
        break;
    case G_TEXTURE_CUBE:
        x_assert (d3d9tex->cube_tex);
        tex->faces = 6;
        tex->buffers = x_ptr_array_new_full ((tex->mipmaps + 1)*6, NULL);
        for(face=0; face<X_BOX_FACES; ++face){ 
            for(mip=0; mip<=tex->mipmaps; ++mip) {
                TRACE_CALL (IDirect3DCubeTexture9_GetCubeMapSurface,
                            "%p, %d, %d, %p",
                            (d3d9tex->cube_tex, _d3d_map_box_plane((xBoxFace)face), mip, &surface));
                IDirect3DSurface9_Release (surface);

                pixel_buffer = x_object_new (render_class->pbuf,
                                             "surface", surface, NULL);
                x_ptr_array_append1 (tex->buffers, pixel_buffer);
            }
        }
        break;
    case G_TEXTURE_3D:
        break;
    };
}
static void
set_final_attributes    (D3d9Tex        *d3d9tex,
                         xsize          width,
                         xsize          height,
                         xsize          depth,
                         D3DFORMAT      d3dformat)
{
    gTexture *tex = (gTexture*)d3d9tex;
    if (tex->width != width || tex->height != height) {
        x_warning ("dimensions altered by directx: from  %dx%d to %dx%d",
                   tex->width, tex->height, width, height);
    }
    tex->width = width;
    tex->height = height;
    tex->depth = depth;
    tex->format = _d3d_unmap_pixel_format (d3dformat);
    create_surface_list (d3d9tex);
}

static void
create_normal_tex     (D3d9Tex        *d3d9tex)
{
    DWORD usage;
    UINT  numMips;
    D3DFORMAT d3dPF;
    D3DSURFACE_DESC desc;
    gTexture *tex = (gTexture*)d3d9tex;

    if (tex->format == X_PIXEL_UNKNOWN)
        d3dPF = _d3d9_render->params.BackBufferFormat;
    else
        d3dPF = _d3d_map_pixel_format (tex->format);

    TRACE_CALL (D3DXCheckTextureRequirements,
                "%p,%d,%d,%d,%d,%p,%d",
                (_d3d9_render->device, NULL, NULL, NULL, 0,
                 &d3dPF, d3d9tex->pool));
    if (tex->flags & G_TEXTURE_RENDER_TARGET)
        usage = D3DUSAGE_RENDERTARGET;
    else
        usage = 0;
    if (tex->flags & G_TEXTURE_MIP_UNLIMITED)
        numMips = D3DX_DEFAULT;
    else
        numMips = tex->mipmaps + 1;

    TRACE_CALL (D3DXCreateTexture,
                "%p,%d,%d,%d,%d,%d,%d,%p",
                (_d3d9_render->device, tex->width, tex->height,
                 numMips, usage, d3dPF, d3d9tex->pool,
                 &d3d9tex->norm_tex));

    TRACE_CALL (IDirect3DTexture9_QueryInterface,
                "%p, %d, %p",
                (d3d9tex->norm_tex, &IID_IDirect3DBaseTexture9,
                 &d3d9tex->tex));

    TRACE_CALL (IDirect3DTexture9_GetLevelDesc,
                "%p, %d, %p",
                (d3d9tex->norm_tex, 0, &desc));
    set_final_attributes (d3d9tex, desc.Width, desc.Height, 1, desc.Format);
}
static void
create_cube_tex         (D3d9Tex        *d3d9tex)
{
    DWORD usage;
    UINT  numMips;
    D3DFORMAT d3dPF;
    D3DSURFACE_DESC desc;
    gTexture *tex = (gTexture*)d3d9tex;

    x_assert (tex->width == tex->height);

    if (tex->format == X_PIXEL_UNKNOWN)
        d3dPF = _d3d9_render->params.BackBufferFormat;
    else
        d3dPF = _d3d_map_pixel_format (tex->format);

    if (tex->flags & G_TEXTURE_RENDER_TARGET)
        usage = D3DUSAGE_RENDERTARGET;
    else
        usage = 0;
    if (tex->flags & G_TEXTURE_MIP_UNLIMITED)
        numMips = D3DX_DEFAULT;
    else
        numMips = tex->mipmaps + 1;

    TRACE_CALL (D3DXCheckCubeTextureRequirements,
        "%p,%p,%p,%d,%p,%d",
        (_d3d9_render->device, &tex->width, &numMips, usage,
         &d3dPF, d3d9tex->pool));

    tex->format = _d3d_unmap_pixel_format(d3dPF);
    TRACE_CALL (D3DXCreateCubeTexture,
                "%p,%d,%d,%d,%d,%d,%p",
                (_d3d9_render->device, tex->width,
                 numMips, usage, d3dPF, d3d9tex->pool,
                 &d3d9tex->cube_tex));

    TRACE_CALL (IDirect3DTexture9_QueryInterface,
                "%p, %d, %p",
                (d3d9tex->cube_tex, &IID_IDirect3DBaseTexture9,
                 &d3d9tex->tex));

    TRACE_CALL (IDirect3DTexture9_GetLevelDesc,
                "%p, %d, %p",
                (d3d9tex->cube_tex, 0, &desc));
    set_final_attributes (d3d9tex, desc.Width, desc.Height, 1, desc.Format);
}
static void
d3d9_tex_load           (xResource      *resource,
                         xbool          backThread)
{
    gTexture *tex = (gTexture*)resource;
    D3d9Tex *d3d9tex = (D3d9Tex*)resource;
    x_return_if_fail (tex->width>0 || tex->height>0);

    switch (tex->type){
    case G_TEXTURE_1D:
    case G_TEXTURE_2D:
        create_normal_tex(d3d9tex);
        break;
    case G_TEXTURE_CUBE:
        create_cube_tex (d3d9tex);
        break;
    }
    X_RESOURCE_CLASS (d3d9_tex_parent_class)->load (resource, backThread);
}
static void
d3d9_tex_init           (D3d9Tex        *self)
{

}
static void
d3d9_tex_class_init     (D3d9TexClass   *klass)
{
    xResourceClass *rclass;
    gTextureClass *tclass;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->load = d3d9_tex_load;

    tclass = G_TEXTURE_CLASS (klass);
    tclass->bind = d3d9_tex_bind;
    tclass->unbind = d3d9_tex_unbind;
}

static void
d3d9_tex_class_finalize (D3d9TexClass   *klass)
{

}

