#ifndef __GFW_D3D9_H__
#define __GFW_D3D9_H__
#define X_LOG_DOMAIN                "gfw-d3d9"
#include <gfw.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
X_BEGIN_DECLS

typedef struct _D3d9Render          D3d9Render;
typedef struct _D3d9RenderClass     D3d9RenderClass;
typedef struct _D3dWnd              D3dWnd;
typedef struct _D3dWndClass         D3dWndClass;
typedef struct _D3d9Tex             D3d9Tex;
typedef struct _D3d9TexClass        D3d9TexClass;
typedef struct _D3d9PixelBuf        D3d9PixelBuf;
typedef struct _D3d9PixelBufClass   D3d9PixelBufClass;
typedef struct _D3d9VertexBuf       D3d9VertexBuf;
typedef struct _D3d9VertexBufClass  D3d9VertexBufClass;
typedef struct _D3d9IndexBuf        D3d9IndexBuf;
typedef struct _D3d9IndexBufClass   D3d9IndexBufClass;

struct _D3d9Render
{
    gRender                     parent;
    LPDIRECT3D9                 d3d9;
    D3DCAPS9                    caps;
    D3DPRESENT_PARAMETERS       params;
    LPDIRECT3DDEVICE9           device;
    D3dWnd                      *window;
    gViewport                   *viewport;
    xuint                       last_source;
    xHashTable                  *vdeclares;
};
struct _D3d9RenderClass 
{
    gRenderClass                parent;
};

struct _D3dWnd
{
    gTarget                     parent;
    HWND                        hwnd;
    xbool                       external;
    LPDIRECT3DSWAPCHAIN9        swap_chain;
    LPDIRECT3DSURFACE9          back_surface;
    LPDIRECT3DSURFACE9          z_buffer;
};
struct _D3dWndClass
{
    gTargetClass                parent;
};

struct _D3d9Tex
{
    gTexture                    parent;
    LPDIRECT3DTEXTURE9          norm_tex;
    LPDIRECT3DCUBETEXTURE9      cube_tex;
    LPDIRECT3DVOLUMETEXTURE9    vol_tex;
    LPDIRECT3DBASETEXTURE9      tex;
    LPDIRECT3DSURFACE9          surface;
    D3DPOOL                     pool;
};
struct _D3d9TexClass
{
    gTextureClass               parent;
};

struct _D3d9PixelBuf
{
    gPixelBuffer                parent;
    LPDIRECT3DSURFACE9          surface;
    LPDIRECT3DSURFACE9          fsaa_surface;
};
struct _D3d9PixelBufClass
{
    gPixelBufferClass           parent;
};

struct _D3d9VertexBuf
{
    gVertexBuffer               parent;
    LPDIRECT3DVERTEXBUFFER9     handle;
    D3DPOOL                     pool;
};
struct _D3d9VertexBufClass
{
    gVertexBufferClass          parent;
};

struct _D3d9IndexBuf
{
    gIndexBuffer                parent;
    LPDIRECT3DINDEXBUFFER9      handle;
    D3DPOOL                     pool;
};
struct _D3d9IndexBufClass
{
    gIndexBufferClass           parent;
};

X_INTERN_FUNC
xType       _d3d9_render_register   (xTypeModule    *module);
X_INTERN_FUNC
xType       _d3d9_tex_register      (xTypeModule    *module);
X_INTERN_FUNC
xType       _d3d_wnd_register       (xTypeModule    *module);
X_INTERN_FUNC
xType       _d3d9_pixelbuf_register (xTypeModule    *module);
X_INTERN_FUNC
xType       _d3d9_vertexbuf_register(xTypeModule    *module);
X_INTERN_FUNC
xType       _d3d9_indexbuf_register (xTypeModule    *module);
X_INTERN_FUNC
xType       _d3d9_hlsl_register     (xTypeModule    *module);
X_INTERN_FUNC
HINSTANCE   _module_instance        (void);

#define __UNMARSHALER(...) __VA_ARGS__
#define TRACE_CALL(fn, fmt, argv)   X_STMT_START{                       \
    HRESULT hr = fn argv;                                               \
    if FAILED (hr) {                                                    \
        x_critical ("%x(%s)=%s(" fmt ")", hr,                           \
                    DXGetErrorDescriptionA (hr), #fn,                  \
                    __UNMARSHALER##argv);                               \
    }                                                                   \
} X_STMT_END

#define SAFE_RELEASE(p) X_STMT_START {                                  \
    if (p != NULL) {                                                    \
        (p)->lpVtbl->Release(p);                                        \
        p = NULL;                                                       \
    }                                                                   \
} X_STMT_END

X_END_DECLS

#include "d3dmap.h"
X_INTERN_VAR
extern D3d9Render *_d3d9_render;
#endif /* __GFW_D3D9_H__ */

