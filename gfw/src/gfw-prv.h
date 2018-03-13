#ifndef __G_INIT_PRV_H__
#define __G_INIT_PRV_H__
X_BEGIN_DECLS
X_INTERN_FUNC
void        _g_value_types_init     (void);
X_INTERN_FUNC
void        _g_material_init        (void);
X_INTERN_FUNC
void        _g_process_queued_nodes (void);
X_INTERN_FUNC
void        _g_shader_init          (void);
X_INTERN_FUNC
void        _g_buffer_init          (void);
X_INTERN_FUNC
void        _g_buffer_gc            (void);
X_INTERN_FUNC
void        _g_movable_init         (void);


X_INTERN_FUNC
void        _g_render_fhook_update  (void);

X_INTERN_VAR gRender *_g_render;
X_END_DECLS
#endif /* __G_INIT_PRV_H__ */

