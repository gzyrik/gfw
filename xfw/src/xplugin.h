#ifndef __X_PLUGIN_H__
#define __X_PLUGIN_H__
#include "xtype.h"
X_BEGIN_DECLS

#define X_TYPE_PULGIN                (x_codec_type())
#define X_PULGIN(object)             (X_INSTANCE_CAST((object), X_TYPE_PULGIN, xCodec))
#define X_PULGIN_CLASS(klass)        (X_CLASS_CAST((klass), X_TYPE_PULGIN, xCodecClass))
#define X_IS_PULGIN(object)          (X_INSTANCE_IS_TYPE((object), X_TYPE_PULGIN))
#define X_IS_PULGIN_CLASS(klass)     (X_CLASS_IS_TYPE((klass), X_TYPE_PULGIN))
#define X_PULGIN_GET_CLASS(object)   (X_INSTANCE_GET_CLASS((object), X_TYPE_PULGIN, xCodecClass))

struct _xPulgin
{
    xTypeModule         parent;
};
struct _xCodecClass
{
    xTypeModuleClass    parent;
    xbyte*  (*decode)   (xCodec*, xFile*, xsize*);
    xbyte*  (*encode)   (xCodec*, xFile*, xsize*);
};

#define X_TYPE_IMAGE_PULGIN                (x_image_codec_type())
#define X_IMAGE_PULGIN(object)             (X_INSTANCE_CAST((object), X_TYPE_IMAGE_PULGIN, xImageCodec))
#define X_IMAGE_PULGIN_CLASS(klass)        (X_CLASS_CAST((klass), X_TYPE_IMAGE_PULGIN, xImageCodecClass))
#define X_IS_IMAGE_PULGIN(object)          (X_INSTANCE_IS_TYPE((object), X_TYPE_IMAGE_PULGIN))
#define X_IS_IMAGE_PULGIN_CLASS(klass)     (X_CLASS_IS_TYPE((klass), X_TYPE_IMAGE_PULGIN))
#define X_IMAGE_PULGIN_GET_CLASS(object)   (X_INSTANCE_GET_CLASS((object), X_TYPE_IMAGE_PULGIN, xImageCodecClass))
struct _xImageCodec
{
    xCodec          parent;
    xsize           width;
    xsize           height;
    xsize           depth;
    xsize           mipmaps;
    xint            format;
};
struct _xImageCodecClass
{
    xCodecClass     parent;
};

xType       x_codec_type            (void);

xType       x_image_codec_type      (void);

xbyte*      x_codec_decode          (xCodec         *codec,
                                     xFile          *file,
                                     xsize          *outlen);

xbyte*      x_codec_encode          (xCodec         *codec,
                                     xFile          *file,
                                     xsize          *outlen);

X_END_DECLS
#endif /* __X_PLUGIN_H__ */
