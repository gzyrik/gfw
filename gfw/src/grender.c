#include "config.h"
#include "grender.h"
#include "gtarget.h"
#include "gmaterial.h"
#include "gshader.h"
#include "gtexture.h"
#include "gfw-prv.h"
#include "gbuffer.h"
#include "gtexunit.h"
#include "gpass.h"
#include "gtechnique.h"
#include "gmesh-prv.h"
struct FrameHook
{
    gFrameHook      hook;
    xptr            user;
};
static void
gfw_init                (void)
{
    _g_value_types_init();
    _g_material_init();
    _g_mesh_script_register();
    _g_mesh_chunk_register();
    _g_skeleton_chunk_register();
    _g_skeleton_script_register();
    _g_shader_init();
    _g_buffer_init();
    _g_movable_init();
}
X_DEFINE_TYPE_EXTENDED (gRender, g_render, X_TYPE_OBJECT, 0,gfw_init());
gRender *_g_render;
static void
g_render_init           (gRender       *render)
{
    _g_render = render;
    render->frame_smooting = 1000;
    render->frame_timer.time_factor = 1;
    render->frame_times = x_array_new (sizeof(xuint));
    render->targets = x_ptr_array_new_full (0, x_object_unref);
    render->frame_hooks = x_array_new (sizeof(struct FrameHook));
}

static void
render_finalize         (xObject        *object)
{
    gRender *render = (gRender*) object;

    x_array_unref (render->frame_times);
    x_array_unref (render->frame_hooks);
    x_ptr_array_unref (render->targets);
    x_ptr_array_unref (render->shaders);
    x_ptr_array_unref (render->texUnits);
    _g_render = NULL;
}
static void
g_render_class_init     (gRenderClass  *klass)
{
    xObjectClass *oclass;
    
    oclass = X_OBJECT_CLASS (klass);
    oclass->finalize = render_finalize;

    klass->ibuf = G_TYPE_INDEX_BUFFER;
    klass->vbuf = G_TYPE_VERTEX_BUFFER;
    klass->pbuf = G_TYPE_PIXEL_BUFFER;
}

void
g_render_open           (xcstr          type,
                         xcstr          first_property,
                         ...)
{
    va_list argv;
    xType xtype;
    x_return_if_fail (_g_render == NULL);

    xtype = x_type_from_mime (G_TYPE_RENDER, type);
    x_return_if_fail (xtype != X_TYPE_INVALID);

    va_start(argv, first_property);
    x_object_new_valist (xtype, first_property, argv);
    va_end(argv);
}

void
g_render_close          (void)
{
    x_object_unref (_g_render);
    _g_render = NULL;
}

void
g_render_get            (xcstr          first_property,
                         ...)
{
    va_list argv;

    va_start(argv, first_property);
    x_object_get_valist(_g_render, first_property, argv);
    va_end(argv);
}

static xint
target_priority_cmp     (gTarget        *target1,
                         gTarget        *target2)
{
    return G_TARGET_GET_CLASS(target1)->priority
        - G_TARGET_GET_CLASS(target2)->priority;
}

gTarget*
g_render_target         (xcstr          name)
{
    xsize i;

    for (i=0; i<_g_render->targets->len; ++i) {
        gTarget *target = _g_render->targets->data[i];
        if (target && !x_strcmp (name, target->name))
            return target;
    }
    return NULL;
}

void
g_render_attach         (gTarget        *target)
{
    x_return_if_fail (G_IS_TARGET (target));

    if (x_ptr_array_find_sorted (_g_render->targets, target, 
                                 (xCmpFunc)target_priority_cmp) < 0) {
        x_object_sink (target);
        x_ptr_array_insert_sorted (_g_render->targets, target, target,
                                   (xCmpFunc)target_priority_cmp, FALSE);
    }
}

void
g_render_detach         (gTarget        *target)
{
    x_return_if_fail (G_IS_TARGET (target));

    x_ptr_array_remove_data (_g_render->targets, target);
}

