#include "config.h"
#include "gshader.h"
#include "gscenerender.h"
#include "grender.h"
#include <string.h>
#include <stdlib.h>
static xint
name_cmp                (xcptr  p0, xcptr  p1)
{
    return strcmp (*(xcstr*)p0, *(xcstr*)p1);
}
struct uniform_spec {
    xcstr               name;
    gUniformSemantic    semantic;
    xsize               extra;
};
struct uniform_spec _uniform_spec[]= {
    {"manual", G_UNIFORM_MANUAL,0},

    {"model_matrix", G_UNIFORM_MODEL_MATRIX, 0},
    {"model_affine_array", G_UNIFORM_MODEL_AFFINE_ARRAY, 0},

    {"node_matrix", G_UNIFORM_NODE_MATRIX,0},
    {"nodeviewproj_matrix", G_UNIFORM_NODEVIEWPROJ_MATRIX,0},

    {"world_matrix", G_UNIFORM_WORLD_MATRIX,0},
    {"inverse_world_matrix", G_UNIFORM_INVERSE_WORLD_MATRIX,0},
    {"transpose_world_matrix", G_UNIFORM_TRANSPOSE_WORLD_MATRIX,0},
    {"inverse_transpose_world_matrix", G_UNIFORM_INVERSE_TRANSPOSE_WORLD_MATRIX,0},
    {"world_matrix_array_3x4", G_UNIFORM_WORLD_MATRIX_ARRAY_3x4,0},
    {"world_matrix_array", G_UNIFORM_WORLD_MATRIX_ARRAY,0},

    {"view_matrix", G_UNIFORM_VIEW_MATRIX,0},
    {"inverse_view_matrix", G_UNIFORM_INVERSE_VIEW_MATRIX,0},
    {"transpose_view_matrix", G_UNIFORM_TRANSPOSE_VIEW_MATRIX,0},
    {"inverse_transpose_view_matrix", G_UNIFORM_INVERSE_TRANSPOSE_VIEW_MATRIX,0},

    {"projection_matrix", G_UNIFORM_PROJECTION_MATRIX,0},
    {"inverse_projection_matrix", G_UNIFORM_INVERSE_PROJECTION_MATRIX,0},
    {"transpose_projection_matrix", G_UNIFORM_TRANSPOSE_PROJECTION_MATRIX,0},
    {"inverse_transpose_projection_matrix", G_UNIFORM_INVERSE_TRANSPOSE_PROJECTION_MATRIX,0},

    {"viewproj_matrix", G_UNIFORM_VIEWPROJ_MATRIX,0},
    {"inverse_viewproj_matrix", G_UNIFORM_INVERSE_VIEWPROJ_MATRIX,0},
    {"transpose_viewproj_matrix", G_UNIFORM_TRANSPOSE_VIEWPROJ_MATRIX,0},
    {"inverse_transpose_viewproj_matrix", G_UNIFORM_INVERSE_TRANSPOSE_VIEWPROJ_MATRIX,0},

    {"worldview_matrix", G_UNIFORM_WORLDVIEW_MATRIX,0},
    {"inverse_worldview_matrix", G_UNIFORM_INVERSE_WORLDVIEW_MATRIX,0},
    {"transpose_worldview_matrix", G_UNIFORM_TRANSPOSE_WORLDVIEW_MATRIX,0},
    {"inverse_transpose_worldview_matrix", G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEW_MATRIX,0},

    {"worldviewproj_matrix", G_UNIFORM_WORLDVIEWPROJ_MATRIX,0},
    {"inverse_worldviewproj_matrix", G_UNIFORM_INVERSE_WORLDVIEWPROJ_MATRIX,0},
    {"transpose_worldviewproj_matrix", G_UNIFORM_TRANSPOSE_WORLDVIEWPROJ_MATRIX,0},
    {"inverse_transpose_worldviewproj_matrix", G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX,0},

    {"fog_colour", G_UNIFORM_FOG_COLOUR,0},
    {"fog_params", G_UNIFORM_FOG_PARAMS,0},

