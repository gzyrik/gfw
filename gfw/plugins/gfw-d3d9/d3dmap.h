X_INLINE_FUNC D3DFORMAT
_d3d_map_pixel_format   (xPixelFormat   format)
{
    switch(format) {
    case X_PIXEL_L8: return D3DFMT_L8;
    case X_PIXEL_L16: return D3DFMT_L16;
    case X_PIXEL_A8: return D3DFMT_A8;
    case X_PIXEL_A4L4: return D3DFMT_A4L4;
    case X_PIXEL_BYTE_LA: return D3DFMT_A8L8; /* Assume little endian here */
    case X_PIXEL_R3G3B2: return D3DFMT_R3G3B2;
    case X_PIXEL_A1R5G5B5: return D3DFMT_A1R5G5B5;
    case X_PIXEL_R5G6B5: return D3DFMT_R5G6B5;
    case X_PIXEL_A4R4G4B4: return D3DFMT_A4R4G4B4;
    case X_PIXEL_R8G8B8: return D3DFMT_R8G8B8;
    case X_PIXEL_A8R8G8B8: return D3DFMT_A8R8G8B8;
    case X_PIXEL_A8B8G8R8: return D3DFMT_A8B8G8R8;
    case X_PIXEL_X8R8G8B8: return D3DFMT_X8R8G8B8;
    case X_PIXEL_X8B8G8R8: return D3DFMT_X8B8G8R8;
    case X_PIXEL_B8G8R8A8: return D3DFMT_A8R8G8B8;
    case X_PIXEL_A2B10G10R10: return D3DFMT_A2B10G10R10;
    case X_PIXEL_A2R10G10B10: return D3DFMT_A2R10G10B10;
    case X_PIXEL_FLOAT16_R: return D3DFMT_R16F;
    case X_PIXEL_FLOAT16_GR: return D3DFMT_G16R16F;
    case X_PIXEL_FLOAT16_RGBA: return D3DFMT_A16B16G16R16F;
    case X_PIXEL_FLOAT32_R: return D3DFMT_R32F;
    case X_PIXEL_FLOAT32_GR: return D3DFMT_G32R32F;
    case X_PIXEL_FLOAT32_RGBA: return D3DFMT_A32B32G32R32F;
    case X_PIXEL_SHORT_RGBA: return D3DFMT_A16B16G16R16;
    case X_PIXEL_SHORT_GR: return D3DFMT_G16R16;
    case X_PIXEL_DXT1: return D3DFMT_DXT1;
    case X_PIXEL_DXT2: return D3DFMT_DXT2;
    case X_PIXEL_DXT3: return D3DFMT_DXT3;
    case X_PIXEL_DXT4: return D3DFMT_DXT4;
    case X_PIXEL_DXT5: return D3DFMT_DXT5;
    }
    x_error_if_reached ();
    return 0;
}

X_INLINE_FUNC xPixelFormat
_d3d_unmap_pixel_format (D3DFORMAT   format)
{
    switch(format) {
    case D3DFMT_A8:     return X_PIXEL_A8;
    case D3DFMT_L8:     return X_PIXEL_L8;
    case D3DFMT_L16:    return X_PIXEL_L16;
    case D3DFMT_A4L4:   return X_PIXEL_A4L4;
    case D3DFMT_A8L8:   return X_PIXEL_BYTE_LA;/* Assume little endian here */
    case D3DFMT_R3G3B2: return X_PIXEL_R3G3B2;
    case D3DFMT_A1R5G5B5: return X_PIXEL_A1R5G5B5;
    case D3DFMT_A4R4G4B4: return X_PIXEL_A4R4G4B4;
    case D3DFMT_R5G6B5: return X_PIXEL_R5G6B5;
    case D3DFMT_R8G8B8: return X_PIXEL_R8G8B8;
    case D3DFMT_X8R8G8B8: return X_PIXEL_X8R8G8B8;
    case D3DFMT_A8R8G8B8: return X_PIXEL_A8R8G8B8;
    case D3DFMT_X8B8G8R8: return X_PIXEL_X8B8G8R8;
    case D3DFMT_A8B8G8R8: return X_PIXEL_A8B8G8R8;
    case D3DFMT_A2R10G10B10: return X_PIXEL_A2R10G10B10;
    case D3DFMT_A2B10G10R10: return X_PIXEL_A2B10G10R10;
    case D3DFMT_R16F: return X_PIXEL_FLOAT16_R;
    case D3DFMT_A16B16G16R16F: return X_PIXEL_FLOAT16_RGBA;
    case D3DFMT_R32F: return X_PIXEL_FLOAT32_R;
    case D3DFMT_G32R32F: return X_PIXEL_FLOAT32_GR;
    case D3DFMT_A32B32G32R32F: return X_PIXEL_FLOAT32_RGBA;
    case D3DFMT_G16R16F: return X_PIXEL_FLOAT16_GR;
    case D3DFMT_A16B16G16R16: return X_PIXEL_SHORT_RGBA;
    case D3DFMT_G16R16: return X_PIXEL_SHORT_GR;
    case D3DFMT_DXT1: return X_PIXEL_DXT1;
    case D3DFMT_DXT2: return X_PIXEL_DXT2;
    case D3DFMT_DXT3: return X_PIXEL_DXT3;
    case D3DFMT_DXT4: return X_PIXEL_DXT4;
    case D3DFMT_DXT5: return X_PIXEL_DXT5;
    }
    x_error_if_reached ();
    return 0;
}

