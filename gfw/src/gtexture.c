#include "config.h"
#include "gtexture.h"
#include "gbuffer.h"
#include "grender.h"
#include "gfw-prv.h"
#include <string.h>

static xHashTable       *_textures;
X_DEFINE_TYPE_EXTENDED (gTexture, g_texture, X_TYPE_RESOURCE, X_TYPE_ABSTRACT,);

static xType
texture_type_type       (void)
{
    static xType texture_type = 0;
    if (!texture_type) {
        static xEnumValue types[]={
            {G_TEXTURE_1D, "1d", "1d texture"},
            {G_TEXTURE_2D, "2d", "2d texture"},
            {G_TEXTURE_CUBE,"cube", "cube texture"},
            {G_TEXTURE_3D, "3d", "3d texture"},
            {0,0,0}
        };
        texture_type = x_enum_register_static ("gTextureType",types);
    }
    return texture_type;
}
static void
g_texture_init          (gTexture       *texture)
{
    texture->type = G_TEXTURE_2D;
}
/* texture property */
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
    gTexture *tex = (gTexture*)object;
    xResource* res = (xResource*)object;

    switch(property_id) {
    case PROP_NAME:
        if (tex->name) {
            x_hash_table_remove (_textures, tex->name);
            x_free (tex->name);
        }
        tex->name = x_value_dup_str (value);
        if (tex->name) {
            x_hash_table_insert (_textures, tex->name, tex);
        }
        break;
    case PROP_TYPE:
        x_return_if_fail (res->state == X_RESOURCE_UNLOADED);
        tex->type = x_value_get_enum (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gTexture *tex = (gTexture*)object;

    switch(property_id) {
    case PROP_NAME:
        x_value_set_str (value, tex->name);
        break;
    case PROP_TYPE:
        x_value_set_enum (value, tex->type);
        break;
    }
}
static void
texture_finalize        (xObject        *object)
{
    gTexture *tex = (gTexture*)object;

    if (tex->name) {
        x_hash_table_remove (_textures, tex->name);
        x_free (tex->name);
    }
}
static void texture_prepare (xResource *resource, xbool backThread)
{
    gTexture *tex = (gTexture*)resource;

    if (tex->name) {
        xImage *image = x_image_load (tex->name,
                                      resource->group,
                                      tex->type == G_TEXTURE_CUBE);
        if (image) {
            x_return_if_fail (image->width>0 || image->height>0);
            tex->width = image->width;
            tex->height = image->height;
            tex->depth = image->depth;
            tex->faces = image->faces;
            if (tex->format == X_PIXEL_UNKNOWN)
                tex->format = image->format;
            if (image->mipmaps > 0) {
                tex->mipmaps = image->mipmaps;
                tex->flags &= ~G_TEXTURE_AUTO_MIPMAP;
            }
            if (tex->depth > 1)
                tex->type = G_TEXTURE_3D;
            else if (tex->faces > 1)
                tex->type = G_TEXTURE_CUBE;
            else if (tex->width ==1 || tex->height == 1)
                tex->type = G_TEXTURE_1D;
            else
                tex->type = G_TEXTURE_2D;
            tex->image = image;
        }
    }
}

static void texture_load (xResource *resource, xbool backThread)
{
    xsize mip,face;
    gTexture *tex = (gTexture*)resource;
    xImage *image = tex->image;

    if (!image) return;
    tex->image = NULL;
    if (!tex->buffers){
        x_image_unref (image);
        x_warning ("no texture pixel buffers");
        return;
    }
    for (face=0; face<image->faces; ++face) {
        for (mip=0;mip<=image->mipmaps; ++mip) {
            gPixelBuffer *buffer;
            xPixelBox pixel_box;
            xsize idx = face*(tex->mipmaps+1) + mip;
            if (idx >= tex->buffers->len)
                break;
            buffer = tex->buffers->data[idx];
            pixel_box = x_image_pixel_box (image, face, mip);
            g_pixel_buffer_write (G_PIXEL_BUFFER (buffer), &pixel_box, NULL);
        }
    }
    x_image_unref (image);
}
static void texture_unload (xResource *resource, xbool backThread)
{

}
static void
g_texture_class_init    (gTextureClass  *klass)
{
    xParam         *param;
    xObjectClass   *oclass;
    xResourceClass *rclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = texture_finalize;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = texture_prepare;
    rclass->load = texture_load;
    rclass->unload = texture_unload;

    if (!_textures)
        _textures = x_hash_table_new (x_str_hash, x_str_cmp);

    param = x_param_str_new ("name",
                             "Name",
                             "The name of the texture object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_NAME, param);

    param = x_param_enum_new ("type",
                              "Type",
                              "the gTextureType of the texture object",
                              texture_type_type(), G_TEXTURE_2D,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_TYPE, param);
}

gTexture*
g_texture_get           (xcstr          name)
{
    x_return_val_if_fail (name != NULL, NULL);
    if (!_textures)
        return NULL;

    return x_hash_table_lookup (_textures, name);
}

void
g_texture_bind          (gTexture       *texture,
                         xsize          index)
{
    gTextureClass *klass;
    x_return_if_fail (G_IS_TEXTURE (texture));

    if (texture->buffers) {
        xsize i;
        for (i=0; i<texture->buffers->len; ++i)
            g_buffer_upload (texture->buffers->data[i]);
    }
    klass = G_TEXTURE_GET_CLASS (texture);
    klass->bind (texture, index);
}

void
g_texture_unbind        (gTexture       *texture,
                         xsize          index)
{
    gTextureClass *klass;
    x_return_if_fail (G_IS_TEXTURE (texture));

    klass = G_TEXTURE_GET_CLASS (texture);
    klass->unbind (texture, index);
}