    {"light_count", G_UNIFORM_LIGHT_COUNT,0},
    {"ambient_light_colour", G_UNIFORM_AMBIENT_LIGHT_COLOUR,0},
    {"light_diffuse_colour", G_UNIFORM_LIGHT_DIFFUSE_COLOUR,0},
    {"light_specular_colour", G_UNIFORM_LIGHT_SPECULAR_COLOUR,0},
    {"light_attenuation", G_UNIFORM_LIGHT_ATTENUATION,0},
    {"light_position", G_UNIFORM_LIGHT_POSITION,0},
    {"light_position_object_space", G_UNIFORM_LIGHT_POSITION_OBJECT_SPACE,0},
    {"light_direction", G_UNIFORM_LIGHT_DIRECTION,0},
    {"light_index", G_UNIFORM_LIGHT_INDEX,0},

    {"camera_position", G_UNIFORM_CAMERA_POSITION,0},
    ///物体局部坐标系中的镜头位置
    {"camera_position_object_space", G_UNIFORM_CAMERA_POSITION_OBJECT_SPACE,0},
    {"texture_viewproj_matrix", G_UNIFORM_TEXTURE_VIEWPROJ_MATRIX,0},

    {"time", G_UNIFORM_TIME,0},
    {"frame_time", G_UNIFORM_FRAME_TIME,0},
    {"fps", G_UNIFORM_FPS,0},

    {"time_0_x", G_UNIFORM_TIME_0_X, 1},
    {"cos_time_0_x", G_UNIFORM_COS_TIME_0_X,1},
    {"sin_time_0_x", G_UNIFORM_SIN_TIME_0_X,1},
    {"tan_time_0_x", G_UNIFORM_TAN_TIME_0_X,1},
    {"time_0_x_packed", G_UNIFORM_TIME_0_X_PACKED,1},

    {"time_0_1", G_UNIFORM_TIME_0_1, 1},
    {"cos_time_0_1", G_UNIFORM_COS_TIME_0_1,1},
    {"sin_time_0_1", G_UNIFORM_SIN_TIME_0_1,1},
    {"tan_time_0_1", G_UNIFORM_TAN_TIME_0_1,1},
    {"time_0_1_packed", G_UNIFORM_TIME_0_1_PACKED,1},

