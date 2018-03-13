#include "ftyuv.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H
#include <assert.h>
#include <stdlib.h>

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) > (b) ? (a) : (b))

struct ftyuv
{
    int hspace, vspace;
    int hmargin, vmargin;
    FT_Face face;
    FT_Library lib;
    unsigned  pixel_size;
    volatile int ref_count;
    unsigned char font_y,font_u,font_v;
    unsigned char border_y, border_u, border_v;

    /* runtime buffer */
    struct line *line_buf;
    int n_line;
    unsigned char* tmp_buf;
    int tmp_len;
};
struct line
{
    const unsigned short *str;
    int len;
    int bearingX, bearingY;
    int width,height;
};
struct layout
{
    struct line *line;
    int n_line;
    int x,y;
    int width,height;
};
struct cxt
{
    struct ftyuv* ftyuv;
    unsigned flags;
    int width, height;
    unsigned char *y_plane, *u_plane, *v_plane;
    int y_pitch, u_pitch, v_pitch, uv_x_off;
    int clip_w;
    int x, y, y_d, x_d; /* clip box */
    int y_off, x_off;
    int bearingY;
    char *msk;
#ifdef _DEBUG
    unsigned char *u_min, *u_max;
    unsigned char *v_min, *v_max;
#endif
};

struct ftyuv*
ftyuv_new (const char* fontfile)
{
    struct ftyuv *ftyuv;
    FT_Face face;
    FT_Library lib;

    FT_Open_Args args = {0,};
    if (FT_Init_FreeType(&lib))
        return NULL;

    args.flags = FT_OPEN_PATHNAME;
    args.pathname = (FT_String*) fontfile;
    if (FT_Open_Face(lib, &args, 0, &face))
        return NULL;
    ftyuv = malloc (sizeof(struct ftyuv));
    memset(ftyuv, 0, sizeof(struct ftyuv));
    ftyuv->lib = lib;
    ftyuv->face = face;
    ftyuv->ref_count = 1;
    ftyuv_resize (ftyuv, 16);

    return ftyuv;
}

void
ftyuv_unref (struct ftyuv* ftyuv)
{
    if (--ftyuv->ref_count == 0) {
        if (ftyuv->face)
            FT_Done_Face(ftyuv->face);

        if (ftyuv->lib)
            FT_Done_FreeType (ftyuv->lib);
        if (ftyuv->line_buf)
            free(ftyuv->line_buf);
        if (ftyuv->tmp_buf)
            free(ftyuv->tmp_buf);
        free(ftyuv);
    }
}
void
ftyuv_resize    (struct ftyuv *ftyuv,
                 unsigned pixel_size)
{
    if (ftyuv->pixel_size == pixel_size) return;

    if (FT_IS_SCALABLE(ftyuv->face) ) {
        FT_Set_Pixel_Sizes(ftyuv->face, pixel_size, 0);
    }
    else {
        if (pixel_size >= ftyuv->face->num_fixed_sizes )  
            pixel_size = ftyuv->face->num_fixed_sizes - 1;   
        FT_Set_Pixel_Sizes(ftyuv->face,  
                           ftyuv->face->available_sizes[pixel_size].height,  
                           ftyuv->face->available_sizes[pixel_size].width);
    }
    ftyuv->pixel_size = pixel_size;
}
void ftyuv_space (struct ftyuv *ftyuv,
                  const int hspace, const int vspace)
{
    ftyuv->hspace = hspace;
    ftyuv->vspace = vspace;
}

void ftyuv_margin (struct ftyuv *ftyuv,
                   const int hmargin, const int vmargin)
{
    ftyuv->hmargin = hmargin;
    ftyuv->vmargin = vmargin;
}

