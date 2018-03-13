#include "gfw-d3d9.h"
#include <string.h>
X_DEFINE_DYNAMIC_TYPE (D3d9PixelBuf, d3d9_pbuf, G_TYPE_PIXEL_BUFFER);
X_DEFINE_DYNAMIC_TYPE (D3d9VertexBuf, d3d9_vbuf, G_TYPE_VERTEX_BUFFER);
X_DEFINE_DYNAMIC_TYPE (D3d9IndexBuf, d3d9_ibuf, G_TYPE_INDEX_BUFFER);
xType
_d3d9_pixelbuf_register (xTypeModule    *module)
{
    return d3d9_pbuf_register (module);
}
xType
_d3d9_vertexbuf_register (xTypeModule    *module)
{
    return d3d9_vbuf_register (module);
}
xType
_d3d9_indexbuf_register (xTypeModule    *module)
{
    return d3d9_ibuf_register (module);
}

enum {
    PROP_0,
    PROP_SURFACE,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
}
static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
}

static xptr
d3d9_pbuf_constructor   (xType          type,
                         xsize          n_properties,
                         xConstructParam *params)
{
    xsize i;
    D3DSURFACE_DESC desc;
    IDirect3DSurface9 *surface=NULL;
    D3d9PixelBuf *d3d9buf;
    gPixelBuffer *buf;

    for (i=0; i< n_properties; ++i) {
        switch (params[i].pspec->param_id) {
        case PROP_SURFACE:
            surface = x_value_get_ptr (params[i].value);
            break;
        }
    }
    x_return_val_if_fail (surface != NULL, NULL);
    TRACE_CALL (IDirect3DSurface9_GetDesc,
                "%p, %p",
                (surface, &desc));

    d3d9buf = (D3d9PixelBuf*) x_instance_new (type);
    d3d9buf->surface = surface;
    buf = (gPixelBuffer*)d3d9buf;
    buf->width = desc.Width;
    buf->height= desc.Height;
    buf->depth = 1;
    buf->row_pitch = desc.Width;
    buf->slice_pitch = desc.Width * desc.Height;
    buf->format = _d3d_unmap_pixel_format (desc.Format);
    buf->parent.size = x_pixel_box_bytes (desc.Width, desc.Height, 1, buf->format); 
    return buf;
}
static void
d3d9_pbuf_finalize      (xObject        *object)
{
}
static void
d3d9_pbuf_init          (D3d9PixelBuf   *self)
{
}
static void
d3d9_pbuf_upload        (gBuffer        *buffer)
{
    D3d9PixelBuf *d3d9buf = (D3d9PixelBuf*) buffer;
    gPixelBuffer *pixelbuf = (gPixelBuffer*)d3d9buf;
    RECT rt;

    x_return_if_fail (d3d9buf->surface != NULL);

    rt.left   = pixelbuf->dirty_box.left;
    rt.right  = pixelbuf->dirty_box.right;
    rt.top    = pixelbuf->dirty_box.top;
    rt.bottom = pixelbuf->dirty_box.bottom;

    TRACE_CALL (D3DXLoadSurfaceFromMemory,
                "%p,%p,%p,%p,%d,%d,%p,%p,%d,%d",
                (d3d9buf->surface, NULL, &rt,
                 buffer->data, _d3d_map_pixel_format (pixelbuf->format),
                 x_pixel_format_bytes(pixelbuf->format, pixelbuf->row_pitch),
                 NULL, &rt, D3DX_DEFAULT, 0));
}
static void
d3d9_pbuf_class_init    (D3d9PixelBufClass *klass)
{
    gBufferClass *bclass;
    xObjectClass *oclass;
    xParam *param;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->constructor  = d3d9_pbuf_constructor;
    oclass->finalize     = d3d9_pbuf_finalize;

    bclass = G_BUFFER_CLASS (klass);
    bclass->upload = d3d9_pbuf_upload;

    param = x_param_ptr_new ("surface","surface","surface",
                             X_PARAM_STATIC_STR | X_PARAM_WRITABLE
                             | X_PARAM_CONSTRUCT_ONLY);
    x_oclass_install_param(oclass, PROP_SURFACE, param);
}

