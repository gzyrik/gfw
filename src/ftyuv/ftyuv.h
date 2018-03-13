/** draw freetype font to yuv (I420) buffer */
#ifndef __FTYUV_H__
#define __FTYUV_H__
#ifdef __cplusplus
extern "C" {
#endif

enum {
    FTYUV_LEFT          = 1<<0,     /* layout default */
    FTYUV_RIGHT         = 1<<1,
    FTYUV_TOP           = 1<<2,     /* layout default */
    FTYUV_BOTTOM        = 1<<3,
    FTYUV_HCENTER       = FTYUV_LEFT | FTYUV_RIGHT,
    FTYUV_VCENTER       = FTYUV_TOP | FTYUV_BOTTOM,
    FTYUV_CENTER        = FTYUV_HCENTER | FTYUV_VCENTER,

    FTYUV_LINE_LEFT     = 1<<4,     /* line default */
    FTYUV_LINE_RIGHT    = 1<<5,
    FTYUV_LINE_TOP      = 1<<6,     /* line default */
    FTYUV_LINE_BOTTOM   = 1<<7,
    FTYUV_LINE_HCENTER  = FTYUV_LINE_LEFT|FTYUV_LINE_RIGHT,
    FTYUV_LINE_VCENTER  = FTYUV_LINE_TOP|FTYUV_LINE_BOTTOM,

    /* default is HORIZONTAL */
    FTYUV_VERTICAL      = 1<<8,     

    /* default is y-plane xor */
    FTYUV_BLEND         = 1<<9,
    FTYUV_STROKE        = 1<<10,

    /* defualt is plane */
    FTYUV_PACKED        = 1<<11,

    /* default is yuv444 */
    FTYUV_420           = 1<<12,
    FTYUV_422           = 1<<13,
    FTYUV_411           = 1<<14,
};
struct ftyuv;

struct ftyuv* ftyuv_new     (const char        *fontfile);

void        ftyuv_resize    (struct ftyuv       *ftyuv,
                             unsigned           pixel);

void        ftyuv_unref     (struct ftyuv       *ftyuv);

void        ftyuv_space     (struct ftyuv       *ftyuv,
                             const int          hspace,
                             const int          vspace);

void        ftyuv_margin    (struct ftyuv       *ftyuv,
                             const int          hmargin,
                             const int          vmargin);

void        ftyuv_color     (struct ftyuv       *ftyuv,
                             const unsigned     font_xrgb,
                             const unsigned     border_xrgb);

void        ftyuv_draw      (struct ftyuv       *ftyuv,
                             const unsigned short *str,
                             const unsigned     len,
                             unsigned char      *yuv,
                             const unsigned     width,
                             const unsigned     height,
                             const unsigned     flags);

#ifdef __cplusplus
}
#endif
#endif /* __FTYUV_H__ */