void ftyuv_color (struct ftyuv *ftyuv,
                  const unsigned font_xrgb, const unsigned border_xrgb)
{
    unsigned char r,g,b;
    /* y = 0.299r + 0.587g + 0.114b
     * u =-0.169r - 0.331g +   0.5b + 128
     * v = 0.5r   - 0.419g - 0.081b + 128
     */
    r = (unsigned char)(font_xrgb>>16);
    g = (unsigned char)(font_xrgb>>8);
    b = (unsigned char)(font_xrgb);
    ftyuv->font_y=(unsigned char) ( 0.299f*r+0.587f*g+0.114f*b);
    ftyuv->font_u=(unsigned char) (-0.169f*r-0.331f*g+  0.5f*b + 128);
    ftyuv->font_v=(unsigned char) (   0.5f*r-0.419f*g-0.081f*b + 128);

    r = (unsigned char)(border_xrgb>>16);
    g = (unsigned char)(border_xrgb>>8);
    b = (unsigned char)(border_xrgb);
    ftyuv->border_y=(unsigned char) ( 0.299f*r+0.587f*g+0.114f*b);
    ftyuv->border_u=(unsigned char) (-0.169f*r-0.331f*g+  0.5f*b + 128);
    ftyuv->border_v=(unsigned char) (   0.5f*r-0.419f*g-0.081f*b + 128);
}

static void
h_line (struct ftyuv *ftyuv, struct line *line)
{
    int i;
    FT_GlyphSlot slot = ftyuv->face->glyph;

    line->width = 0;
    line->height = 0;
    line->bearingY = -0x7FFFFFFF;
    for (i=0;i<line->len; ++i) {
        int height, width;
        FT_Load_Char (ftyuv->face, line->str[i], FT_LOAD_RENDER);
        height = (slot->metrics.height >> 6);
        width  = (slot->metrics.horiAdvance >> 6);

        if (height > line->height )
            line->height = height;
        line->width += width + ftyuv->hspace;
        if( slot->metrics.horiBearingY > line->bearingY )
            line->bearingY = slot->metrics.horiBearingY;
    }
    if (line->width > 0)
        line->width -= ftyuv->hspace;
}
static void
v_line (struct ftyuv *ftyuv, struct line *line)
{
    int i;
    FT_GlyphSlot slot = ftyuv->face->glyph;

    line->width = 0;
    line->height = 0;
    line->bearingX = 0x7FFFFFFF;
    for (i=0;i<line->len; ++i) {
        int height, width;
        FT_Load_Char (ftyuv->face, line->str[i], FT_LOAD_RENDER|FT_LOAD_VERTICAL_LAYOUT);
        width  = (slot->metrics.width >> 6);
        height = (slot->metrics.vertAdvance >> 6);

        if (width > line->width)
            line->width = width;
        line->height += height + ftyuv->vspace;
        if (slot->metrics.vertBearingX < line->bearingX)
            line->bearingX = slot->metrics.vertBearingX;
    }
    if (line->height > 0)
        line->height -= ftyuv->vspace;
}

static void
h_layout (struct ftyuv *ftyuv, struct layout *lt)
{
    int i;
    h_line (ftyuv, lt->line);
    lt->width = lt->line[0].width;
    lt->height = lt->line[0].height;

    for (i=1;i<lt->n_line;++i) {
        h_line (ftyuv, lt->line + i);
        if (lt->line[i].width > lt->width)
            lt->width = lt->line[i].width;
        lt->height += ftyuv->vspace + lt->line[i].height;
    }
}
static void
v_layout (struct ftyuv *ftyuv, struct layout *lt)
{
    int i;
    v_line (ftyuv, lt->line);
    lt->width = lt->line[0].width;
    lt->height = lt->line[0].height;

    for (i=1;i<lt->n_line;++i) {
        v_line (ftyuv, lt->line + i);
        if (lt->line[i].height > lt->height)
            lt->height = lt->line[i].height;
        lt->width += ftyuv->hspace + lt->line[i].width;
    }
}

static unsigned
line_break (const unsigned short *str, unsigned len,
            struct layout *lt)
{
    unsigned i,j,n;
    for (i=j=n=0;i<len && str[i];++i) {
        if (str[i] == '\n') {
            if (i > j) {
                if (lt) {
                    lt->line[n].str = str+j;
                    lt->line[n].len = i-j;
                }
                ++n;
            }
            j = i+1;
        }
    }
    if (i > j) {
        if (lt) {
            lt->line[n].str = str+j;
            lt->line[n].len = i-j;
        }
        ++n;
    }
    return n;
}

