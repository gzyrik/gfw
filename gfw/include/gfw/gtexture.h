#ifndef __G_TEXTURE_H__
#define __G_TEXTURE_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_TEXTURE              (g_texture_type())
#define G_TEXTURE(object)           (X_INSTANCE_CAST((object), G_TYPE_TEXTURE, gTexture))
#define G_TEXTURE_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_TEXTURE, gTextureClass))
#define G_IS_TEXTURE(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_TEXTURE))
#define G_TEXTURE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_TEXTURE, gTextureClass))

typedef enum {
    G_TEXTURE_AUTO_MIPMAP,
    G_TEXTURE_RENDER_TARGET,
    G_TEXTURE_MIP_UNLIMITED
} gTextureFlags;

typedef enum {
    G_TEXTURE_1D,
    G_TEXTURE_2D,
    G_TEXTURE_CUBE,
    G_TEXTURE_3D
} gTextureType;

gTexture*   g_texture_new           (xcstr          file,
                                     xcstr          group,
                                     xcstr          first_property,
                                     ...);

gTexture*   g_texture_get           (xcstr          name);

/* bind to gpu */
void        g_texture_bind          (gTexture       *texture,
                                     xsize          index);

/* unbind from gpu */
void        g_texture_unbind        (gTexture       *texture,
                                     xsize          index);

struct _gTexture
{
    xResource                       parent;
    xstr                            name;
    gTextureType                    type;
    gTextureFlags                   flags;
    xsize                           width;
    xsize                           height;
    xsize                           depth;
    xsize                           faces;
    xsize                           mipmaps;
    xPixelFormat                    format;
    xPtrArray                       *buffers;
    xImage                          *image; /* used for image load */
};
struct _gTextureClass
{
    xResourceClass                  parent;
    void (*bind)   (gTexture *texture, xsize index);
    void (*unbind) (gTexture *texture, xsize index);
};


xType       g_texture_type          (void);

X_END_DECLS
#endif /* __G_TEXTURE_H__ */
