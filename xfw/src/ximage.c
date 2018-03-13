#include "config.h"
#include "ximage.h"
#include "xcodec.h"
#include "xresource.h"
#include "xpixel.h"
#include <string.h>

xImage*
x_image_load            (xcstr          filename,
                         xcstr          group,
                         xbool          cube)
{
    xType type;
    xcstr dot;
    xstr name = NULL;
    xFile *file = NULL;
    xImage *image = NULL;
    xImageCodec *codec= NULL;
    xbyte *outdat;
    xsize outlen;

    x_return_val_if_fail (filename != NULL, NULL);
    /* suffix is codec type */
    dot = strrchr (filename, '.');
    x_return_val_if_fail (dot != NULL, NULL);

    type = x_type_from_mime (X_TYPE_IMAGE_CODEC, dot+1);
    x_return_val_if_fail (type != X_TYPE_INVALID, NULL);

    codec = x_object_new (type, NULL);
    x_return_val_if_fail (codec != NULL, NULL);

    if (!cube) {
        file = x_repository_open (group, filename, NULL);
        x_return_val_if_fail (file != NULL, NULL);

        outdat = x_codec_decode (X_CODEC (codec), filename, file, &outlen);
        x_goto_clean_if_fail (outdat != NULL);

        image = x_slice_new (xImage);
        image->ref_count = 1;
        image->faces = 1;
        image->data[0] = outdat;
        image->bytes = outlen;
        image->width = codec->width;
        image->height = codec->height;
        image->depth = codec->depth;
        image->mipmaps = codec->mipmaps;
        image->format = codec->format;
    }
    else {
        xsize i;
        xstr p;
        xcstr suffixes[X_BOX_FACES] = {"_RT","_LF",  "_UP", "_DN", "_FR", "_BK"};

        name = x_malloc (4 + strlen (filename));
        strncpy (name, filename, dot-filename);
        p = name + (dot-filename);
        for (i=0;i<X_BOX_FACES;++i){
            strcpy ( x_stpcpy (p, suffixes[i]), dot);
            file = x_repository_open (group, name, NULL);
            x_goto_clean_if_fail (file != NULL);

            outdat = x_codec_decode (X_CODEC (codec), name, file, &outlen);
            x_goto_clean_if_fail (outdat != NULL);
            x_object_unref (file);
            file = NULL;

            if (!image) {
                image = x_slice_new0 (xImage);
                image->ref_count = 1;
                image->data[i] = outdat;
                image->bytes = outlen;
                image->width = codec->width;
                image->height = codec->height;
                image->depth = codec->depth;
                image->mipmaps = codec->mipmaps;
                image->format = codec->format;
                image->faces = 6;
            }
            else {
                image->data[i] = outdat;
                if (image->bytes != outlen
                    ||  image->width != codec->width
                    ||  image->width != codec->height
                    ||  image->depth != codec->depth
                    ||  image->mipmaps != codec->mipmaps
                    ||  image->format != codec->format)
                    x_error ("cube image must the same image format");
            }
        }
    }
clean:
    if (file)
        x_object_unref (file);
    if (codec)
        x_object_unref (codec);
    if (name)
        x_free (name);
    return image;
}

xImage*
x_image_ref             (xImage         *image)
{
    x_return_val_if_fail (image != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&image->ref_count) > 0, NULL);
    x_atomic_int_inc (&image->ref_count);
    return image;
}

xint
x_image_unref           (xImage         *image)
{
    xsize i;
    xint    ref_count;
    x_return_val_if_fail (image != NULL, -1);
    x_return_val_if_fail (x_atomic_int_get (&image->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&image->ref_count);
    if X_LIKELY (ref_count !=0)
        return ref_count;
    for (i=0;i<image->faces;++i)
        x_free (image->data[i]);
    x_slice_free (xImage, image);
    return ref_count;
}
xPixelBox
x_image_pixel_box       (xImage         *image,
                         xsize          face,
                         xsize          mipmap)
{
    xsize mip;
    xsize faceoffset;
    xsize width,height, depth;
    xPixelBox pbox={0,};
    x_return_val_if_fail (image != NULL, pbox);
    x_return_val_if_fail (face < image->faces, pbox);
    x_return_val_if_fail (mipmap <= image->mipmaps, pbox);

    faceoffset = 0;
    width = image->width;
    height = image->height;
    depth = image->depth;
    for (mip=0; mip<=image->mipmaps; ++mip) {
        if (mip == mipmap) {
            pbox.right = width;
            pbox.bottom = height;
            pbox.back = depth;
            pbox.row_pitch = width;
            pbox.slice_pitch = width * height;
            pbox.format = image->format;
            pbox.data = image->data[face]+faceoffset;
            break;
        }
        faceoffset += x_pixel_box_bytes (width, height, depth, image->format);
        if (width !=1 ) width /= 2;
        if (height != 1) height /= 2;
        if (depth != 1) depth /= 2;
    }
    return pbox;
}