static void
d3d9_pbuf_class_finalize(D3d9PixelBufClass *klass)
{
}
static void
d3d9_vbuf_finalize      (xObject        *object)
{
}
static void
d3d9_vbuf_init          (D3d9VertexBuf  *self)
{
}
static void
d3d9_vbuf_upload        (gBuffer        *buffer)
{
    D3d9VertexBuf *d3d9buf = (D3d9VertexBuf*)buffer;
    gVertexBuffer *vbuf = (gVertexBuffer*)buffer;
    xptr p;
    xsize count;


    if (!d3d9buf->handle) {
        UINT usage = D3DUSAGE_WRITEONLY;
        if(buffer->dynamic) usage |= D3DUSAGE_DYNAMIC;
        buffer->size = vbuf->vertex_size * vbuf->vertex_count;
        TRACE_CALL (IDirect3DDevice9_CreateVertexBuffer,
                    "%p, %d, %d, %d, %d, %p, %p",
                    (_d3d9_render->device, buffer->size, usage, 0,
                     d3d9buf->pool, &d3d9buf->handle, NULL));
    }

    count = buffer->dirty_end - buffer->dirty_start;

    TRACE_CALL (IDirect3DVertexBuffer9_Lock,
                "%p, %d, %d, %p, %d",
                (d3d9buf->handle, buffer->dirty_start, count,
                 &p, D3DLOCK_DISCARD));

    memcpy (p, buffer->data+buffer->dirty_start, count);

    TRACE_CALL (IDirect3DVertexBuffer9_Unlock, "%p", (d3d9buf->handle)); 
}
static void
d3d9_vbuf_class_init   (D3d9VertexBufClass *klass)
{
    gBufferClass *bclass;
    xObjectClass *oclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->finalize     = d3d9_vbuf_finalize;

    bclass = G_BUFFER_CLASS (klass);
    bclass->upload = d3d9_vbuf_upload;
}

static void
d3d9_vbuf_class_finalize (D3d9VertexBufClass *klass)
{
}
static void
d3d9_ibuf_finalize      (xObject        *object)
{
}
static void
d3d9_ibuf_init          (D3d9IndexBuf   *self)
{
}
static void
d3d9_ibuf_upload        (gBuffer        *buffer)
{
    D3d9IndexBuf *d3d9buf = (D3d9IndexBuf*)buffer;
    gIndexBuffer *ibuf = (gIndexBuffer*)buffer;
    xptr p;
    xsize count;

    if (!d3d9buf->handle) {
        UINT usage = D3DUSAGE_WRITEONLY;
        if(buffer->dynamic) usage |= D3DUSAGE_DYNAMIC;
        buffer->size = ibuf->index_size * ibuf->index_count;
        TRACE_CALL (IDirect3DDevice9_CreateIndexBuffer,
                    "%p, %d, %d, %d, %d, %p, %p",
                    (_d3d9_render->device, buffer->size, usage,
                     ibuf->index_size == sizeof(xuint32) ? D3DFMT_INDEX32 : D3DFMT_INDEX16,
                     d3d9buf->pool, &d3d9buf->handle, NULL));
    }

    count = buffer->dirty_end - buffer->dirty_start;
    TRACE_CALL (IDirect3DIndexBuffer9_Lock,
                "%p, %d, %d, %p, %d",
                (d3d9buf->handle, buffer->dirty_start, count,
                 &p, D3DLOCK_DISCARD));

    memcpy (p, buffer->data + buffer->dirty_start, count);

    TRACE_CALL (IDirect3DIndexBuffer9_Unlock, "%p", (d3d9buf->handle)); 
}
static void
d3d9_ibuf_class_init   (D3d9IndexBufClass *klass)
{
    gBufferClass *bclass;
    xObjectClass *oclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->finalize = d3d9_ibuf_finalize;

    bclass = G_BUFFER_CLASS (klass);
    bclass->upload = d3d9_ibuf_upload;
}

static void
d3d9_ibuf_class_finalize (D3d9IndexBufClass *klass)
{
}
