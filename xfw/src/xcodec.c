#include "config.h"
#include "xcodec.h"
X_DEFINE_TYPE_EXTENDED (xCodec, x_codec, X_TYPE_OBJECT, X_TYPE_ABSTRACT, );
static void
x_codec_init            (xCodec         *codec)
{
}


static void
x_codec_class_init      (xCodecClass    *klass)
{
}

xbyte*
x_codec_decode          (xCodec         *codec,
                         xcstr          filename,
                         xFile          *file,
                         xsize          *outlen)
{
    xCodecClass *klass;
    x_return_val_if_fail (X_IS_CODEC (codec), 0);

    klass = X_CODEC_GET_CLASS (codec);

    return klass->decode (codec, filename, file, outlen);
}

xbyte*
x_codec_encode          (xCodec         *codec,
                         xcstr          filename,
                         xFile          *file,
                         xsize          *outlen)
{
    xCodecClass *klass;
    x_return_val_if_fail (X_IS_CODEC (codec), 0);

    klass = X_CODEC_GET_CLASS (codec);

    return klass->encode (codec, filename, file, outlen);
}

X_DEFINE_TYPE_EXTENDED (xImageCodec, x_image_codec, X_TYPE_CODEC, X_TYPE_ABSTRACT, );
static void
x_image_codec_init      (xImageCodec    *codec)
{
}


static void
x_image_codec_class_init(xImageCodecClass     *klass)
{
}
