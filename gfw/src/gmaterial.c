#include "config.h"
#include "gmaterial.h"
#include "gtexture.h"
#include "grender.h"
#include "gshader.h"
#include "gpass.h"
#include "gtexunit.h"
#include "gtechnique.h"
#include "gtexture.h"
#include <string.h>
#include <stdlib.h>
static xHashTable      *_materials;
X_DEFINE_TYPE (gMaterial, g_material, X_TYPE_RESOURCE);
struct client
{
    xfloat      *manual_fbuf;
    xint        *manual_ibuf;
    xfloat      *fbuf;
    xint        *ibuf;
    xHashTable  *manuals;
};
/* materail property */
enum {
    PROP_0,
    PROP_NAME,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gMaterial *mat = (gMaterial*)object;

    switch(property_id) {
    case PROP_NAME:
        if (mat->name) {
            x_hash_table_remove (_materials, mat->name);
            x_free (mat->name);
        }
        mat->name = x_value_dup_str (value);
        if (mat->name) {
            x_hash_table_insert (_materials, mat->name, mat);
        }
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gMaterial *mat = (gMaterial*)object;

    switch(property_id) {
    case PROP_NAME:
        x_value_set_str (value, mat->name);
        break;
    }
}
static void
g_material_init         (gMaterial      *material)
{
    material->techniques = x_ptr_array_new_full(4, x_object_unref);
}
static void
material_prepare        (xResource      *resource,
                         xbool          backThread)
{
    gMaterial *material = (gMaterial*)resource;

    x_ptr_array_foreach(material->techniques, x_resource_prepare, (xptr)backThread);
}
static void
material_load           (xResource      *resource,
                         xbool          backThread)
{
    gMaterial *material = (gMaterial*)resource;

    x_ptr_array_foreach(material->techniques, x_resource_load, (xptr)backThread);
}
static void
material_finalize       (xObject        *object)
{
    gMaterial *material = (gMaterial*)object;

    if (material->name) {
        x_hash_table_remove (_materials, material->name);
        x_free (material->name);
        material->name = NULL;
    }
    x_ptr_array_unref(material->techniques);
    material->techniques = NULL;
}
static void
g_material_class_init   (gMaterialClass *klass)
{
    xParam         *param;
    xResourceClass *rclass ;
    xObjectClass   *oclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = material_finalize;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = material_prepare;
    rclass->load = material_load;
    if (!_materials)
        _materials = x_hash_table_new (x_str_hash, x_str_cmp);

    param = x_param_str_new ("name",
                             "Name",
                             "The name of the material object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);

    x_oclass_install_param(oclass, PROP_NAME,param);
}
static gMaterial* material_get (xcstr name)
{
    if (!_materials)
        return NULL;
    return x_hash_table_lookup (_materials, name);
}
gMaterial*
g_material_attach       (xcstr          name,
                         xcptr          stub)
{
    gMaterial* material;

    x_return_val_if_fail (name != NULL, NULL);

    material = x_object_ref(material_get(name));
    if (!material) return NULL;
/*
    if (stub) {
        struct client *client;

        client = x_hash_table_lookup (material->clients, stub);
        if (!client) {
            client = x_slice_new0 (struct client);
            x_hash_table_insert (material->clients, (xptr)stub, client);
        }
    }
*/
    return material;
}
void
g_material_detach       (gMaterial      *material,
                         xcptr          stub)
{
    if (!material) return;
    x_return_if_fail (G_IS_MATERIAL (material));

    if (stub){
        //x_hash_table_remove (material->clients, (xptr)stub);
    }
    x_object_unref (material);
}
gTechnique*
g_material_add_technique(gMaterial      *material,
                         xcstr          name)
{
    gTechnique *technique;

    technique  = x_object_new (G_TYPE_TECHNIQUE, NULL);
    technique->material = material;
    technique->name = x_strdup (name);

    x_ptr_array_append1(material->techniques, technique);

    return technique;
}
gTechnique*
g_material_technique    (gMaterial      *material)
{
    x_return_val_if_fail (G_IS_MATERIAL(material), NULL);

    return material->techniques->data[0];
}
/*
gPass*
g_material_pass         (gMaterial      *material,
                         xsize          tech_index,
                         xsize          pass_index)
{
    gTechnique* technique;
    technique = g_material_technique (material, tech_index);
    x_return_val_if_fail (technique != NULL, NULL);
    x_return_val_if_fail (pass_index < technique->passes->len, NULL);

    return technique->passes->data[pass_index];
}

gTexUnit*
g_material_texunit      (gMaterial      *material,
                         xsize          tech_index,
                         xsize          pass_index,
                         xsize          texUnit_index)
{
    gPass *pass;
    x_return_val_if_fail (texUnit_index < G_MAX_N_TEXUNITS, NULL);

    pass = g_material_pass (material, tech_index, pass_index);
    x_return_val_if_fail (pass != NULL, NULL);
    x_return_val_if_fail (texUnit_index < pass->texUnits->len, NULL);


    return pass->texUnits->data[texUnit_index];
}
gTexture*
g_material_texture      (gMaterial      *material,
xsize          tech_index,
xsize          pass_index,
xsize          texUnit_index,
xsize          frame_index)
{
gTexUnit *texUnit;

texUnit = g_material_texunit (material, tech_index, pass_index, texUnit_index);
x_return_val_if_fail (texUnit != NULL, NULL);

return g_texunit_texture (texUnit, frame_index);
}
*/
static void pass_3d(gPass *pass)
{
}
xbool
g_material_is_3d        (gMaterial      *material)
{
    gTechnique* technique;
    xbool is_3d;
    technique = g_material_technique (material);
    x_return_val_if_fail (technique != NULL, FALSE);

    g_technique_foreach(technique, pass_3d, NULL);
}

void
g_material_apply_textures (gMaterial      *material,
                           xptr           client,
                           xHashTable     *texture_aliases)
{

}
static xptr
on_material_enter       (xptr           parent,
                         xcstr          name,
                         xcstr          group,
                         xQuark         parent_name)
{
    gMaterial *mat = material_get (name);
    if (!mat) {
        mat = x_object_new (G_TYPE_MATERIAL,
                            "name", name,
                            "group", group, NULL);
    }
    return mat;
}
struct shader_pass
{
    gShader     *shader;
    gPass       *pass;
};
static xptr
on_shader_enter (xptr pass, xcstr name, xcstr group, xQuark parent_name)
{
    struct shader_pass *ret;
    ret = x_slice_new (struct shader_pass);
    ret->shader = g_pass_set_shader (pass, name);
    ret->pass = pass;
    return ret;
}
static void
on_shader_visit         (xptr           object,
                         xcstr          key,
                         xcstr          val)
{
    struct shader_pass *ret = object;
    g_shader_puts (ret->shader, ret->pass, key, val);
}
static void
on_shader_leave         (xptr           object,
                         xptr           parent,
                         xcstr          group)
{
    x_slice_free (struct shader_pass, object);
}
static xptr
on_texture_enter        (xptr           texunit,
                         xcstr          name,
                         xcstr          group,
                         xQuark         parent_name)
{
    gTexture *tex;
    tex = g_texture_new (name, group, NULL);
    g_texunit_set_texture (texunit, tex);
    return tex;
}
void
_g_material_init        (void)
{
    xScript *mat, *tech, *pass, *texunit;
    x_set_script_priority ("material", 100);
    mat = x_script_set (NULL, x_quark_from_static("material"),
        on_material_enter, x_object_set_str, NULL);
    tech = x_script_set (mat, x_quark_from_static("technique"),
        (xScriptEnter)g_material_add_technique, x_object_set_str, NULL);
    pass = x_script_set (tech, x_quark_from_static("pass"),
        (xScriptEnter)g_technique_add_pass, x_object_set_str, NULL);
    texunit = x_script_set (pass, x_quark_from_static("texunit"), 
        (xScriptEnter)g_pass_add_texunit, x_object_set_str, NULL);
    x_script_set (pass, x_quark_from_static("shader"), 
        on_shader_enter, on_shader_visit, on_shader_leave);
    x_script_set (texunit, x_quark_from_static("texture"),
        on_texture_enter, x_object_set_str, NULL);
}