    {"texture_size", G_UNIFORM_TEXTURE_SIZE,0},
    {"texture_matrix", G_UNIFORM_TEXTURE_MATRIX,0},
    {"viewport_size", G_UNIFORM_VIEWPORT_SIZE,0},
    {"fov", G_UNIFORM_FOV,0},
    {"custom", G_UNIFORM_CUSTOM,0},
};
gUniformSemantic
find_uniform_semantic   (xcstr          val,
                         greal          *extra)
{
    xchar name[128];
    struct uniform_spec *psepc;
    gUniformSemantic  semantic;
    xstr pExtra;

    strcpy (name, val);
    val = name;
    pExtra = strchr (val, ' ');
    if (pExtra)
        *pExtra++ = '\0';
    semantic = G_UNIFORM_MANUAL;
    *extra = 0;
    psepc = bsearch (&val, _uniform_spec, X_N_ELEMENTS(_uniform_spec),
                     sizeof(struct uniform_spec), name_cmp);
    if (psepc != NULL) {
        semantic = psepc->semantic;
        if (psepc->extra) {
            if (pExtra)
                *extra = (greal)atof(pExtra);
        }
    }
    return semantic;
}
static inline xbool
is_float_uniform        (gUniformType   type)
{
    if (type >= G_UNIFORM_FLOAT1 && type <= G_UNIFORM_MATRIX_4X4)
        return TRUE;
    return FALSE;
}
static gUniformScope
get_uniform_scope       (gUniformSemantic semantic)
{
    switch(semantic) {
    case G_UNIFORM_MANUAL:

    case G_UNIFORM_MODEL_MATRIX:
    case G_UNIFORM_MODEL_AFFINE_ARRAY:

    case G_UNIFORM_NODE_MATRIX:
    case G_UNIFORM_NODEVIEWPROJ_MATRIX:

    case G_UNIFORM_WORLD_MATRIX:
    case G_UNIFORM_INVERSE_WORLD_MATRIX:
    case G_UNIFORM_TRANSPOSE_WORLD_MATRIX:
    case G_UNIFORM_INVERSE_TRANSPOSE_WORLD_MATRIX:
    case G_UNIFORM_WORLD_MATRIX_ARRAY_3x4:
    case G_UNIFORM_WORLD_MATRIX_ARRAY:

    case G_UNIFORM_VIEW_MATRIX:
    case G_UNIFORM_INVERSE_VIEW_MATRIX:
    case G_UNIFORM_TRANSPOSE_VIEW_MATRIX:
    case G_UNIFORM_INVERSE_TRANSPOSE_VIEW_MATRIX:

    case G_UNIFORM_PROJECTION_MATRIX:
    case G_UNIFORM_INVERSE_PROJECTION_MATRIX:
    case G_UNIFORM_TRANSPOSE_PROJECTION_MATRIX:
    case G_UNIFORM_INVERSE_TRANSPOSE_PROJECTION_MATRIX:

    case G_UNIFORM_VIEWPROJ_MATRIX:
    case G_UNIFORM_INVERSE_VIEWPROJ_MATRIX:
    case G_UNIFORM_TRANSPOSE_VIEWPROJ_MATRIX:
    case G_UNIFORM_INVERSE_TRANSPOSE_VIEWPROJ_MATRIX:

    case G_UNIFORM_WORLDVIEW_MATRIX:
    case G_UNIFORM_INVERSE_WORLDVIEW_MATRIX:
    case G_UNIFORM_TRANSPOSE_WORLDVIEW_MATRIX:
    case G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEW_MATRIX:

    case G_UNIFORM_WORLDVIEWPROJ_MATRIX:
    case G_UNIFORM_INVERSE_WORLDVIEWPROJ_MATRIX:
    case G_UNIFORM_TRANSPOSE_WORLDVIEWPROJ_MATRIX:
    case G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX:
        return G_UNIFORM_OBJECT;
    }
    return G_UNIFORM_GLOBAL;
}
static xType
shader_type_type        (void)
{
    static xType shader_type = 0;
    if (!shader_type) {
        static xEnumValue types[]={
            {G_SHADER_VERTEX, "vertex", "vertex shader"},
            {G_SHADER_PIXEL, "pixel", "pixel shader"},
            {G_SHADER_GEOMETRY,"geometry", "geometry shader"},
            {0,0,0}
        };
        shader_type = x_enum_register_static ("gShaderType",types);
    }
    return shader_type;
}
static xHashTable       *_shaders;
X_DEFINE_TYPE (gShader, g_shader, X_TYPE_RESOURCE);
X_DEFINE_TYPE_EXTENDED (gProgram, g_program, X_TYPE_OBJECT, X_TYPE_ABSTRACT,);
static void
g_program_init          (gProgram       *program)
{

}
static void
g_program_class_init    (gProgramClass *klass)
{
}
struct uniform
{
    xstr            name;
    xint            index;
    gUniformType    type;
    xsize           elem_size;
    xsize           array_count;
    gUniformSemantic semantic;
    greal           extra;
    gUniformScope   scope;
    xsize           buf_index;  /* 值在client->fbuf/ibuf 中的序号 */
};
struct client
{
    xfloat      *manual_fbuf;
    xint        *manual_ibuf;
    xfloat      *fbuf;
    xint        *ibuf;
    xHashTable  *manuals;
};
static void
client_free             (struct client  *c)
{
    x_free (c->fbuf);
    x_free (c->ibuf);
    x_slice_free (struct client, c);
}
static void
uniform_free            (struct uniform *u)
{
    x_free (u->name);
    x_slice_free (struct uniform, u);
}
static void
g_shader_init           (gShader       *shader)
{
    shader->programs = x_ptr_array_new ();
    shader->uniforms = x_hash_table_new_full (x_str_hash, x_str_cmp,
                                              NULL, uniform_free);
    shader->clients = x_hash_table_new_full (x_direct_hash, x_direct_cmp,
                                             NULL, client_free);
}
/* shader property */
enum { 
    PROP_0,
    PROP_NAME,
    PROP_TYPE,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    gShader *shader = (gShader*)object;