X_INLINE_FUNC xint
_d3d_map_vertex_semantic(gVertexSemantic   semantic)
{
    switch (semantic) {
    case G_VERTEX_BINDICES:
        return D3DDECLUSAGE_BLENDINDICES;
    case G_VERTEX_BWEIGHTS:
        return D3DDECLUSAGE_BLENDWEIGHT;
    case G_VERTEX_DIFFUSE:
        return D3DDECLUSAGE_COLOR;
    case G_VERTEX_SPECULAR:
        return D3DDECLUSAGE_COLOR;
    case G_VERTEX_NORMAL:
        return D3DDECLUSAGE_NORMAL;
    case G_VERTEX_POSITION:
        return D3DDECLUSAGE_POSITION;
    case G_VERTEX_TEXCOORD:
        return D3DDECLUSAGE_TEXCOORD;
    case G_VERTEX_BINORMAL:
        return D3DDECLUSAGE_BINORMAL;
    case G_VERTEX_TANGENT:
        return D3DDECLUSAGE_TANGENT;
    case G_VERTEX_PSIZE:
        return D3DDECLUSAGE_PSIZE;
    }
    x_error_if_reached ();
    return 0;
}

X_INLINE_FUNC xint
_d3d_map_vertex_type     (gVertexType   type)
{
    switch (type){
    case G_VERTEX_A8R8G8B8:
        return D3DDECLTYPE_UBYTE4N;
    case G_VERTEX_FLOAT1:
        return D3DDECLTYPE_FLOAT1;
    case G_VERTEX_FLOAT2:
        return D3DDECLTYPE_FLOAT2;
    case G_VERTEX_FLOAT3:
        return D3DDECLTYPE_FLOAT3;
    case G_VERTEX_FLOAT4:
        return D3DDECLTYPE_FLOAT4;
    case G_VERTEX_SHORT2:
        return D3DDECLTYPE_SHORT2;
    case G_VERTEX_SHORT4:
        return D3DDECLTYPE_SHORT4;
    case G_VERTEX_UBYTE4:
        return D3DDECLTYPE_UBYTE4;
    }
    x_error_if_reached ();
    return 0;
}
X_INLINE_FUNC void
_d3d_map_primitive      (gRenderOp      *op,
                         D3DPRIMITIVETYPE *primType,
                         DWORD          *primCount)
{
    switch( op->primitive ) {
    case G_RENDER_POINT_LIST:
        *primType = D3DPT_POINTLIST;
        *primCount = (DWORD)(op->idata ? op->idata->count : op->vdata->count);
        break;
    case G_RENDER_LINE_LIST:
        *primType = D3DPT_LINELIST;
        *primCount = (DWORD)(op->idata ? op->idata->count : op->vdata->count) / 2;
        break;
    case G_RENDER_LINE_STRIP:
        *primType = D3DPT_LINESTRIP;
        *primCount = (DWORD)(op->idata ? op->idata->count : op->vdata->count) - 1;
        break;
    case G_RENDER_TRIANGLE_LIST:
        *primType = D3DPT_TRIANGLELIST;
        *primCount = (DWORD)(op->idata ? op->idata->count : op->vdata->count) / 3;
        break;
    case G_RENDER_TRIANGLE_STRIP:
        *primType = D3DPT_TRIANGLESTRIP;
        *primCount = (DWORD)(op->idata ? op->idata->count : op->vdata->count) - 2;
        break;
    case G_RENDER_TRIANGLE_FAN:
        *primType = D3DPT_TRIANGLEFAN;
        *primCount = (DWORD)(op->idata ? op->idata->count : op->vdata->count) - 2;
        break;
    default:
        x_error_if_reached ();
    }
}
X_INLINE_FUNC DWORD
_d3d_map_cull_mode      (gCullMode      mode,
                         xbool          flip)
{
    switch( mode ) {
    case G_CULL_NONE:
        return D3DCULL_NONE;
    case G_CULL_CW:
        return flip ?  D3DCULL_CCW : D3DCULL_CW;
    case G_CULL_CCW:
        return flip ? D3DCULL_CW : D3DCULL_CCW;
    }
    x_error_if_reached ();
    return 0;
}
X_INLINE_FUNC DWORD
_d3d_map_fill_mode      (gFillMode      mode)
{
    switch( mode ) {
    case G_FILL_POINT:
        return D3DFILL_POINT;
    case G_FILL_WIREFRAME:
        return D3DFILL_WIREFRAME;
    case G_FILL_SOLID:
        return D3DFILL_SOLID;
    }
    x_error_if_reached ();
    return 0;
}
X_INLINE_FUNC D3DCUBEMAP_FACES
_d3d_map_box_plane      (xBoxFace       face)
{
    switch( face ) {
    case X_BOX_FRONT:
        return D3DCUBEMAP_FACE_NEGATIVE_Z;
    case X_BOX_BACK:
        return D3DCUBEMAP_FACE_POSITIVE_Z;
    case X_BOX_LEFT:
        return D3DCUBEMAP_FACE_POSITIVE_X;
    case X_BOX_RIGHT:
        return D3DCUBEMAP_FACE_NEGATIVE_X;
    case X_BOX_UP:
        return D3DCUBEMAP_FACE_POSITIVE_Y;
    case X_BOX_DOWN:
        return D3DCUBEMAP_FACE_NEGATIVE_Y;
    }
    x_error_if_reached ();
    return 0;
}
