#ifndef __X_PIXEL_H__
#define __X_PIXEL_H__
#include "xtype.h"
X_BEGIN_DECLS
enum _xPixelFormat
{
    /* unknown pixel format.*/
    X_PIXEL_UNKNOWN = 0,
    /* 8-bit pixel format, all bits luminace.*/
    X_PIXEL_L8      = 1,
    X_PIXEL_BYTE_L  = X_PIXEL_L8,
    /* 16-bit pixel format, all bits luminace.*/
    X_PIXEL_L16     = 2,
    X_PIXEL_SHORT_L = X_PIXEL_L16,
    /* 8-bit pixel format, all bits alpha.*/
    X_PIXEL_A8      = 3,
    X_PIXEL_BYTE_A = X_PIXEL_A8,
    /* 8-bit pixel format, 4 bits alpha, 4 bits luminance.*/
    X_PIXEL_A4L4    = 4,
    /* 2 byte pixel format, 1 byte luminance, 1 byte alpha*/
    X_PIXEL_BYTE_LA = 5,
    /* 16-bit pixel format, 5 bits red, 6 bits green, 5 bits blue.*/
    X_PIXEL_R5G6B5  = 6,
    /* 16-bit pixel format, 5 bits red, 6 bits green, 5 bits blue.*/
    X_PIXEL_B5G6R5  = 7,
    /* 8-bit pixel format, 2 bits blue, 3 bits green, 3 bits red.*/
    X_PIXEL_R3G3B2  = 30,
    /* 16-bit pixel format, 4 bits for alpha, red, green and blue.*/
    X_PIXEL_A4R4G4B4 = 8,
    /* 16-bit pixel format, 5 bits for blue, green, red and 1 for alpha.*/
    X_PIXEL_A1R5G5B5 = 9,
    /* 24-bit pixel format, 8 bits for red, green and blue.*/
    X_PIXEL_R8G8B8  = 10,
    /* 24-bit pixel format, 8 bits for blue, green and red.*/
    X_PIXEL_B8G8R8      = 11,
    /* 32-bit pixel format, 8 bits for alpha, red, green and blue.*/
    X_PIXEL_A8R8G8B8    = 12,
    /* 32-bit pixel format, 8 bits for blue, green, red and alpha.*/
    X_PIXEL_A8B8G8R8    = 13,
    /* 32-bit pixel format, 8 bits for blue, green, red and alpha.*/
    X_PIXEL_B8G8R8A8    = 14,
    /* 32-bit pixel format, 8 bits for red, green, blue and alpha.*/
    X_PIXEL_R8G8B8A8    = 28,
    /* 32-bit pixel format, 8 bits for red, 8 bits for green, 8 bits for blue
       like X_PIXEL_A8R8G8B8, but alpha will get discarded */
    X_PIXEL_X8R8G8B8    = 26,
    /* 32-bit pixel format, 8 bits for blue, 8 bits for green, 8 bits for red
       like X_PIXEL_A8B8G8R8, but alpha will get discarded */
    X_PIXEL_X8B8G8R8    = 27,
#if X_BYTE_ORDER == X_BIG_ENDIAN
    /* 3 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue*/
    X_PIXEL_BYTE_RGB    = X_PIXEL_R8G8B8,
    /* 3 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red*/
    X_PIXEL_BYTE_BGR    = X_PIXEL_B8G8R8,
    /* 4 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red and one byte for alpha*/
    X_PIXEL_BYTE_BGRA   = X_PIXEL_B8G8R8A8,
    /* 4 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue, and one byte for alpha*/
    X_PIXEL_BYTE_RGBA   = X_PIXEL_R8G8B8A8,
#else
    /* 3 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue*/
    X_PIXEL_BYTE_RGB    = X_PIXEL_B8G8R8,
    /* 3 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red*/
    X_PIXEL_BYTE_BGR    = X_PIXEL_R8G8B8,
    /* 4 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red and one byte for alpha*/
    X_PIXEL_BYTE_BGRA   = X_PIXEL_A8R8G8B8,
    /* 4 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue, and one byte for alpha*/
    X_PIXEL_BYTE_RGBA   = X_PIXEL_A8B8G8R8,
#endif        
    /* 32-bit pixel format, 2 bits for alpha, 10 bits for red, green and blue.*/
    X_PIXEL_A2R10G10B10 = 15,
    /* 32-bit pixel format, 10 bits for blue, green and red, 2 bits for alpha.*/
    X_PIXEL_A2B10G10R10 = 16,
    /* DDS (DirectDraw Surface) DXT1 format*/
    X_PIXEL_DXT1        = 17,
    /* DDS (DirectDraw Surface) DXT2 format*/
    X_PIXEL_DXT2        = 18,
    /* DDS (DirectDraw Surface) DXT3 format*/
    X_PIXEL_DXT3        = 19,
    /* DDS (DirectDraw Surface) DXT4 format*/
    X_PIXEL_DXT4        = 20,
    /* DDS (DirectDraw Surface) DXT5 format*/
    X_PIXEL_DXT5        = 21,
    /* 16-bit pixel format, 16 bits (float) for red*/
    X_PIXEL_FLOAT16_R   = 31,
    /* 48-bit pixel format, 16 bits (float) for red, 16 bits (float) for green, 16 bits (float) for blue*/
    X_PIXEL_FLOAT16_RGB = 22,
    /* 64-bit pixel format, 16 bits (float) for red, 16 bits (float) for green, 16 bits (float) for blue, 16 bits (float) for alpha*/
    X_PIXEL_FLOAT16_RGBA = 23,
    /* 16-bit pixel format, 16 bits (float) for red*/
    X_PIXEL_FLOAT32_R   = 32,
    /* 96-bit pixel format, 32 bits (float) for red, 32 bits (float) for green, 32 bits (float) for blue*/
    X_PIXEL_FLOAT32_RGB = 24,
    /* 128-bit pixel format, 32 bits (float) for red, 32 bits (float) for green, 32 bits (float) for blue, 32 bits (float) for alpha*/
    X_PIXEL_FLOAT32_RGBA = 25,
    /* 32-bit, 2-channel s10e5 floating point pixel format, 16-bit green, 16-bit red*/
    X_PIXEL_FLOAT16_GR  = 34,
    /* 64-bit, 2-channel floating point pixel format, 32-bit green, 32-bit red*/
    X_PIXEL_FLOAT32_GR  = 35,
    /* 64-bit pixel format, 16 bits for red, green, blue and alpha*/
    X_PIXEL_SHORT_RGBA  = 29,
    /* 32-bit pixel format, 16-bit green, 16-bit red*/
    X_PIXEL_SHORT_GR    = 33,
    /* 48-bit pixel format, 16 bits for red, green and blue*/
    X_PIXEL_SHORT_RGB   = 36,
    /* PVRTC (PowerVR) RGB 2 bpp*/
    X_PIXEL_PVRTC_RGB2  = 37,
    /* PVRTC (PowerVR) RGBA 2 bpp*/
    X_PIXEL_PVRTC_RGBA2 = 38,
    /* PVRTC (PowerVR) RGB 4 bpp*/
    X_PIXEL_PVRTC_RGB4  = 39,
    /* PVRTC (PowerVR) RGBA 4 bpp*/
    X_PIXEL_PVRTC_RGBA4 = 40,
    /* Number of pixel formats currently defined*/
    X_PIXEL_COUNT       = 41
};