static void
raster_yxor (const int y, const int count,
             const FT_Span * const spans,
             void * const user)
{
    int i, j, y_off, x_off, dx;
    unsigned char *y_p;
    const FT_Span *span;
    const struct cxt * const cxt = user;

    y_off = cxt->y_off - y;
    if (0<= y_off && y_off < cxt->y_d) {
        for (i=0; i< count; ++i) {
            span = spans+i;
            x_off = cxt->x_off + span->x;
            if (0<= x_off && x_off < cxt->x_d) {
                y_p   = cxt->y_plane + cxt->y_pitch*y_off + x_off;
                dx = cxt->x_d - x_off;
                dx = MIN (dx, span->len);
                for (j=0;j<dx;++j, ++y_p)
                    *y_p ^= span->coverage;
            }
        }
    }
}
static void
raster_blend (const int y, const int count,
              const FT_Span * const spans,
              void * const user)
{
    int i, j, y_off, x_off, dx;
    unsigned char *y_p, *u_p, *v_p;
    const FT_Span *span;
    const struct cxt * const cxt = user;
    const struct ftyuv* const ftyuv = cxt->ftyuv;

    y_off = cxt->y_off - y;
    if (0<= y_off && y_off < cxt->y_d) {
        for (i=0; i< count; ++i) {
            span = spans+i;
            x_off = cxt->x_off + span->x;
            if (x_off < cxt->x_d) {
                dx = span->len;
                if (x_off < cxt->x) {
                    dx -= cxt->x - x_off;
                    if (dx<=0) continue;
                    x_off = cxt->x;
                }
                dx = MIN (dx, cxt->x_d - x_off);
                y_p   = cxt->y_plane + cxt->y_pitch*y_off + x_off;
                u_p   = cxt->u_plane + cxt->u_pitch*y_off + x_off - cxt->uv_x_off;
                v_p   = cxt->v_plane + cxt->v_pitch*y_off + x_off - cxt->uv_x_off;
                for (j=0;j<dx;++j, ++y_p,++u_p,++v_p) {

                    assert(cxt->u_min<= u_p && u_p<cxt->u_max);
                    assert(cxt->v_min<= v_p && v_p<cxt->v_max);

                    *y_p = (*y_p * (255 - span->coverage) + ftyuv->font_y * span->coverage)>>8;
                    *u_p = (*u_p * (255 - span->coverage) + ftyuv->font_u * span->coverage)>>8;
                    *v_p = (*v_p * (255 - span->coverage) + ftyuv->font_v * span->coverage)>>8;
                }
            }
        }
    }
}
static void
raster_stroke (const int y, const int count,
               const FT_Span * const spans,
               void * const user)
{
    int i, j, y_off, x_off, dx;
    unsigned char *y_p, *u_p, *v_p;
    char *msk_p;
    const FT_Span *span;
    const struct cxt * const cxt = user;
    const struct ftyuv* const ftyuv = cxt->ftyuv;

    y_off = cxt->y_off - y;
    if (0<= y_off && y_off < cxt->y_d) {
        for (i=0; i< count; ++i) {
            span = spans+i;
            x_off = cxt->x_off + span->x;
            if (x_off < cxt->x_d) {
                dx = span->len;
                if (x_off < cxt->x) {
                    dx -= cxt->x - x_off;
                    if (dx<=0) continue;
                    x_off = cxt->x;
                }
                msk_p = cxt->msk+x_off-cxt->x;
                dx = MIN (dx, cxt->x_d - x_off);
                /* left border */
                if (x_off > cxt->x && *(msk_p-1) < y) {
                    y_p   = cxt->y_plane + cxt->y_pitch*y_off + x_off-1;
                    u_p   = cxt->u_plane + cxt->u_pitch*y_off + x_off-1 - cxt->uv_x_off;
                    v_p   = cxt->v_plane + cxt->v_pitch*y_off + x_off-1 - cxt->uv_x_off;

                    assert(cxt->u_min<= u_p && u_p<cxt->u_max);
                    assert(cxt->v_min<= v_p && v_p<cxt->v_max);

                    *y_p++ = ftyuv->border_y;
                    *u_p++ = ftyuv->border_u;
                    *v_p++ = ftyuv->border_v;
                }
                else {
                    y_p   = cxt->y_plane + cxt->y_pitch*y_off + x_off;
                    u_p   = cxt->u_plane + cxt->u_pitch*y_off + x_off - cxt->uv_x_off;
                    v_p   = cxt->v_plane + cxt->v_pitch*y_off + x_off - cxt->uv_x_off;
                }
                for (j=0;j<dx;++j, ++y_p,++u_p,++v_p,++msk_p) {

                    assert(cxt->msk<=msk_p && msk_p<cxt->msk+cxt->clip_w);
                    assert(cxt->u_min<= u_p && u_p<cxt->u_max);
                    assert(cxt->v_min<= v_p && v_p<cxt->v_max);

                    if (span->coverage>10) {
                        if (cxt->flags&FTYUV_BLEND) {
                            *y_p = (*y_p * (255 - span->coverage) + ftyuv->font_y * span->coverage)>>8;
                            *u_p = (*u_p * (255 - span->coverage) + ftyuv->font_u * span->coverage)>>8;
                            *v_p = (*v_p * (255 - span->coverage) + ftyuv->font_v * span->coverage)>>8;
                        }
                        if (*msk_p < y) {/* line border */
                            if (y_off < cxt->height-1) {

                                assert(cxt->u_min<= u_p+cxt->u_pitch && u_p+cxt->u_pitch<cxt->u_max);
                                assert(cxt->v_min<= v_p+cxt->v_pitch && v_p+cxt->v_pitch<cxt->v_max);

                                *(y_p+cxt->y_pitch) = ftyuv->border_y;
                                *(u_p+cxt->u_pitch) = ftyuv->border_u;
                                *(v_p+cxt->v_pitch) = ftyuv->border_v;
                            }
                        }
                        if (y == cxt->bearingY-1) {/* top line */
                            if (y_off > 0) {

                                assert(cxt->u_min<= u_p-cxt->u_pitch && u_p-cxt->u_pitch<cxt->u_max);
                                assert(cxt->v_min<= v_p-cxt->v_pitch && v_p-cxt->v_pitch<cxt->v_max);

                                *(y_p-cxt->y_pitch) = ftyuv->border_y;
                                *(u_p-cxt->u_pitch) = ftyuv->border_u;
                                *(v_p-cxt->v_pitch) = ftyuv->border_v;
                            }
                        }
                    }
                    else if (*msk_p == y) { /* line border */
                        *y_p = ftyuv->border_y;
                        *u_p = ftyuv->border_u;
                        *v_p = ftyuv->border_v;
                    }
                    *msk_p = span->coverage>10 ? (char)(y+1) : -127;
                }
                /* right border */
                if (x_off + dx < cxt->x_d) {

                    assert(cxt->u_min<= u_p && u_p<cxt->u_max);
                    assert(cxt->v_min<= v_p && v_p<cxt->v_max);

                    *y_p = ftyuv->border_y;
                    *u_p = ftyuv->border_u;
                    *v_p = ftyuv->border_v;
                }
            }
        }
    }
}
static void
h_draw (struct cxt *cxt, const struct layout *lt)
{
    int i, j;
    const struct ftyuv *const ftyuv = cxt->ftyuv;
    const struct line *line;
    FT_Raster_Params params;
    const FT_GlyphSlot slot = ftyuv->face->glyph;

    memset (&params, 0, sizeof(params));
    params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
    if (cxt->flags&FTYUV_STROKE)
        params.gray_spans = raster_stroke;
    else if (cxt->flags&FTYUV_BLEND)
        params.gray_spans = raster_blend;
    else
        params.gray_spans = raster_yxor;
    params.user = cxt;

    cxt->y = lt->y;
    for (i=0;i<lt->n_line;++i) {
        line = lt->line + i;
        /* layout line */
        if ((cxt->flags&FTYUV_LINE_HCENTER) == FTYUV_LINE_HCENTER)
            cxt->x = lt->x + (lt->width - line->width)/2;
        else if (cxt->flags&FTYUV_LINE_RIGHT)
            cxt->x = lt->x + lt->width - line->width;
        else
            cxt->x = lt->x;
        cxt->x_d = cxt->x + line->width;
        cxt->y_d = cxt->y + line->height;
        if (cxt->x_d > cxt->width) cxt->x_d = cxt->width;
        if (cxt->y_d > cxt->height) cxt->y_d = cxt->height;

        /* draw line */
        cxt->y_off = cxt->y +(line->bearingY >> 6)-1;
        cxt->x_off = cxt->x;
        if (cxt->flags & FTYUV_STROKE) {
            cxt->y_off += 1;
            cxt->x_off += 1;
        }
        cxt->x = MAX(cxt->x, 0);
        for (j=0;j<line->len && cxt->x_off < cxt->x_d; ++j) {
            FT_UInt index;

            index = FT_Get_Char_Index (ftyuv->face, line->str[j]);
            FT_Load_Glyph (ftyuv->face, index, FT_LOAD_NO_BITMAP);

            if (cxt->flags&FTYUV_STROKE) {
                memset (cxt->msk, -127, cxt->clip_w);
                cxt->bearingY = (slot->metrics.horiBearingY>>6);
            }
            FT_Outline_Render (ftyuv->lib, &slot->outline, &params);

            cxt->x_off += (slot->metrics.horiAdvance >> 6) + ftyuv->hspace;
        }

        cxt->y += line->height + ftyuv->vspace;
    }
}

