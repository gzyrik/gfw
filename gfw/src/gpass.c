#include "config.h"
#include "gpass.h"
#include "grender.h"
#include "gtexunit.h"
#include "gshader.h"

static xType
cull_mode_type (void)
{
    static xType type = 0;
    if (!type) {
        static xEnumValue values[]={
            {G_CULL_NONE, "none", "vertex shader"},
            {G_CULL_CW, "cw", "pixel shader"},
            {G_CULL_CCW,"ccw", "geometry shader"},
            {0,0,0}
        };
        type = x_enum_register_static ("gCullMode",values);
    }
    return type;
}

static xType
fill_mode_type (void)
{
    static xType type = 0;
    if (!type) {
        static xEnumValue values[]={
            {G_FILL_POINT, "point", "vertex shader"},
            {G_FILL_WIREFRAME, "wireframe", "pixel shader"},
            {G_FILL_SOLID,"solid", "geometry shader"},
            {0,0,0}
        };
        type = x_enum_register_static ("gFillMode",values);
    }
    return type;
}

X_DEFINE_TYPE (gPass, g_pass, X_TYPE_RESOURCE);
enum {
    PROP_0,
    PROP_CULL_MODE,
    PROP_FILL_MODE,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gPass *pass = (gPass*)object;

    switch(property_id) {
    case PROP_CULL_MODE:
        pass->cull_mode = x_value_get_enum (value);
        break;
    case PROP_FILL_MODE:
        pass->fill_mode = x_value_get_enum (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gPass *pass = (gPass*)object;

    switch(property_id) {
    case PROP_CULL_MODE:
        x_value_set_enum (value, pass->cull_mode);
        break;
    case PROP_FILL_MODE:
        x_value_set_enum (value, pass->fill_mode);
        break;
    }
}
static void
g_pass_init             (gPass          *pass)
{
    pass->shaders  = x_ptr_array_new_full(4, x_object_unref);
    pass->texUnits  = x_ptr_array_new_full(8, x_object_unref);

}

static void
pass_finalize           (xObject        *object)
{
    gPass *pass = (gPass*)object;

    x_free (pass->name);
    pass->name = NULL;

    x_ptr_array_unref(pass->shaders);
    pass->shaders = NULL;

    x_ptr_array_unref(pass->texUnits);
    pass->texUnits = NULL;
}

static void
pass_prepare            (xResource      *resource,
                         xbool          backThread)
{
    gPass *pass=(gPass*)resource;

    x_ptr_array_foreach (pass->shaders, x_resource_prepare, (xptr)backThread);
    x_ptr_array_foreach (pass->texUnits, x_resource_prepare, (xptr)backThread);
}
static void
pass_load               (xResource      *resource,
                         xbool          backThread)
{
    gPass *pass=(gPass*)resource;

    x_ptr_array_foreach (pass->shaders, x_resource_load, (xptr)backThread);
    x_ptr_array_foreach (pass->texUnits, x_resource_load, (xptr)backThread);
}
static void
pass_unload (xResource *resource, xbool backThread)
{
    gPass *pass=(gPass*)resource;

    x_ptr_array_foreach (pass->shaders, x_resource_unload, (xptr)backThread);
    x_ptr_array_foreach (pass->texUnits, x_resource_unload, (xptr)backThread);

}
static void
g_pass_class_init       (gPassClass     *klass)
{
    xObjectClass    *oclass;
    xResourceClass  *rclass;
    xParam          *param;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = pass_prepare;
    rclass->load = pass_load;
    rclass->unload = pass_unload;
    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize = pass_finalize;

    param = x_param_enum_new ("cull-mode",
                              "Cull mode",
                              "The cull mode of the pass object",
                              cull_mode_type(), G_CULL_NONE,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_CULL_MODE, param);

    param = x_param_enum_new ("fill-mode",
                              "Fill mode",
                              "The fill mode of the pass object",
                              fill_mode_type(), G_FILL_SOLID,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_FILL_MODE, param);
}

gTexUnit*
g_pass_add_texunit      (gPass          *pass,
                         xcstr          name)
{
    gTexUnit *texUnit;

    texUnit = x_object_new (G_TYPE_TEXUNIT, NULL);
    texUnit->name = x_strdup (name);
    texUnit->pass = pass;
    texUnit->index = pass->texUnits->len;
    x_ptr_array_append1 (pass->texUnits, texUnit);

    return texUnit;
}

gShader*
g_pass_set_shader       (gPass          *pass,
                         xcstr          name)
{
    gShader *shader;

    shader = g_shader_attach (name, pass);
    x_return_val_if_fail (shader != NULL, NULL);

    x_ptr_array_append1 (pass->shaders, shader);

    return shader;
}

void
g_pass_foreach_shader   (gPass          *pass,
                         xCallback      func,
                         xptr           user_data)
{
    x_ptr_array_foreach(pass->shaders, func, user_data);
}