#include "config.h"
#include "gtechnique.h"
#include "gpass.h"
#include "grender.h"

X_DEFINE_TYPE (gTechnique, g_technique, X_TYPE_RESOURCE);

static void
g_technique_init        (gTechnique     *technique)
{
    technique->passes = x_ptr_array_new_full(4, x_object_unref);
}

static void
technique_finalize      (xObject        *object)
{
    gTechnique *technique = (gTechnique*)object;

    x_ptr_array_unref(technique->passes);
    technique->passes = NULL;
}

static void
technique_prepare(xResource *resource, xbool backThread)
{
    gTechnique *technique= (gTechnique*)resource;

    x_ptr_array_foreach(technique->passes, x_resource_prepare, (xptr)backThread);
}
static void
technique_load (xResource *resource, xbool  backThread)
{
    gTechnique *technique= (gTechnique*)resource;

    x_ptr_array_foreach(technique->passes, x_resource_load, (xptr)backThread);
}
static void technique_unload (xResource *resource, xbool backThread)
{
    gTechnique *technique= (gTechnique*)resource;

    x_ptr_array_foreach(technique->passes, x_resource_unload, (xptr)backThread);
}

static void
g_technique_class_init  (gTechniqueClass *klass)
{
    xResourceClass *rclass;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = technique_prepare;
    rclass->load = technique_load;
    rclass->unload = technique_unload;
}

gPass*
g_technique_add_pass    (gTechnique     *technique,
                         xcstr          name)
{
    gPass *pass;

    pass = x_object_new (G_TYPE_PASS, NULL);
    pass->name = x_strdup (name);
    pass->technique = technique;
    x_ptr_array_append1 (technique->passes, pass);

    return pass;
}
void
g_technique_foreach     (gTechnique     *technique,
                         xCallback      func,
                         xptr           user_data)
{
    x_ptr_array_foreach(technique->passes, func, user_data);
}