static void
v_draw (struct cxt *cxt, const struct layout *lt)
{
    int i, j, y;
    const struct ftyuv *const ftyuv = cxt->ftyuv;
    const struct line *line;
    FT_Raster_Params params;
    const FT_GlyphSlot slot = ftyuv->face->glyph;

    memset (&params, 0, sizeof(params));
    params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
    if (cxt->flags&FTYUV_STROKE)
        params.gray_spans = raster_stroke;
    else if (cxt->flags&FTYUV_BLEND)
        params.gray_spans = raster_blend;
    else
        params.gray_spans = raster_yxor;
    params.user = cxt;

    cxt->x = lt->x;
    for (i=0;i<lt->n_line;++i) {
        line = lt->line + i;
        /* layout line */
        if ((cxt->flags&FTYUV_LINE_VCENTER) == FTYUV_LINE_VCENTER)
            cxt->y = lt->y + (lt->height - line->height)/2;
        else if (cxt->flags&FTYUV_LINE_BOTTOM)
            cxt->y = lt->y + lt->height - line->height;
        else
            cxt->y = lt->y;
        cxt->x_d = cxt->x + line->width;
        cxt->y_d = cxt->y + line->height;
        if (cxt->x_d > cxt->width) cxt->x_d = cxt->width;
        if (cxt->y_d > cxt->height) cxt->y_d = cxt->height;

        /* draw line */
        y = cxt->y;
        for (j=0;j<line->len && y < cxt->y_d; ++j) {
            FT_UInt index;

            index = FT_Get_Char_Index (ftyuv->face, line->str[j]);
            FT_Load_Glyph (ftyuv->face, index, FT_LOAD_NO_BITMAP|FT_LOAD_VERTICAL_LAYOUT);

            if (cxt->flags&FTYUV_STROKE) {
                memset (cxt->msk, -127, cxt->clip_w);
                cxt->bearingY = (slot->metrics.horiBearingY>>6);
            }
            cxt->x_off = cxt->x + ((slot->metrics.vertBearingX - line->bearingX) >> 6)-1;
            cxt->y_off = y + ((slot->metrics.vertBearingY + slot->metrics.horiBearingY) >>6) - 2;
            if (cxt->flags & FTYUV_STROKE) {
                cxt->y_off += 1;
                cxt->x_off += 1;
            }
            FT_Outline_Render (ftyuv->lib, &slot->outline, &params);

            y += (slot->metrics.vertAdvance >> 6) + ftyuv->vspace;
        }

        cxt->x += line->width + ftyuv->hspace;
    }
}

