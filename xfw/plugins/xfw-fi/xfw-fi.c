#define X_LOG_DOMAIN                "xfw-freeimage"
#include <xfw.h>
#include <xos.h>
#include "FreeImage.h"
typedef struct {
    xImageCodec     parent;
} FreeImage;
typedef struct {
    xImageCodecClass      parent;
} FreeImageClass;
static void free_image_set_mime (xType type);
X_DEFINE_DYNAMIC_TYPE_EXTENDED (FreeImage,free_image,X_TYPE_IMAGE_CODEC,0,
                                free_image_set_mime (x_type));
xbool X_MODULE_EXPORT
type_module_main        (xTypeModule    *module)
{
    free_image_register (module);
    return TRUE;
}
static unsigned DLL_CALLCONV
free_image_read         (void           *buffer,
                         unsigned       size,
                         unsigned       count,
                         fi_handle      handle)
{
    return x_file_get (handle, buffer, size, count);
}
static unsigned DLL_CALLCONV
free_image_write        (void           *buffer,
                         unsigned       size,
                         unsigned       count,
                         fi_handle      handle)
{
    return x_file_put (handle, buffer, size, count);
}
static int DLL_CALLCONV
free_image_seek         (fi_handle      handle,
                         long           offset,
                         int            origin)
{
    return x_file_seek (handle, offset, origin) ? 0 : 1;
}
static long DLL_CALLCONV
free_image_tell         (fi_handle      handle)
{
    return x_file_tell (handle);
}
static xbyte*
free_image_decode       (xCodec         *codec,
                         xcstr          filename,
                         xFile          *file,
                         xsize          *outlen)
{
    FREE_IMAGE_FORMAT fi_format;
    xsize y;
    unsigned char* srcData;
    unsigned srcPitch;
    xsize dstPitch;
    xbyte *pSrc, *pDst, *ret;
    FIBITMAP *bitmap;
    FREE_IMAGE_TYPE image_type;
    FREE_IMAGE_COLOR_TYPE color_type;
    unsigned bpp;
    xImageCodec *icodec = (xImageCodec*)codec;
    static FreeImageIO io = {
        free_image_read,
        free_image_write,
        free_image_seek,
        free_image_tell
    };

    fi_format = FreeImage_GetFIFFromFilename(filename);
    bitmap = FreeImage_LoadFromHandle(fi_format, &io, (fi_handle)file, 0);
    
    x_return_val_if_fail (bitmap != NULL, NULL);

    icodec->depth = 1;
    icodec->width = FreeImage_GetWidth(bitmap);
    icodec->height = FreeImage_GetHeight(bitmap);
    icodec->mipmaps = 0;

    image_type = FreeImage_GetImageType (bitmap);
    color_type = FreeImage_GetColorType (bitmap);
    bpp = FreeImage_GetBPP(bitmap);

    switch(image_type) {
    case FIT_UNKNOWN:
    case FIT_COMPLEX:
    case FIT_UINT32:
    case FIT_INT32:
    case FIT_DOUBLE:
    default:
        x_warning ("FreeImage: unknown or unsupported image format");
        break;
    case FIT_BITMAP:
        if (color_type == FIC_MINISWHITE || color_type == FIC_MINISBLACK) {
            FIBITMAP* newBitmap = FreeImage_ConvertToGreyscale(bitmap);
            FreeImage_Unload(bitmap);
            bitmap = newBitmap;
            bpp = FreeImage_GetBPP(bitmap);
            color_type = FreeImage_GetColorType(bitmap);
        }
        else if (bpp < 8 || color_type == FIC_PALETTE || color_type == FIC_CMYK) {
            FIBITMAP* newBitmap = FreeImage_ConvertTo24Bits(bitmap);
            FreeImage_Unload(bitmap);
            bitmap = newBitmap;
            bpp = FreeImage_GetBPP(bitmap);
            color_type = FreeImage_GetColorType(bitmap);
        }
        switch(bpp) {
        case 8:
            icodec->format = X_PIXEL_L8;
            break;
        case 16:
            if(FreeImage_GetGreenMask(bitmap) == FI16_565_GREEN_MASK)
                icodec->format = X_PIXEL_R5G6B5;
            else
                /* FreeImage doesn't support 4444 format so must be 1555 */
                icodec->format = X_PIXEL_A1R5G5B5;
            break;
        case 24:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
            icodec->format = X_PIXEL_BYTE_RGB;
#else
            icodec->format = X_PIXEL_BYTE_BGR;
#endif
            break;
        case 32:
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
            icodec->format = X_PIXEL_BYTE_RGBA;
#else
            icodec->format = X_PIXEL_BYTE_BGRA;
#endif
            break;

        };
        break;
    case FIT_UINT16:
    case FIT_INT16:
        icodec->format = X_PIXEL_L16;
        break;
    case FIT_FLOAT:
        icodec->format = X_PIXEL_FLOAT32_R;
        break;
    case FIT_RGB16:
        icodec->format = X_PIXEL_SHORT_RGB;
        break;
    case FIT_RGBA16:
        icodec->format = X_PIXEL_SHORT_RGBA;
        break;
    case FIT_RGBF:
        icodec->format = X_PIXEL_FLOAT32_RGB;
        break;
    case FIT_RGBAF:
        icodec->format = X_PIXEL_FLOAT32_RGBA;
        break;
    };



    srcData = FreeImage_GetBits(bitmap);
    srcPitch = FreeImage_GetPitch(bitmap);

    // Final data - invert image and trim pitch at the same time
    dstPitch = x_pixel_format_bytes (icodec->format, icodec->width);
    *outlen = dstPitch * icodec->height;
    ret = x_malloc (*outlen);
    pDst = ret;

    for ( y = 0; y < icodec->height; ++y)
    {
        pSrc = srcData + (icodec->height - y - 1) * srcPitch;
        memcpy(pDst, pSrc, dstPitch);
        pDst += dstPitch;
    }

    FreeImage_Unload(bitmap);

    return ret;
}
static void
free_image_init         (FreeImage        *file)
{
}
static void
free_image_log          (FREE_IMAGE_FORMAT fif,
                         const char *message) 
{
    xcstr typeName = FreeImage_GetFormatFromFIF(fif);
    x_error ("FreeImage load format %s error: %s",typeName, message);
}
static void
free_image_class_init   (FreeImageClass *klass)
{
    xCodecClass *dclass;
    dclass = X_CODEC_CLASS (klass);
    dclass->decode = free_image_decode;
} 
static void
free_image_class_finalize (FreeImageClass *klass)
{
}
static void
free_image_set_mime     (xType xtype)
{
    xint i;
    xString *suffix = x_string_new ("", 1024);
    FreeImage_Initialise(FALSE);
    FreeImage_SetOutputMessage (free_image_log);
    x_debug ("\nFreeImage %s\n%s", FreeImage_GetVersion(), FreeImage_GetCopyrightMessage());

    for (i = 0; i < FreeImage_GetFIFCount(); ++i) {
        if ((FREE_IMAGE_FORMAT)i == FIF_DDS) continue;
        if (suffix->len >0)
            x_string_append_char (suffix, ',');
        x_string_append (suffix, 
                         FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i));
    }
    x_type_set_mime (xtype, x_string_delete (suffix, FALSE));
}
