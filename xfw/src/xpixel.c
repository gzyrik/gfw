#include "config.h"
#include "xpixel.h"
#include <string.h>
struct PixelSpec
{
    /* Name of the format, as in the enum */
    xcstr name;
    /* Number of bytes one element (colour value) takes. */
    xuint8 bytes;
    /* Pixel format flags, see enum PixelFormatFlags for the bit field definitions */
    xuint flags;
    /** Component type */
    xPixelType type;
    /** Component count */
    xuint8 count;
    /* Number of bits for red(or luminance), green, blue, alpha */
    xuint8 rbits,gbits,bbits,abits; /*, ibits, dbits, ... */
    /* Masks and shifts as used by packers/unpackers */
    xuint32 rmask, gmask, bmask, amask;
    xuint8 rshift, gshift, bshift, ashift;
};
static struct PixelSpec _pixel_spec[X_PIXEL_COUNT] = {
    {"UNKNOWN",
        /* Bytes per element */
        0,
        /* Flags */
        0,
        /* Component type and count */
        X_PIXEL_BYTE, 0,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"L8",
        /* Bytes per element */
        1,
        /* Flags */
        X_PIXEL_LUMINANCE | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 1,
        /* rbits, gbits, bbits, abits */
        8, 0, 0, 0,
        /* Masks and shifts */
        0xFF, 0, 0, 0, 0, 0, 0, 0
    },
    {"L16",
        /* Bytes per element */
        2,
        /* Flags */
        X_PIXEL_LUMINANCE | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_SHORT, 1,
        /* rbits, gbits, bbits, abits */
        16, 0, 0, 0,
        /* Masks and shifts */
        0xFFFF, 0, 0, 0, 0, 0, 0, 0
    },
    {"A8",
        /* Bytes per element */
        1,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 1,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 8,
        /* Masks and shifts */
        0, 0, 0, 0xFF, 0, 0, 0, 0
    },
    {"A4L4",
        /* Bytes per element */
        1,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_LUMINANCE | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 2,
        /* rbits, gbits, bbits, abits */
        4, 0, 0, 4,
        /* Masks and shifts */
        0x0F, 0, 0, 0xF0, 0, 0, 0, 4
    },
    {"BYTE_LA",
        /* Bytes per element */
        2,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_LUMINANCE,
        /* Component type and count */
        X_PIXEL_BYTE, 2,
        /* rbits, gbits, bbits, abits */
        8, 0, 0, 8,
        /* Masks and shifts */
        0,0,0,0,0,0,0,0
    },
    {"R5G6B5",
        /* Bytes per element */
        2,
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        5, 6, 5, 0,
        /* Masks and shifts */
        0xF800, 0x07E0, 0x001F, 0,
        11, 5, 0, 0
    },
    {"B5G6R5",
        /* Bytes per element */
        2,
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        5, 6, 5, 0,
        /* Masks and shifts */
        0x001F, 0x07E0, 0xF800, 0,
        0, 5, 11, 0
    },
    {"A4R4G4B4",
        /* Bytes per element */
        2,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        4, 4, 4, 4,
        /* Masks and shifts */
        0x0F00, 0x00F0, 0x000F, 0xF000,
        8, 4, 0, 12
    },
    {"A1R5G5B5",
        /* Bytes per element */
        2,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        5, 5, 5, 1,
        /* Masks and shifts */
        0x7C00, 0x03E0, 0x001F, 0x8000,
        10, 5, 0, 15,
    },
    {"R8G8B8",
        /* Bytes per element */
        3,  // 24 bit integer -- special
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 0,
        /* Masks and shifts */
        0xFF0000, 0x00FF00, 0x0000FF, 0,
        16, 8, 0, 0
    },
    {"B8G8R8",
        /* Bytes per element */
        3,  // 24 bit integer -- special
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 0,
        /* Masks and shifts */
        0x0000FF, 0x00FF00, 0xFF0000, 0,
        0, 8, 16, 0
    },
    {"A8R8G8B8",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 8,
        /* Masks and shifts */
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
        16, 8, 0, 24
    },
    {"A8B8G8R8",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 8,
        /* Masks and shifts */
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
        0, 8, 16, 24,
    },
    {"B8G8R8A8",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 8,
        /* Masks and shifts */
        0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF,
        8, 16, 24, 0
    },
    {"A2R10G10B10",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        10, 10, 10, 2,
        /* Masks and shifts */
        0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000,
        20, 10, 0, 30
    },
    {"A2B10G10R10",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        10, 10, 10, 2,
        /* Masks and shifts */
        0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000,
        0, 10, 20, 30
    },
    {"DXT1",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_DXT_COMPRESSED | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_BYTE, 3, // No alpha
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"DXT2",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_DXT_COMPRESSED | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"DXT3",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_DXT_COMPRESSED | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"DXT4",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_DXT_COMPRESSED | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"DXT5",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_DXT_COMPRESSED | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"FLOAT16_RGB",
        /* Bytes per element */
        6,
        /* Flags */
        X_PIXEL_FLOAT,
        /* Component type and count */
        X_PIXEL_FLOAT16, 3,
        /* rbits, gbits, bbits, abits */
        16, 16, 16, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"FLOAT16_RGBA",
        /* Bytes per element */
        8,
        /* Flags */
        X_PIXEL_FLOAT | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_FLOAT16, 4,
        /* rbits, gbits, bbits, abits */
        16, 16, 16, 16,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"FLOAT32_RGB",
        /* Bytes per element */
        12,
        /* Flags */
        X_PIXEL_FLOAT,
        /* Component type and count */
        X_PIXEL_FLOAT32, 3,
        /* rbits, gbits, bbits, abits */
        32, 32, 32, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"FLOAT32_RGBA",
        /* Bytes per element */
        16,
        /* Flags */
        X_PIXEL_FLOAT | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_FLOAT32, 4,
        /* rbits, gbits, bbits, abits */
        32, 32, 32, 32,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"X8R8G8B8",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 8,
        /* Masks and shifts */
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
        16, 8, 0, 24
    },
    {"X8B8G8R8",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 8,
        /* Masks and shifts */
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
        0, 8, 16, 24
    },
    {"R8G8B8A8",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_ALPHA | X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        8, 8, 8, 8,
        /* Masks and shifts */
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF,
        24, 16, 8, 0
    },
    {"SHORT_RGBA",
        /* Bytes per element */
        8,
        /* Flags */
        X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_SHORT, 4,
        /* rbits, gbits, bbits, abits */
        16, 16, 16, 16,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"R3G3B2",
        /* Bytes per element */
        1,
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        3, 3, 2, 0,
        /* Masks and shifts */
        0xE0, 0x1C, 0x03, 0,
        5, 2, 0, 0
    },
    {"FLOAT16_R",
        /* Bytes per element */
        2,
        /* Flags */
        X_PIXEL_FLOAT,
        /* Component type and count */
        X_PIXEL_FLOAT16, 1,
        /* rbits, gbits, bbits, abits */
        16, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"FLOAT32_R",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_FLOAT,
        /* Component type and count */
        X_PIXEL_FLOAT32, 1,
        /* rbits, gbits, bbits, abits */
        32, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"SHORT_GR",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_NATIVE_ENDIAN,
        /* Component type and count */
        X_PIXEL_SHORT, 2,
        /* rbits, gbits, bbits, abits */
        16, 16, 0, 0,
        /* Masks and shifts */
        0x0000FFFF, 0xFFFF0000, 0, 0, 
        0, 16, 0, 0
    },
    {"FLOAT16_GR",
        /* Bytes per element */
        4,
        /* Flags */
        X_PIXEL_FLOAT,
        /* Component type and count */
        X_PIXEL_FLOAT16, 2,
        /* rbits, gbits, bbits, abits */
        16, 16, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"FLOAT32_GR",
        /* Bytes per element */
        8,
        /* Flags */
        X_PIXEL_FLOAT,
        /* Component type and count */
        X_PIXEL_FLOAT32, 2,
        /* rbits, gbits, bbits, abits */
        32, 32, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"SHORT_RGB",
        /* Bytes per element */
        6,
        /* Flags */
        0,
        /* Component type and count */
        X_PIXEL_SHORT, 3,
        /* rbits, gbits, bbits, abits */
        16, 16, 16, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"PVRTC_RGB2",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_PVR_COMPRESSED,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"PVRTC_RGBA2",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_PVR_COMPRESSED | X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"PVRTC_RGB4",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_PVR_COMPRESSED,
        /* Component type and count */
        X_PIXEL_BYTE, 3,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
    {"PVRTC_RGBA4",
        /* Bytes per element */
        0,
        /* Flags */
        X_PIXEL_PVR_COMPRESSED| X_PIXEL_ALPHA,
        /* Component type and count */
        X_PIXEL_BYTE, 4,
        /* rbits, gbits, bbits, abits */
        0, 0, 0, 0,
        /* Masks and shifts */
        0, 0, 0, 0, 0, 0, 0, 0
    },
};
static struct PixelSpec* pixel_spec (xint index)
{
    if (index >= X_PIXEL_COUNT || index < 0)
        x_error ("invalid pixel format %d", index);
    return &_pixel_spec[index];
}

xsize
x_pixel_format_bytes    (xPixelFormat   format,
                         xsize          row_pitch)
{
    struct PixelSpec* spec = pixel_spec (format);
    switch (format) {
    case X_PIXEL_DXT1:
        /* 64 bits (8 bytes) per 4x4 block */
        return (row_pitch / 4) * 8;
    case X_PIXEL_DXT2:
    case X_PIXEL_DXT3:
    case X_PIXEL_DXT4:
    case X_PIXEL_DXT5:
        /* 128 bits (16 bytes) per 4x4 block */
        return (row_pitch / 4) * 16;
    case X_PIXEL_PVRTC_RGB2:
    case X_PIXEL_PVRTC_RGBA2:
    case X_PIXEL_PVRTC_RGB4:
    case X_PIXEL_PVRTC_RGBA4:
        //TODO
        return 0;
    default:
        return spec->bytes * row_pitch;
    }
}

xPixelType
x_pixel_format_type     (xPixelFormat   format)
{
    return pixel_spec (format)->type;
}

xuint
x_pixel_format_flags    (xPixelFormat   format)
{
    return pixel_spec (format)->flags;
}

xcstr
x_pixel_format_name     (xPixelFormat   format)
{
    return pixel_spec (format)->name;
}

xPixelFormat
x_pixel_format_from_name (xcstr         name)
{
    xsize i;
    for (i=0; i< X_N_ELEMENTS(_pixel_spec); ++i) {
        if (x_stristr (_pixel_spec [i].name, name))
            return (xPixelFormat)i;
    }
    return X_PIXEL_UNKNOWN;
}
static inline xuint
float_to_fixed          (xfloat         value,
                         xuint          bits)
{
    if(value <= 0.0f)
        return 0;
    else if (value >= 1.0f)
        return (1<<bits)-1;
    else
        return (xuint)(value * (1<<bits));  
}
static inline xfloat
fixed_to_float          (xuint          value,
                         xuint          bits)
{
    return (xfloat)value/(xfloat)((1<<bits)-1);
}
static inline xuint16
float_to_half_i         (xuint32        i)
{
    register xint s =  (i >> 16) & 0x00008000;
    register xint e = ((i >> 23) & 0x000000ff) - (127 - 15);
    register xint m =   i        & 0x007fffff;

    if (e <= 0) {
        if (e < -10)
            return 0;

        m = (m | 0x00800000) >> (1 - e);
        return (xuint16)(s | (m >> 13));
    }
    else if (e == 0xff - (127 - 15)) {
        if (m == 0)/* Inf */
            return (xuint16)(s | 0x7c00);
        else {   /* NAN */
            m >>= 13;
            return (xuint16)(s | 0x7c00 | m | (m == 0));
        }
    }
    else {
        if (e > 30) /* Overflow */
            return (xuint16)(s | 0x7c00);
        return (xuint16)(s | (e << 10) | (m >> 13));
    }
}
static inline xuint32
half_to_float_i         (xuint16        y)
{
    register xint s = (y >> 15) & 0x00000001;
    register xint e = (y >> 10) & 0x0000001f;
    register xint m =  y        & 0x000003ff;

    if (e == 0) {
        if (m == 0) /* Plus or minus zero */
            return s << 31;
        else {/* Denormalized number -- renormalize it */
            while (!(m & 0x00000400)) {
                m <<= 1;
                e -=  1;
            }

            e += 1;
            m &= ~0x00000400;
        }
    }
    else if (e == 31) {
        if (m == 0) /* Inf */
            return (s << 31) | 0x7f800000;
        else /* NaN */
            return (s << 31) | 0x7f800000 | (m << 13);
    }

    e = e + (127 - 15);
    m = m << 13;

    return (s << 31) | (e << 23) | m;
}
static inline xuint16
float_to_half           (xfloat         f)
{
    union { xfloat f; xuint32 i; } v;
    v.f = f;
    return float_to_half_i (v.i);
}
static inline xfloat
half_to_float           (xuint16        y)
{
    union { xfloat f; xuint32 i; } v;
    v.i = half_to_float_i (y);
    return v.f;
}
static inline void
int_write               (xptr           *dest,
                         xint           n,
                         xuint          value)
{
    switch(n) {
    case 1:
        ((xbyte*)dest)[0] = (xbyte)value;
        break;
    case 2:
        ((xuint16*)dest)[0] = (xuint16)value;
        break;
    case 3:
#if X_BYTE_ORDER == X_BIG_ENDIAN      
        ((xbyte*)dest)[0] = (xbyte)((value >> 16) & 0xFF);
        ((xbyte*)dest)[1] = (xbyte)((value >> 8) & 0xFF);
        ((xbyte*)dest)[2] = (xbyte)(value & 0xFF);
#else
        ((xbyte*)dest)[2] = (xbyte)((value >> 16) & 0xFF);
        ((xbyte*)dest)[1] = (xbyte)((value >> 8) & 0xFF);
        ((xbyte*)dest)[0] = (xbyte)(value & 0xFF);
#endif
        break;
    case 4:
        ((xuint32*)dest)[0] = (xuint32)value;                
        break;                
    }        
}
static inline xuint
int_read                (xcptr           src,
                         xint            n)
{
    switch(n) {
    case 1:
        return ((xbyte*)src)[0];
    case 2:
        return ((xuint16*)src)[0];
    case 3:
#if X_BYTE_ORDER == X_BIG_ENDIAN      
        return ((xuint32)((xbyte*)src)[0]<<16)
            | ((xuint32)((xbyte*)src)[1]<<8)
            | ((xuint32)((xbyte*)src)[2]);
#else
        return ((xuint32)((xbyte*)src)[0])
            | ((xuint32)((xbyte*)src)[1]<<8)
            | ((xuint32)((xbyte*)src)[2]<<16);
#endif
    case 4:
        return ((xuint32*)src)[0];
    } 
    x_warn_if_reached ();
    return 0;
}

void
x_color_pack            (const xcolor   *color,
                         xPixelFormat   format,
                         xptr           dest)
{
    struct PixelSpec* spec = pixel_spec (format);
    if (spec->flags & X_PIXEL_NATIVE_ENDIAN) {
        xuint value =
            ((float_to_fixed(color->r, spec->rbits)<<spec->rshift) & spec->rmask) |
            ((float_to_fixed(color->g, spec->gbits)<<spec->gshift) & spec->gmask) |
            ((float_to_fixed(color->b, spec->bbits)<<spec->bshift) & spec->bmask) |
            ((float_to_fixed(color->a, spec->abits)<<spec->ashift) & spec->amask);
        int_write (dest, spec->bytes, value);
    }
    else {
        switch (format) {
        case X_PIXEL_FLOAT32_R:
            ((xfloat*)dest)[0] = color->r;
            break;
        case X_PIXEL_FLOAT32_GR:
            ((xfloat*)dest)[0] = color->g;
            ((xfloat*)dest)[1] = color->r;
            break;
        case X_PIXEL_FLOAT32_RGB:
            ((xfloat*)dest)[0] = color->r;
            ((xfloat*)dest)[1] = color->g;
            ((xfloat*)dest)[2] = color->b;
            break;
        case X_PIXEL_FLOAT32_RGBA:
            ((xfloat*)dest)[0] = color->r;
            ((xfloat*)dest)[1] = color->g;
            ((xfloat*)dest)[2] = color->b;
            ((xfloat*)dest)[3] = color->a;
            break;
        case X_PIXEL_FLOAT16_R:
            ((xuint16*)dest)[0] = float_to_half (color->r);
            break;
        case X_PIXEL_FLOAT16_GR:
            ((xuint16*)dest)[0] = float_to_half (color->g);
            ((xuint16*)dest)[1] = float_to_half (color->r);
            break;
        case X_PIXEL_FLOAT16_RGB:
            ((xuint16*)dest)[0] = float_to_half (color->r);
            ((xuint16*)dest)[1] = float_to_half (color->g);
            ((xuint16*)dest)[2] = float_to_half (color->b);
            break;
        case X_PIXEL_FLOAT16_RGBA:
            ((xuint16*)dest)[0] = float_to_half (color->r);
            ((xuint16*)dest)[1] = float_to_half (color->g);
            ((xuint16*)dest)[2] = float_to_half (color->b);
            ((xuint16*)dest)[3] = float_to_half (color->a);
            break;
        case X_PIXEL_SHORT_RGB:
            ((xuint16*)dest)[0] = float_to_fixed (color->r, 16);
            ((xuint16*)dest)[1] = float_to_fixed (color->g, 16);
            ((xuint16*)dest)[2] = float_to_fixed (color->b, 16);
            break;
        case X_PIXEL_SHORT_RGBA:
            ((xuint16*)dest)[0] = float_to_fixed (color->r, 16);
            ((xuint16*)dest)[1] = float_to_fixed (color->g, 16);
            ((xuint16*)dest)[2] = float_to_fixed (color->b, 16);
            ((xuint16*)dest)[3] = float_to_fixed (color->a, 16);
            break;
        case X_PIXEL_BYTE_LA:
            ((xbyte*)dest)[0] = float_to_fixed (color->r, 8);
            ((xbyte*)dest)[1] = float_to_fixed (color->a, 8);
            break;
        default:
            x_error ("pack to '%s' not implemented",
                     x_pixel_format_name (format));
        }
    }
}

void
x_color_unpack          (xcolor         *color,
                         xPixelFormat   format,
                         xcptr          src)
{
    struct PixelSpec* spec = pixel_spec (format);
    if (spec->flags & X_PIXEL_NATIVE_ENDIAN) {
        xuint value = int_read (src, spec->bytes);
        if(spec->flags & X_PIXEL_LUMINANCE) {
            /* Luminance format -- only rbits used */
            color->r = color->g = color->b = fixed_to_float ((value & spec->rmask)>>spec->rshift, spec->rbits);
        }
        else {
            color->r = fixed_to_float ((value & spec->rmask)>>spec->rshift, spec->rbits);
            color->g = fixed_to_float ((value & spec->gmask)>>spec->gshift, spec->gbits);
            color->b = fixed_to_float ((value & spec->bmask)>>spec->bshift, spec->bbits);
        }
        if(spec->flags & X_PIXEL_ALPHA) {
            color->a = fixed_to_float ((value & spec->amask)>>spec->ashift, spec->abits);
        }
        else {
            color->a = 1.0f; /* No alpha, default a component to full */
        }
    } else {
        switch(format) {
        case X_PIXEL_FLOAT32_R:
            color->r = color->g = color->b = ((xfloat*)src)[0];
            color->a = 1.0f;
            break;
        case X_PIXEL_FLOAT32_GR:
            color->g = ((xfloat*)src)[0];
            color->r = color->b = ((xfloat*)src)[1];
            color->a = 1.0f;
            break;
        case X_PIXEL_FLOAT32_RGB:
            color->r = ((xfloat*)src)[0];
            color->g = ((xfloat*)src)[1];
            color->b = ((xfloat*)src)[2];
            color->a = 1.0f;
            break;
        case X_PIXEL_FLOAT32_RGBA:
            color->r = ((xfloat*)src)[0];
            color->g = ((xfloat*)src)[1];
            color->b = ((xfloat*)src)[2];
            color->a = ((xfloat*)src)[3];
            break;
        case X_PIXEL_FLOAT16_R:
            color->r = color->g = color->b = half_to_float(((xuint16*)src)[0]);
            color->a = 1.0f;
            break;
        case X_PIXEL_FLOAT16_GR:
            color->g = half_to_float (((xuint16*)src)[0]);
            color->r = color->b = half_to_float (((xuint16*)src)[1]);
            color->a = 1.0f;
            break;
        case X_PIXEL_FLOAT16_RGB:
            color->r = half_to_float (((xuint16*)src)[0]);
            color->g = half_to_float (((xuint16*)src)[1]);
            color->b = half_to_float (((xuint16*)src)[2]);
            color->a = 1.0f;
            break;
        case X_PIXEL_FLOAT16_RGBA:
            color->r = half_to_float (((xuint16*)src)[0]);
            color->g = half_to_float (((xuint16*)src)[1]);
            color->b = half_to_float (((xuint16*)src)[2]);
            color->a = half_to_float (((xuint16*)src)[3]);
            break;
        case X_PIXEL_SHORT_RGB:
            color->r = fixed_to_float (((xuint16*)src)[0], 16);
            color->g = fixed_to_float (((xuint16*)src)[1], 16);
            color->b = fixed_to_float (((xuint16*)src)[2], 16);
            color->a = 1.0f;
            break;
        case X_PIXEL_SHORT_RGBA:
            color->r = fixed_to_float (((xuint16*)src)[0], 16);
            color->g = fixed_to_float (((xuint16*)src)[1], 16);
            color->b = fixed_to_float (((xuint16*)src)[2], 16);
            color->a = fixed_to_float (((xuint16*)src)[3], 16);
            break;
        case X_PIXEL_BYTE_LA:
            color->r = color->g = color->b = fixed_to_float (((xbyte*)src)[0], 8);
            color->a = fixed_to_float (((xbyte*)src)[1], 8);
            break;
        default:
            x_error ("unpack from '%s' not implemented",
                     x_pixel_format_name (format));
        }
    }
}
xsize
x_pixel_box_bytes       (xsize          width,
                         xsize          height,
                         xsize          depth,
                         xPixelFormat   format)
{
    struct PixelSpec* spec = pixel_spec (format);
    if (spec->flags & X_PIXEL_COMPRESSED) {
        switch(format) {
            /* DXT formats work by dividing the image into 4x4 blocks, then encoding each
               4x4 block with a certain number of bytes. DXT can only be used on 2D images.*/
        case X_PIXEL_DXT1:
            x_assert(depth == 1&&"DXT can only be used on 2D images");
            return ((width+3)/4)*((height+3)/4)*8;
        case X_PIXEL_DXT2:
        case X_PIXEL_DXT3:
        case X_PIXEL_DXT4:
        case X_PIXEL_DXT5:
            x_assert(depth == 1&&"DXT can only be used on 2D images");
            return ((width+3)/4)*((height+3)/4)*16;

            /* Size calculations from the PVRTC OpenGL extension spec
             * http://www.khronos.org/registry/gles/extensions/IMG/IMG_texture_compression_pvrtc.txt
             * Basically, 32 bytes is the minimum texture size.  Smaller textures are padded up to 32 bytes
             */
        case X_PIXEL_PVRTC_RGB2:
        case X_PIXEL_PVRTC_RGBA2:
            x_assert(depth == 1&&"PVRTC can only be used on 2D images");
            return (MAX(width, 16) * MAX((int)height, 8) * 2 + 7) / 8;
        case X_PIXEL_PVRTC_RGB4:
        case X_PIXEL_PVRTC_RGBA4:
            x_assert(depth == 1&&"PVRTC can only be used on 2D images");
            return (MAX((int)width, 8) * MAX((int)height, 8) * 4 + 7) / 8;
        default:
            x_error ("Invalid compressed pixel format");
        }
    }
    return width*height*depth*spec->bytes;
}
void
x_pixel_box_init        (xPixelBox      *box,
                         xsize          width,
                         xsize          height,
                         xsize          depth,
                         xPixelFormat   format,
                         xbyte          *data)
{
    if (!data) {
        xsize bytes = x_pixel_box_bytes (width, height, depth, format);
        data = x_malloc (bytes);
    }
    box->left = 0;
    box->right = width;
    box->top = 0;
    box->bottom = height;
    box->front = 0;
    box->back = depth;
    box->format = format;
    box->data = data;
    box->row_pitch = width;
    box->slice_pitch = width * height;
}

static inline xbool
pixelbox_is_consecutive (const xPixelBox      *box)
{
    xsize width = box->right - box->left;
    xsize height = box->bottom - box->top;
    return (box->row_pitch == width && box->slice_pitch == width*height);
}

static void
nearest_resampler       (const xPixelBox    *src,
                         const xPixelBox    *dst,
                         const int          elemsize)
{
    /* srcdata stays at beginning, pdst is a moving pointer */
    xbyte* srcdata = (xbyte*)src->data;
    xbyte* pdst = (xbyte*)dst->data;

    /* sx_48,sy_48,sz_48 represent current position in source
     * using 16/48-bit fixed precision, incremented by steps
     */
    xuint64 stepx = ((xuint64)(src->right-src->left) << 48) / (dst->right-dst->left);
    xuint64 stepy = ((xuint64)(src->bottom-src->top) << 48) / (dst->bottom-dst->top);
    xuint64 stepz = ((xuint64)(src->back-src->front) << 48) / (dst->back-dst->front);

    /* note: ((stepz>>1) - 1) is an extra half-step increment to adjust
     * for the center of the destination pixel, not the top-left corner
     */
    xuint64 sz_48 = (stepz >> 1) - 1;
    const int dst_row_skip = dst->row_pitch - (dst->right-dst->left);
    const int dst_slice_skip = dst->slice_pitch - (dst->back-dst->front) * dst->row_pitch;
    xsize x,y, z;
    for (z= dst->front; z < dst->back; z++, sz_48 += stepz) {
        xsize srczoff = (xsize)(sz_48 >> 48) * src->slice_pitch;

        xuint64 sy_48 = (stepy >> 1) - 1;
        for (y = dst->top; y < dst->bottom; y++, sy_48 += stepy) {
            xsize srcyoff = (xsize)(sy_48 >> 48) * src->row_pitch;

            xuint64 sx_48 = (stepx >> 1) - 1;
            for (x = dst->left; x < dst->right; x++, sx_48 += stepx) {
                xbyte* psrc = srcdata +
                    elemsize*((xsize)(sx_48 >> 48) + srcyoff + srczoff);
                memcpy(pdst, psrc, elemsize);
                pdst += elemsize;
            }
            pdst += elemsize * dst_row_skip;
        }
        pdst += elemsize*dst_slice_skip;
    }
}
void
x_pixel_box_convert     (const xPixelBox*src,
                         xPixelBox      *dst,
                         xScaleFilter   filter)
{
    xPixelBox tmp;
    xbool boxEqual = (src->right - src->left == dst->right - dst->left
                      && src->bottom - src->top == dst->bottom - dst->top
                      && src->back - src->front == dst->back - dst->front);
    struct PixelSpec* src_spec = pixel_spec (src->format);
    struct PixelSpec* dst_spec = pixel_spec (dst->format);
    xsize mem_len = x_pixel_box_bytes (src->right - src->left,
                                       src->bottom - src->top,
                                       src->back - src->front,
                                       src->format);
    if (((src_spec->flags | dst_spec->flags)&X_PIXEL_UNACCESSIBLE) != 0) {
        /*针对不可访问的格式,只支持直接复制 */
        if(boxEqual)
            memcpy(dst->data, src->data, mem_len);
        else
            x_error ("can not convert compressed images");
        return;
    }
    if (boxEqual) {
        if(src->format == dst->format && pixelbox_is_consecutive(src) && pixelbox_is_consecutive(dst)) {
            /*完全相同,直接快速复制 */
            memcpy(dst->data, src->data, mem_len);
            return;
        }
        /* No intermediate buffer needed */
        tmp = *src;
    }
    else {
        if (src->format == dst->format) {
            /* No intermediate buffer needed */
            tmp = *dst;
        }
        else {
            /* Allocate temporary buffer of destination size in source format  */
            x_pixel_box_init (&tmp,
                              dst->right-dst->left,
                              dst->bottom-dst->top,
                              dst->back-dst->front,
                              src->format,
                              NULL);
        }
        nearest_resampler (src, &tmp, src_spec->bytes);
    }
    if(tmp.data != dst->data) {
        const size_t srcPixelSize = pixel_spec (tmp.format)->bytes;
        const size_t dstPixelSize = dst_spec->bytes;
        const size_t srcRowSkipBytes = (tmp.row_pitch-(tmp.right-tmp.left))  * srcPixelSize;
        const size_t srcSliceSkipBytes = (tmp.slice_pitch-(tmp.bottom-tmp.top)*tmp.row_pitch)* srcPixelSize;
        const size_t dstRowSkipBytes = (dst->row_pitch -(dst->right-dst->left))  * dstPixelSize;
        const size_t dstSliceSkipBytes = (dst->slice_pitch * (dst->bottom-dst->top)*dst->row_pitch) *dstPixelSize;
        xbyte *srcptr = tmp.data
            + (tmp.left + tmp.top * tmp.row_pitch + tmp.front * tmp.slice_pitch)
            * srcPixelSize;
        xbyte *dstptr = dst->data
            + (dst->left + dst->top * dst->row_pitch + dst->front * dst->slice_pitch)
            * dstPixelSize;

        size_t z,y,x;
        xcolor color;

        xPixelFormat srcFormat = tmp.format;
        xPixelFormat dstFormat = dst->format;

        if (dstFormat == X_PIXEL_X8R8G8B8)
            dstFormat = X_PIXEL_A8R8G8B8;
        else if (dstFormat == X_PIXEL_X8B8G8R8)
            dstFormat = X_PIXEL_A8B8G8R8;

        if (!(dst_spec->flags & X_PIXEL_ALPHA)) {
            if (srcFormat == X_PIXEL_X8R8G8B8)
                srcFormat = X_PIXEL_A8R8G8B8;
            else if (srcFormat == X_PIXEL_X8B8G8R8)
                srcFormat = X_PIXEL_A8B8G8R8;
        }

        for (z=tmp.front; z<tmp.back; z++) {
            for (y=tmp.top; y<tmp.bottom; y++) {
                for (x=tmp.left; x<tmp.right; x++) {
                    x_color_unpack(&color, tmp.format, srcptr);
                    x_color_pack(&color, dst->format, dstptr);
                    srcptr += srcPixelSize;
                    dstptr += dstPixelSize;
                }
                srcptr += srcRowSkipBytes;
                dstptr += dstRowSkipBytes;
            }
            srcptr += srcSliceSkipBytes;
            dstptr += dstSliceSkipBytes;
        }
        if (tmp.data != src->data)
            x_free (tmp.data);
    }
}
void
x_box_merge             (xbox           *box1,
                         const xbox     *box2)
{
    if (box1->left == box1->right || 
        box1->top == box1->bottom ||
        box1->front == box1->back) {
        *box1 = *box2;
    }
    else {
        box1->left = MIN (box1->left, box2->left);
        box1->top = MIN (box1->top, box2->top);
        box1->front = MIN (box1->front, box2->front);
        box1->right = MAX (box1->right, box2->right);
        box1->bottom = MAX (box1->bottom, box2->bottom);
        box1->back = MAX (box1->left, box2->back);
    }
}
