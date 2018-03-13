#ifndef __X_CODEC_H__
#define __X_CODEC_H__
#include "xpixel.h"
X_BEGIN_DECLS

#define X_TYPE_CODEC                (x_codec_type())
#define X_CODEC(object)             (X_INSTANCE_CAST((object), X_TYPE_CODEC, xCodec))
#define X_CODEC_CLASS(klass)        (X_CLASS_CAST((klass), X_TYPE_CODEC, xCodecClass))
#define X_IS_CODEC(object)          (X_INSTANCE_IS_TYPE((object), X_TYPE_CODEC))
#define X_IS_CODEC_CLASS(klass)     (X_CLASS_IS_TYPE((klass), X_TYPE_CODEC))
#define X_CODEC_GET_CLASS(object)   (X_INSTANCE_GET_CLASS((object), X_TYPE_CODEC, xCodecClass))

xbyte*      x_codec_decode          (xCodec         *codec,
                                     xcstr          filename,
                                     xFile          *file,
                                     xsize          *outlen);

xbyte*      x_codec_encode          (xCodec         *codec,
                                     xcstr          filename,
                                     xFile          *file,
                                     xsize          *outlen);

#define X_TYPE_IMAGE_CODEC                (x_image_codec_type())
#define X_IMAGE_CODEC(object)             (X_INSTANCE_CAST((object), X_TYPE_IMAGE_CODEC, xImageCodec))
#define X_IMAGE_CODEC_CLASS(klass)        (X_CLASS_CAST((klass), X_TYPE_IMAGE_CODEC, xImageCodecClass))
#define X_IS_IMAGE_CODEC(object)          (X_INSTANCE_IS_TYPE((object), X_TYPE_IMAGE_CODEC))
#define X_IS_IMAGE_CODEC_CLASS(klass)     (X_CLASS_IS_TYPE((klass), X_TYPE_IMAGE_CODEC))
#define X_IMAGE_CODEC_GET_CLASS(object)   (X_INSTANCE_GET_CLASS((object), X_TYPE_IMAGE_CODEC, xImageCodecClass))

struct _xCodec
{
    xObject             parent;
};
struct _xCodecClass
{
    xObjectClass        parent;
    xbyte*  (*decode)   (xCodec *codec, xcstr filename, xFile *file, xsize *out_len);
    xbyte*  (*encode)   (xCodec *codec, xcstr filename, xFile *file, xsize *out_len);
};

struct _xImageCodec
{
    xCodec          parent;
    xsize           width;
    xsize           height;
    xsize           depth;
    xsize           mipmaps;
    xPixelFormat    format;
};
struct _xImageCodecClass
{
    xCodecClass     parent;
};

xType       x_codec_type            (void);

xType       x_image_codec_type      (void);


X_END_DECLS
#endif /* __X_CODEC_H__ */