static void
layout (const struct cxt *cxt, struct layout *lt)
{
    if (cxt->flags & FTYUV_VERTICAL)
        v_layout (cxt->ftyuv, lt);
    else 
        h_layout (cxt->ftyuv, lt);
    if (cxt->flags & FTYUV_STROKE) {
        lt->width += 2;
        lt->height += 2;
    }
    /* location (x, y) */
    if( (cxt->flags&FTYUV_HCENTER) == FTYUV_HCENTER)
        lt->x = (cxt->width - lt->width)/2;
    else if (cxt->flags&FTYUV_RIGHT)
        lt->x = cxt->width - lt->width - cxt->ftyuv->hmargin;
    else
        lt->x = cxt->ftyuv->hmargin;

    if( (cxt->flags&FTYUV_VCENTER) == FTYUV_VCENTER)
        lt->y = (cxt->height - lt->height)/2;
    else if (cxt->flags&FTYUV_BOTTOM)
        lt->y = cxt->height - lt->height - cxt->ftyuv->vmargin;
    else
        lt->y = cxt->ftyuv->vmargin;
}
static void
draw_yuv444 (struct cxt *cxt, const struct layout *lt)
{
#ifdef _DEBUG
    cxt->u_min = cxt->u_plane;
    cxt->v_min = cxt->v_plane;
    cxt->u_max = cxt->u_min + cxt->u_pitch*cxt->height;
    cxt->v_max = cxt->v_min + cxt->v_pitch*cxt->height;
#endif
    if (cxt->flags & FTYUV_VERTICAL)
        v_draw (cxt, lt);
    else
        h_draw (cxt, lt);
}
static void
draw_yuv420 (struct cxt *cxt, const struct layout *lt)
{
    struct ftyuv* const ftyuv = cxt->ftyuv;
    unsigned char * const u_p = cxt->u_plane;
    unsigned char * const v_p = cxt->v_plane; 
    const int w_2 = cxt->width>>1;

    /* alloc u/v plane */
    if (cxt->flags & (FTYUV_BLEND|FTYUV_STROKE)) {
        int j, m, n;
        int i=lt->y-3;
        int k=lt->x-3;
        int y_d = lt->y + lt->height + 3;
        int x_d = lt->x + lt->width  + 3;
 
        y_d = MIN(y_d, cxt->height);
        x_d = MIN(x_d, cxt->width);
        i = MAX (i, 0);
        k = MAX (k, 0);

        cxt->uv_x_off = k;
        cxt->clip_w = x_d-k;

        cxt->v_pitch = cxt->u_pitch = cxt->clip_w;
        m = cxt->clip_w*(y_d-i);
        n = (m<<1);
        if (cxt->flags&FTYUV_STROKE)
            n += cxt->clip_w;
        if (n > ftyuv->tmp_len) {
            ftyuv->tmp_buf = realloc (ftyuv->tmp_buf, n);
            ftyuv->tmp_len = n;
        }
        cxt->u_plane = ftyuv->tmp_buf;
        cxt->v_plane = cxt->u_plane+m;
        cxt->msk = (char*)cxt->v_plane+m;
#ifdef _DEBUG
        cxt->u_min = cxt->u_plane;
        cxt->v_min = cxt->v_plane;
        cxt->u_max = cxt->u_min + m;
        cxt->v_max = cxt->v_min + m;
#endif
        cxt->u_plane -= cxt->u_pitch*i;
        cxt->v_plane -= cxt->v_pitch*i;
        for (;i<y_d;++i) {
            for (j=k;j<x_d;++j) {
                m = i*cxt->u_pitch + (j - cxt->uv_x_off);
                n = (i>>1)*w_2 + (j>>1);

                assert(cxt->u_min<= cxt->u_plane+m && cxt->u_plane+m<cxt->u_max);
                assert(cxt->v_min<= cxt->v_plane+m && cxt->v_plane+m<cxt->v_max);

                cxt->u_plane[m] = u_p[n];
                cxt->v_plane[m] = v_p[n];
            }
        }
    }

    if (cxt->flags & FTYUV_VERTICAL)
        v_draw (cxt, lt);
    else
        h_draw (cxt, lt);

    /* update u/v plane */
    if (cxt->flags & (FTYUV_BLEND|FTYUV_STROKE)) {
        int j, m, n0, n1, n2, n3;
        int i=lt->y-2;
        int k=lt->x-2;
        int y_d = lt->y + lt->height + 2;
        int x_d = lt->x + lt->width  + 2;

        y_d = MIN(y_d, cxt->height);
        x_d = MIN(x_d, cxt->width);
        i = MAX (i, 0);
        k = MAX (k, 0);

        y_d >>=1; x_d >>=1;
        i >>=1; k>>=1;

        for (;i<y_d;++i) {
            for (j=k;j<x_d;++j) {
                m = i*w_2+j;
                n0= (i<<1)*cxt->u_pitch + ((j<<1) - cxt->uv_x_off);
                n1= n0 + cxt->u_pitch;
                n2= n0 + 1;
                n3= n0 + cxt->u_pitch + 1;

                assert(cxt->u_min<= cxt->u_plane+n0 && cxt->u_plane+n3<cxt->u_max);
                assert(cxt->v_min<= cxt->v_plane+n0 && cxt->v_plane+n3<cxt->v_max);

                u_p[m] = (cxt->u_plane[n0] + cxt->u_plane[n1] + cxt->u_plane[n2] + cxt->u_plane[n3])>>2;
                v_p[m] = (cxt->v_plane[n0] + cxt->v_plane[n1] + cxt->v_plane[n2] + cxt->v_plane[n3])>>2;
            }
        }
    }
}
void
ftyuv_draw (struct ftyuv *ftyuv,
            const unsigned short *str, const unsigned len,
            unsigned char* yuv, const unsigned width, const unsigned height,
            const unsigned flags)
{
    struct layout lt;
    struct cxt cxt;

    /* check args */
    if (!ftyuv || !str || !yuv)
        return;
    if (flags&FTYUV_PACKED) /*TODO not support packed yuv */
        return;
    if (flags&FTYUV_VERTICAL) {
        if (!FT_HAS_VERTICAL(ftyuv->face))
            return;
    }
    ftyuv->ref_count++;

    cxt.ftyuv = ftyuv;
    cxt.width = width;
    cxt.height = height;
    cxt.flags = flags;
    cxt.y_plane = yuv;
    cxt.y_pitch = width;

    /* layout */
    lt.n_line = line_break (str, len, NULL);
    if (lt.n_line > ftyuv->n_line) {
        ftyuv->line_buf = realloc (ftyuv->line_buf, sizeof(struct line)*lt.n_line);
        ftyuv->n_line = lt.n_line;
    }
    lt.line = ftyuv->line_buf;
    line_break (str, len, &lt);
    layout (&cxt, &lt);

    if (flags&FTYUV_420) {
        if (flags&FTYUV_PACKED) {
        }
        else {
            cxt.u_plane = yuv + cxt.width*cxt.height;
            cxt.v_plane = cxt.u_plane + (cxt.width>>1) * (cxt.height>>1);
            cxt.v_pitch = cxt.u_pitch = width/2;
        }
        draw_yuv420 (&cxt, &lt);
    }
    else {
        if (flags&FTYUV_PACKED) {
        }
        else {
            cxt.u_plane = yuv + cxt.width*cxt.height;
            cxt.v_plane = cxt.u_plane +  cxt.width*cxt.height;
            cxt.v_pitch = cxt.u_pitch = width;
        }
        draw_yuv444 (&cxt, &lt);
    }

    ftyuv_unref (ftyuv);
}