gTarget*
g_render_new_target     (xcstr          first_property,
                         ...)
{
    gTarget *target;
    va_list argv;
    gRenderClass *klass;

    klass = G_RENDER_GET_CLASS (_g_render);
    x_return_val_if_fail (klass->window != 0, NULL);


    va_start(argv, first_property);
    target = x_object_new_valist (klass->window, first_property, argv);
    va_end(argv);

    return target;
}

gBuffer*
g_vertex_buffer_new     (xsize          vertex_size,
                         xsize          vertex_count,
                         xcstr          first_property,
                         ...)
{
    gVertexBuffer *buf;
    gRenderClass *klass;

    klass = G_RENDER_GET_CLASS (_g_render);
    x_return_val_if_fail (klass->vbuf != 0, NULL);

    buf = x_object_new (klass->vbuf,NULL);
    buf->vertex_size = vertex_size;
    buf->vertex_count = vertex_count;
    buf->parent.size = vertex_size * vertex_count;

    if (first_property != NULL) {
        va_list argv;

        va_start(argv, first_property);
        x_object_set_valist (buf, first_property, argv);
        va_end(argv);
    }

    return G_BUFFER (buf);
}

gBuffer*
g_index_buffer_new      (xsize          index_size,
                         xsize          index_count,
                         xcstr          first_property,
                         ...)
{
    gIndexBuffer *buf;
    gRenderClass *klass;

    klass = G_RENDER_GET_CLASS (_g_render);
    x_return_val_if_fail (klass->ibuf != 0, NULL);

    buf =  x_object_new (klass->ibuf, NULL);
    buf->index_size  = index_size;
    buf->index_count = index_count;
    buf->parent.size = index_size * index_count;

    if (first_property != NULL) {
        va_list argv;

        va_start(argv, first_property);
        x_object_set_valist (buf, first_property, argv);
        va_end(argv);
    }
    return G_BUFFER (buf);
}
gTexture*
g_texture_new           (xcstr          file,
                         xcstr          group,
                         xcstr          first_property,
                         ...)
{
    va_list argv;
    gTexture* tex;
    gRenderClass *klass;

    tex = g_texture_get (file);
    if (tex)
        return tex;

    klass = G_RENDER_GET_CLASS (_g_render);
    x_return_val_if_fail (klass->texture != 0, NULL);


    va_start(argv, first_property);
    tex = x_object_new_valist1 (klass->texture,
                                first_property, argv,
                                "name", file,
                                "group", group,
                                NULL);
    va_end(argv);
    return tex;
}

gProgram*
g_render_new_program    (xcstr          type,
                         xcstr          first_property,
                         ...)
{
    gProgram *program;
    xType xtype;
    va_list argv;

    xtype = x_type_from_mime (G_TYPE_PROGRAM, type);

    if (!xtype)
        return NULL;

    va_start(argv, first_property);
    program = x_object_new_valist (xtype, first_property, argv);
    va_end(argv);

    return program;
}

void
g_render_begin          (void)
{
    gRenderClass *klass;

    klass = G_RENDER_GET_CLASS (_g_render);
    klass->begin (_g_render);
}
void
g_render_clear          (gViewport      *viewport)
{
    gRenderClass *klass;

    klass = G_RENDER_GET_CLASS (_g_render);
    klass->clear (_g_render, viewport);
}
void
g_render_setup          (gPass          *pass)
{
    gRenderClass *klass;

    if (_g_render->texUnits != pass->texUnits) {
        if (_g_render->texUnits) {
            x_ptr_array_foreach(_g_render->texUnits, g_texunit_unbind, NULL);
            x_ptr_array_unref (_g_render->texUnits);
            _g_render->texUnits = NULL;
        }
        if (pass->texUnits) {
            x_ptr_array_foreach(pass->texUnits, g_texunit_bind, NULL);
            _g_render->texUnits = x_ptr_array_ref(pass->texUnits);
        }
    }

    if (_g_render->shaders != pass->shaders) {
        if (_g_render->shaders) {
            x_ptr_array_foreach(_g_render->shaders, g_shader_unbind, NULL);
            x_ptr_array_unref (_g_render->shaders);
            _g_render->shaders = NULL;
        }
        if (pass->shaders) {
            x_ptr_array_foreach(pass->shaders, g_shader_bind, NULL);
            _g_render->shaders = x_ptr_array_ref(pass->shaders);
        }
    }
    klass = G_RENDER_GET_CLASS (_g_render);
    klass->setup (_g_render, pass);
}
void
g_render_draw           (gRenderOp      *op)
{
    gRenderClass *klass;
    x_return_if_fail (op != NULL);

    klass = G_RENDER_GET_CLASS (_g_render);
    klass->draw (_g_render, op);
}
void
g_render_end            (void)
{
    gRenderClass *klass;

    klass = G_RENDER_GET_CLASS (_g_render);
    klass->end (_g_render);
}