    switch(property_id) {
    case PROP_NAME:
        if (shader->name) {
            x_hash_table_remove (_shaders, shader->name);
            x_free (shader->name);
        }
        shader->name = x_value_dup_str (value);
        if (shader->name) {
            x_hash_table_insert (_shaders, shader->name, shader);
        }
        break;
    case PROP_TYPE:
        shader->type = x_value_get_enum (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gShader *shader = (gShader*)object;

    switch(property_id) {
    case PROP_NAME:
        x_value_set_str (value, shader->name);
        break;
    case PROP_TYPE:
        x_value_set_enum (value, shader->type);
        break;
    }
}
static void
shader_finalize         (xObject        *object)
{
    gShader *shader = (gShader*)object;

    if (shader->name) {
        x_hash_table_remove (_shaders, shader->name);
        x_free (shader->name);
    }
}
static void
shader_prepare          (xResource      *resource,
                         xbool          backThread)
{
    xsize i;
    gShader *shader = (gShader*)resource;

    for (i=0;i<shader->programs->len;++i) {
        gProgram *program = shader->programs->data[i];
        if (program->supported)
            G_PROGRAM_GET_CLASS(program)->prepare(program, backThread);
    }
}
static calculate_buf(struct uniform *uniform, gShader *shader)
{
    if (uniform->semantic == G_UNIFORM_MANUAL) {
        if (is_float_uniform (uniform->type)) {
            uniform->buf_index = shader->manual_fbuf_size;
            shader->manual_fbuf_size += uniform->elem_size * uniform->array_count;
        }
        else {
            uniform->buf_index = shader->manual_ibuf_size;
            shader->manual_ibuf_size += uniform->elem_size * uniform->array_count;
        }
    }
    else {
        if (is_float_uniform (uniform->type)) {
            uniform->buf_index = shader->fbuf_size;
            shader->fbuf_size += uniform->elem_size * uniform->array_count;
        }
        else {
            uniform->buf_index = shader->ibuf_size;
            shader->ibuf_size += uniform->elem_size * uniform->array_count;
        }
    }
}
static adjust_index(struct uniform *uniform, gShader *shader)
{
    if (uniform->semantic != G_UNIFORM_MANUAL) {
        if (is_float_uniform (uniform->type))
            uniform->buf_index += shader->manual_fbuf_size;
        else
            uniform->buf_index += shader->manual_ibuf_size;
    }
}
static void
shader_load             (xResource      *resource,
                         xbool          backThread)
{
    xsize i;
    gShader *shader = (gShader*)resource;

    for (i=0;i<shader->programs->len;++i) {
        gProgram *program = shader->programs->data[i];
        if (program->supported) {
            G_PROGRAM_GET_CLASS(program)->load (program,backThread);
            if (program->supported) {
                shader->current = program;
                break;
            }
        }
    }
    x_assert(shader->manual_ibuf_size == 0 && shader->manual_fbuf_size == 0);
    x_assert(shader->ibuf_size == 0 && shader->fbuf_size == 0);

    x_hash_table_foreach_val(shader->uniforms, calculate_buf, shader);
    x_hash_table_foreach_val(shader->uniforms, adjust_index, shader);
    shader->ibuf_size += shader->manual_ibuf_size;
    shader->fbuf_size += shader->manual_fbuf_size;
}
static void
shader_unload           (xResource      *resource,
                         xbool          backThread)
{
    //TODO
}
static void
g_shader_class_init     (gShaderClass   *klass)
{
    xParam         *param;
    xObjectClass   *oclass;
    xResourceClass *rclass;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = shader_prepare;
    rclass->load    = shader_load;
    rclass->unload  = shader_unload;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = shader_finalize;

    if (!_shaders)
        _shaders = x_hash_table_new (x_str_hash, x_str_cmp);

    param = x_param_str_new ("name",
                             "Name",
                             "The name of the shader object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_NAME,param);

    param = x_param_enum_new ("type",
                              "Type",
                              "The gShaderType of the shader object",
                              shader_type_type(), G_SHADER_VERTEX,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_TYPE, param);
}

static gShader* shader_get (xcstr name)
{
    if (!_shaders)
        return NULL;
    return x_hash_table_lookup (_shaders, name);
}

void
g_shader_register       (gShader        *shader,
                         gUniform       *guniform)
{
    struct uniform *uniform;

    x_return_if_fail (G_IS_SHADER (shader));
    x_return_if_fail (X_RESOURCE(shader)->state != X_RESOURCE_LOADED);
    x_return_if_fail (guniform != NULL);

    uniform = x_hash_table_lookup(shader->uniforms, guniform->name);
    if (!uniform) {
        uniform = x_slice_new0(struct uniform);
        uniform->name = x_strdup (guniform->name);
        x_hash_table_insert (shader->uniforms, uniform->name, uniform);
    }
    x_return_if_fail (uniform->buf_index == 0 && uniform->type == G_UNIFORM_VOID);
    uniform->index = guniform->index;
    uniform->type  = guniform->type;
    uniform->elem_size = guniform->elem_size;
    uniform->array_count = guniform->array_count;
    if (guniform->semantic != G_UNIFORM_MANUAL) {
        uniform->semantic = guniform->semantic;
        uniform->extra = guniform->extra;
    }
}
void
g_shader_link           (gShader        *shader,
                         xcstr          uniform_name,
                         gUniformSemantic semantic,
                         greal          extra)
{
    struct uniform *uniform;

    x_return_if_fail (G_IS_SHADER (shader));
    x_return_if_fail (X_RESOURCE(shader)->state != X_RESOURCE_LOADED);
    x_return_if_fail (uniform_name != NULL);
    x_return_if_fail (semantic != G_UNIFORM_MANUAL);

    uniform = x_hash_table_lookup(shader->uniforms, uniform_name);
    if (!uniform) {
        uniform = x_slice_new0(struct uniform);
        uniform->name = x_strdup (uniform_name);
        x_hash_table_insert (shader->uniforms, uniform->name, uniform);
    }
    x_return_if_fail (uniform->semantic == G_UNIFORM_MANUAL);

    uniform->semantic = semantic;
    uniform->extra = extra;
    uniform->scope = get_uniform_scope (semantic);
}
gShader*
g_shader_attach         (xcstr          name,
                         xcptr          stub)
{
    gShader* shader;

    x_return_val_if_fail (name != NULL, NULL);

    shader = x_object_ref(shader_get(name));
    if (!shader) return NULL;

    if (stub) {
        struct client *client;

        client = x_hash_table_lookup (shader->clients, stub);
        if (!client) {
            client = x_slice_new0 (struct client);
            x_hash_table_insert (shader->clients, (xptr)stub, client);
        }
    }
    
    return shader;
}
void
g_shader_detach         (gShader        *shader,
                         xcptr          stub)
{
    if (!shader) return;
    x_return_if_fail (G_IS_SHADER (shader));
    if (stub)
        x_hash_table_remove (shader->clients, (xptr)stub);
    x_object_unref (shader);
}

static void
write_uniform           (gShader        *shader,
                         struct client  *client,
                         xcstr          uniform_name,
                         xcptr          data)
{
    struct uniform  *uniform;

    uniform = x_hash_table_lookup (shader->uniforms, uniform_name);
    x_return_if_fail (uniform != NULL);
    x_return_if_fail (uniform->semantic == G_UNIFORM_MANUAL);

    if (!client->manual_fbuf && shader->manual_fbuf_size > 0)
        client->manual_fbuf = x_malloc0(sizeof(xfloat)*shader->manual_fbuf_size);
    if (!client->manual_ibuf && shader->manual_ibuf_size > 0)
        client->manual_ibuf = x_malloc0(sizeof(xint)*shader->manual_ibuf_size);

    if (is_float_uniform (uniform->type)) {
        x_assert (uniform->buf_index+
                  uniform->elem_size * uniform->array_count <= shader->manual_fbuf_size);
        memcpy (client->manual_fbuf+ uniform->buf_index, data,
                uniform->elem_size * uniform->array_count * sizeof(xfloat));
    }
    else {
        x_assert (uniform->buf_index+
                  uniform->elem_size * uniform->array_count <= shader->manual_ibuf_size);
        memcpy (client->manual_ibuf+ uniform->buf_index, data,
                uniform->elem_size * uniform->array_count * sizeof(xint));
    }
}
static void
write_uniform_str       (gShader        *shader,
                         struct client  *client,
                         xcstr          uniform_name,
                         xcstr          str)
{
    xint i;
    xfloat data[16]={0,};
    for (i=0;i<16 && *str;++i)
        data[i] = (xfloat)strtod (str, (char**)&str);
    write_uniform (shader, client, uniform_name, data);
}
void
g_shader_puts           (gShader        *shader,
                         xcptr          stub,
                         xcstr          uniform_name,
                         xcstr          value)
{
    struct client   *client;

    x_return_if_fail (G_IS_SHADER (shader));
    x_return_if_fail (shader->clients != NULL);
    x_return_if_fail (uniform_name != NULL);
    x_return_if_fail (stub != NULL); 
    x_return_if_fail (value != NULL);
    client = x_hash_table_lookup (shader->clients, stub);
    x_return_if_fail (client != NULL);

    if (X_RESOURCE(shader)->state != X_RESOURCE_LOADED) {
        if (!client->manuals)
            client->manuals = x_hash_table_new_full(x_str_hash, x_str_cmp, x_free, x_free);
        x_hash_table_insert(client->manuals, x_strdup (uniform_name), x_strdup (value));
    }
    else {
        write_uniform_str (shader, client, uniform_name, value);
    }
}
void
g_shader_put            (gShader        *shader,
                         xcptr          stub,
                         xcstr          uniform_name,
                         xcptr          data)
{
    struct client   *client;

    x_return_if_fail (G_IS_SHADER (shader));
    x_return_if_fail (X_RESOURCE(shader)->state == X_RESOURCE_LOADED);
    x_return_if_fail (shader->uniforms != NULL);
    x_return_if_fail (shader->clients != NULL);

    x_return_if_fail (uniform_name != NULL);
    x_return_if_fail (stub != NULL);
    x_return_if_fail (data != NULL);


    client = x_hash_table_lookup (shader->clients, stub);
    x_return_if_fail (client != NULL);

    write_uniform (shader, client, uniform_name, data);
}

static void flush_manual(xstr name, xstr val, xptr param[2])
{
    gShader *shader = param[0];
    struct client  *client = param[1];
    write_uniform_str (shader, client, name, val);
}
static void refresh_uniform(struct uniform *uniform, xptr param[4])
{
    gShader *shader = param[0];
    struct client  *client = param[1];
    gSceneRender *render = param[2];
    xsize scope = X_PTR_TO_SIZE(param[3]);

    if ((uniform->scope & scope) == 0)
        return;
    if (is_float_uniform (uniform->type)) {
        x_assert (uniform->buf_index+
            uniform->elem_size * uniform->array_count <= shader->fbuf_size);
        g_scene_render_write_uniform (render, (gUniform*) uniform, client->fbuf+ uniform->buf_index);
    }
    else {
        x_assert (uniform->buf_index+
            uniform->elem_size * uniform->array_count <= shader->ibuf_size);
        g_scene_render_write_uniform (render, (gUniform*) uniform, client->ibuf+ uniform->buf_index);
    }
}
void
g_shader_refresh        (gShader        *shader,
                         xcptr          stub,
                         gSceneRender   *render,
                         gUniformScope  scope)
{
    struct client  *client;

    x_return_if_fail (render != NULL);
    x_return_if_fail (G_IS_SHADER (shader));
    x_return_if_fail (X_RESOURCE(shader)->state == X_RESOURCE_LOADED);

    if (!shader->current)
        return;

    x_return_if_fail (shader->clients != NULL);
    x_return_if_fail (stub != NULL);

    client = x_hash_table_lookup (shader->clients, stub);
    x_return_if_fail (client != NULL);

    if (!client->fbuf && shader->fbuf_size > 0)
        client->fbuf = x_malloc0(sizeof(xfloat)*shader->fbuf_size);
    if (!client->ibuf && shader->ibuf_size > 0)
        client->ibuf = x_malloc0(sizeof(xint)*shader->ibuf_size);

    /* flush cached manual values */
    if (client->manuals) {
        xptr param[2]={shader, client};
        x_hash_table_foreach (client->manuals, flush_manual, param);
        x_hash_table_unref(client->manuals);
        client->manuals = NULL;
    }

    /* override default manual values */
    if (client->manual_fbuf)
        memcpy(client->fbuf, client->manual_fbuf, shader->manual_fbuf_size);
    if (client->manual_ibuf)
        memcpy(client->ibuf, client->manual_ibuf, shader->manual_ibuf_size);
    {
        xptr param[4] = {shader, client, render, (xptr)scope};
        x_hash_table_foreach_val(shader->uniforms, refresh_uniform, param);
    }
}
static void upload_uniform(struct uniform *uniform, xptr param[3])
{
    gShader *shader = param[0];
    struct client  *client = param[1];
    gProgramClass *klass = param[2];

    if (uniform->type == G_UNIFORM_VOID) return;

    if (is_float_uniform (uniform->type))
        klass->upload (shader->current, (gUniform*)uniform, client->fbuf+uniform->buf_index);
    else
        klass->upload (shader->current, (gUniform*)uniform, client->ibuf+uniform->buf_index);
}
void
g_shader_upload         (gShader        *shader,
                         xcptr          stub)
{
    struct client  *client;
    gProgramClass *klass;

    x_return_if_fail (G_IS_SHADER (shader));
    x_return_if_fail (G_IS_PROGRAM (shader->current));

    x_return_if_fail (shader->clients != NULL);
    x_return_if_fail (stub != NULL);

    client = x_hash_table_lookup (shader->clients, stub);
    x_return_if_fail (client != NULL);

    klass = G_PROGRAM_GET_CLASS (shader->current);
    {
        xptr param[3] = {shader, client, klass};
        x_hash_table_foreach_val(shader->uniforms,upload_uniform, param);
    }
}
void
g_shader_bind           (gShader        *shader)
{
    gProgramClass *klass;
    x_return_if_fail (G_IS_SHADER (shader));

    if (x_atomic_int_inc(&shader->bound) == 1) {
        x_return_if_fail (G_IS_PROGRAM (shader->current));

        klass = G_PROGRAM_GET_CLASS (shader->current);
        klass->bind (shader->current);
    }
}
void
g_shader_unbind         (gShader        *shader)
{
    if (!shader) return;
    x_return_if_fail (G_IS_SHADER (shader));

    if (x_atomic_int_get(&shader->bound) > 0) {
        if (x_atomic_int_dec(&shader->bound) == 0) {
            gProgramClass *klass;

            x_return_if_fail (G_IS_PROGRAM (shader->current));
            klass = G_PROGRAM_GET_CLASS (shader->current);
            klass->unbind (shader->current);
        }
    }
}
static xptr
on_shader_enter (xptr parent, xcstr name, xcstr group, xQuark parent_name)
{
    gShader *shader = shader_get (name);
    if (!shader) {
        shader = x_object_new (G_TYPE_SHADER,
                               "name", name,
                               "group", group, NULL);
    }
    return shader;
}
static xptr
on_program_enter (xptr  parent, xcstr val,xcstr group, xQuark parent_name)
{
    gProgram *program;
    x_return_val_if_fail (G_IS_SHADER (parent), NULL);

    program = g_render_new_program(val, NULL);
    if (program) {
        program->shader = parent;
        x_ptr_array_append1 (program->shader->programs,  program);
    }
    else {
        x_debug ("shader `%s' have no program type `%s'", G_SHADER(parent)->name, val);
    }

    return program;
}

static void
on_uniforms_visit       (xptr           object,
                         xcstr          key,
                         xcstr          val)
{
    gUniformSemantic semantic;
    greal extra;
    semantic = find_uniform_semantic (val, &extra);
    if (semantic != G_UNIFORM_MANUAL)
        g_shader_link (object, key, semantic, extra);
}

void
_g_shader_init          (void)
{
    xScript *script;
    qsort(_uniform_spec, X_N_ELEMENTS(_uniform_spec),
          sizeof(struct uniform_spec), name_cmp);
    x_set_script_priority ("shader", 90);
    script = x_script_set (NULL, x_quark_from_static("shader"), on_shader_enter, x_object_set_str, NULL);
    x_script_set (script, x_quark_from_static("program"), on_program_enter, x_object_set_str, NULL);
    x_script_set (script, x_quark_from_static("uniforms"), NULL, on_uniforms_visit, NULL);
}
