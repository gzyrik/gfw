#include "config.h"
#include "gtexunit.h"
#include "gtexture.h"
#include "grender.h"
#include "gpass.h"
#include "gmaterial.h"
#include "gtechnique.h"

X_DEFINE_TYPE (gTexUnit, g_texunit, X_TYPE_RESOURCE);

static void
g_texunit_init          (gTexUnit       *texunit)
{
    texunit->textures = x_ptr_array_new ();
    texunit->textures->free_func = x_object_unref;
}

static void
texunit_finalize        (xObject        *object)
{
    gTexUnit *texunit = (gTexUnit*)object;

    x_free (texunit->name);
    x_ptr_array_unref (texunit->textures);
}
static void texunit_prepare (xResource *resource, xbool backThread)
{
    gTexUnit *texunit = (gTexUnit*)resource;
    x_ptr_array_foreach (texunit->textures, x_resource_prepare, (xptr)backThread);
}
static void texunit_load (xResource *resource, xbool backThread)
{
    gTexUnit *texunit = (gTexUnit*)resource;
    x_ptr_array_foreach (texunit->textures, x_resource_load, (xptr)backThread);
}
static void texunit_unload (xResource *resource, xbool backThread)
{
    gTexUnit *texunit = (gTexUnit*)resource;
    x_ptr_array_foreach (texunit->textures, x_resource_unload, (xptr)backThread);
}
static void
g_texunit_class_init    (gTexUnitClass  *klass)
{
    xObjectClass *oclass;
    xResourceClass *rclass;
    
    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = texunit_prepare;
    rclass->load = texunit_load;
    rclass->unload = texunit_unload;

    oclass = X_OBJECT_CLASS (klass);
    oclass->finalize = texunit_finalize;
}

void
g_texunit_bind          (gTexUnit       *texUnit)
{
    x_return_if_fail (G_IS_TEXUNIT (texUnit));

    if (x_atomic_int_inc(&texUnit->bound) == 1) {
        gTexture *tex;

        tex = texUnit->textures->data[texUnit->frame];
        g_texture_bind (tex, texUnit->index);
    }
}
void
g_texunit_unbind        (gTexUnit       *texUnit)
{
    if (!texUnit) return;
    x_return_if_fail (G_IS_TEXUNIT (texUnit));

    if (x_atomic_int_get(&texUnit->bound) > 0) {
        if (x_atomic_int_dec(&texUnit->bound) == 0) {
            gTexture *tex;

            tex = texUnit->textures->data[texUnit->frame];
            g_texture_unbind (tex, texUnit->index);
        }
    }
}

void
g_texunit_set_texture   (gTexUnit       *texUnit,
                         gTexture       *texture)
{
    x_return_if_fail (texture != NULL);

    x_ptr_array_append1 (texUnit->textures, texture);
    texUnit->pass->technique->material->dirty = TRUE;
}

gTexture*
g_texunit_texture       (gTexUnit       *texUnit,
                         xsize          frame_index)
{
    x_return_val_if_fail(frame_index < texUnit->textures->len, NULL);

    return texUnit->textures->data[frame_index];
}

void
g_texunit_apply_textures(gTexUnit       *texUnit,
                         xHashTable     *texture_aliases)
{
    xsize i;
    for(i=0;i<texUnit->textures->len;++i) {
    }
}