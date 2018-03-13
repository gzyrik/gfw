#ifndef __G_RENDER_H__
#define __G_RENDER_H__
#include "gtype.h"
X_BEGIN_DECLS

typedef enum {
    G_RENDER_POINT_LIST,
    G_RENDER_LINE_LIST,
    G_RENDER_LINE_STRIP,
    G_RENDER_TRIANGLE_LIST,
    G_RENDER_TRIANGLE_STRIP,
    G_RENDER_TRIANGLE_FAN
} gPrimitiveType;

typedef enum {
    G_FRAME_COLOR = 1,
    G_FRAME_DEPTH = 2,
    G_FRAME_STENCIL = 4
} gFrameBuffers;

typedef enum {
    G_CULL_NONE,
    G_CULL_CW,
    G_CULL_CCW
} gCullMode;

typedef enum {
    G_FILL_POINT,
    G_FILL_WIREFRAME,
    G_FILL_SOLID
} gFillMode;

struct _gRenderOp
{
    gPrimitiveType  primitive;
    gVertexData     *vdata;
    gIndexData      *idata;
    gRenderable     *renderable;
};

struct _gFrameTimer
{
    xuint           frame_count;
    greal           frame_time;
    greal           elapsed_time;
    greal           time_factor;
    xbool           ftime_fixed;
};

#define G_TYPE_RENDER               (g_render_type())
#define G_RENDER(object)            (X_INSTANCE_CAST((object), G_TYPE_RENDER, gRender))
#define G_RENDER_CLASS(klass)       (X_CLASS_CAST((klass), G_TYPE_RENDER, gRenderClass))
#define G_IS_RENDER(object)         (X_INSTANCE_IS_TYPE((object), G_TYPE_RENDER))
#define G_RENDER_GET_CLASS(object)  (X_INSTANCE_GET_CLASS((object), G_TYPE_RENDER, gRenderClass))

struct _gRender
{
    xObject             parent;
    xuint               frame_count;
    xint                frame_smooting;
    gFrameTimer         frame_timer;
    xArray              *frame_times;
    xArray              *frame_hooks;
    xPtrArray           *targets;
    xPtrArray           *texUnits;
    xPtrArray           *shaders;
};

struct _gRenderClass
{
    xObjectClass        parent;
    xType               window;
    xType               texture;
    xType               vbuf;
    xType               ibuf;
    xType               pbuf;
    void    (*begin)    (gRender *render);
    void    (*clear)    (gRender *render, gViewport *viewport);
    void    (*setup)    (gRender *render, gPass *pass);
    void    (*draw)     (gRender *render, gRenderOp *op);
    void    (*end)      (gRender *render);
};

xType		g_render_type			(void);

void        g_render_open           (xcstr          type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

void        g_render_close          (void);

void        g_render_get            (xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

gTarget*    g_render_target         (xcstr          name);

void        g_render_attach         (gTarget        *target);

void        g_render_detach         (gTarget        *target);

gTarget*    g_render_new_target     (xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

gProgram*   g_render_new_program    (xcstr          type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;


void        g_render_begin          (void);

void        g_render_clear          (gViewport      *viewport);

void        g_render_setup          (gPass          *pass);

void        g_render_draw           (gRenderOp      *op);

void        g_render_end            (void);

greal       g_render_update         (xuint          now_ms);

void        g_render_add_fhook      (gFrameHook     hook,
                                     xptr           user);

void        g_render_remove_fhook   (gFrameHook     hook,
                                     xptr           user);

gFrameTimer* g_render_ftimer        (void);

X_END_DECLS
#endif /* __G_RENDER_H__ */