typedef enum {
    X_PIXEL_ALPHA           = 1 << 0,      
    X_PIXEL_DXT_COMPRESSED  = 1 << 1,
    X_PIXEL_FLOAT           = 1 << 2,
    X_PIXEL_DEPTH           = 1 << 3,
    X_PIXEL_NATIVE_ENDIAN   = 1 << 4,
    X_PIXEL_LUMINANCE       = 1 << 5,
    X_PIXEL_PVR_COMPRESSED  = 1 << 6,
    X_PIXEL_COMPRESSED = X_PIXEL_DXT_COMPRESSED | X_PIXEL_PVR_COMPRESSED,
    X_PIXEL_UNACCESSIBLE = X_PIXEL_COMPRESSED | X_PIXEL_DEPTH
} xPixelFlags;

typedef enum {
    X_PIXEL_BYTE,    /* Byte per component (8 bit fixed 0.0..1.0)*/
    X_PIXEL_SHORT,   /* Short per component (16 bit fixed 0.0..1.0))*/
    X_PIXEL_FLOAT16, /* 16 bit float per component*/
    X_PIXEL_FLOAT32, /* 32 bit float per component*/
} xPixelType;

struct _xPixelBox
{
    xsize   left, right;
    xsize   top, bottom;
    xsize   front, back;
    xsize   row_pitch;
    xsize   slice_pitch;
    xbyte   *data;
    xPixelFormat format;
};
enum _xScaleFilter{
    X_SCALE_NEAREST,
    X_SCALE_LINEAR,
    X_SCALE_BILINEAR,
    X_SCALE_BOX,
    X_SCALE_TRIANGLE,
    X_SCALE_BICUBIC,
};

#define X_COLOR_A8R8G8B8(color)                                         \
    ((((xuint32)((color)->a*255.f))&0xFF)<<24)                          \
|   ((((xuint32)((color)->r*255.f))&0xFF)<<16)                          \
|   ((((xuint32)((color)->g*255.f))&0xFF)<< 8)                          \
|   ((((xuint32)((color)->b*255.f))&0xFF))

#define X_COLOR_INIT(r,g,b,a)       {r,g,b,a}

xsize       x_pixel_format_bytes    (xPixelFormat   format,
                                     xsize          row_pitch);

xPixelType  x_pixel_format_type     (xPixelFormat   format);

xuint       x_pixel_format_flags    (xPixelFormat   format);

xcstr       x_pixel_format_name     (xPixelFormat   format);

xPixelFormat x_pixel_format_from_name   (xcstr      name);

void        x_color_pack            (const xcolor   *color,
                                     xPixelFormat   format,
                                     xptr           bytes);

void        x_color_unpack          (xcolor         *color,
                                     xPixelFormat   format,
                                     xcptr          bytes);

void        x_box_merge             (xbox           *box1,
                                     const xbox     *box2);

void        x_pixel_box_init        (xPixelBox      *box,
                                     xsize          width,
                                     xsize          height,
                                     xsize          depth,
                                     xPixelFormat   format,
                                     xbyte          *data);

xsize       x_pixel_box_bytes       (xsize          width,
                                     xsize          height,
                                     xsize          depth,
                                     xPixelFormat   format);

void        x_pixel_box_convert     (const xPixelBox*src,
                                     xPixelBox      *dst,
                                     xScaleFilter   filter);

X_END_DECLS
#endif /* __X_PIXEL_H__ */