void
_g_render_fhook_update  (void)
{
    static xuint frame_count;
    if (frame_count != _g_render->frame_count) {
        xsize i;
        frame_count = _g_render->frame_count;

        for (i=0;i<_g_render->frame_hooks->len;++i) {
            struct FrameHook *fh;
            fh = &x_array_index (_g_render->frame_hooks, struct FrameHook, i);
            fh->hook(fh->user, _g_render->frame_timer.frame_time);
        }
    }
}
void         
g_render_add_fhook      (gFrameHook     hook,
                         xptr           user)
{
    struct FrameHook *fh;
    fh = x_array_append1 (_g_render->frame_hooks, NULL);
    fh->hook = hook;
    fh->user = user;
}

void
g_render_remove_fhook   (gFrameHook     hook,
                         xptr           user)
{
    xsize i;
    for (i=0;i<_g_render->frame_hooks->len;++i) {
        struct FrameHook *fh;
        fh = &x_array_index (_g_render->frame_hooks, struct FrameHook, i);
        if (fh->hook == hook && fh->user == user) {
            x_array_remove (_g_render->frame_hooks, i);
            return;
        }
    }
}

gFrameTimer*
g_render_ftimer         (void)
{
    return &_g_render->frame_timer;
}

static greal
calc_time               (xuint          now_ms)
{
    x_array_append1 (_g_render->frame_times, &now_ms);
    if(_g_render->frame_times->len == 1)
        return 0;

    if (_g_render->frame_times->len > 2) {
        xsize i;
        for (i=0;i<_g_render->frame_times->len-2;++i) {
            xint dt=now_ms - x_array_index (_g_render->frame_times, xuint, i);
            if (dt <= _g_render->frame_smooting)
                break;
        }
        if (i > 0)
            x_array_remove_range (_g_render->frame_times, 0, i-1);
    }
    now_ms = x_array_index (_g_render->frame_times, xuint, _g_render->frame_times->len-1);
    now_ms -=  x_array_index (_g_render->frame_times, xuint, 0);

    return (greal)now_ms/(greal)((_g_render->frame_times->len-1)*1000);
}

greal
g_render_update         (xuint          now_ms)
{
    xsize i;
    greal ftime;
    gTarget *target;

    ftime = calc_time (now_ms);
    if(_g_render->frame_timer.ftime_fixed)
        _g_render->frame_timer.time_factor =  _g_render->frame_timer.frame_time/ ftime;
    else 
        _g_render->frame_timer.frame_time =  _g_render->frame_timer.time_factor * ftime;
    _g_render->frame_timer.elapsed_time += _g_render->frame_timer.frame_time;

    for (i=0; i<_g_render->targets->len; ++i) {
        target = _g_render->targets->data[i];
        g_target_update (target);
    }
    _g_render->frame_count++;
    _g_buffer_gc();

    for (i=0; i<_g_render->targets->len; ++i) {
        target = _g_render->targets->data[i];
        g_target_swap (target);
    }
    return ftime;
}