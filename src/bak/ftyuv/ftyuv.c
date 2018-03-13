#include "ftyuv.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#ifdef  __GNUC__
#undef alloca
#define alloca(size)   __builtin_alloca (size)
#elif defined(_MSC_VER) || defined(__DMC__)
#include <malloc.h>
#define alloca _alloca
#endif
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

struct _ftyuv
{
    int hspace, vspace;
    int hmargin, vmargin;
    FT_Face face;
    FT_Library lib;
    int ref_count;
    unsigned char font_y,font_u,font_v;
    unsigned char border_y, border_u, border_v;
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

ftyuv*
ftyuv_new (const char* fontfile, int ptsize)
{
    ftyuv *cxt;
    FT_Face face;
    FT_Library lib;

    FT_Open_Args args = {0,};
    if (FT_Init_FreeType(&lib))
        return NULL;

    args.flags = FT_OPEN_PATHNAME;
    args.pathname = (FT_String*) fontfile;
    if (FT_Open_Face(lib, &args, 0, &face))
        return NULL;
    if ( FT_IS_SCALABLE(face) ) {
        /* Set the character size and use default DPI (72) */  
        if (FT_Set_Char_Size(face, 0, ptsize * 64, 0, 0 ))  
            return NULL;  
    }
    else {
        /* Non-scalable font case.  ptsize determines which family 
         * or series of fonts to grab from the non-scalable format. 
         * It is not the point size of the font. 
         */ 
        if ( ptsize >= face->num_fixed_sizes )  
            ptsize = face->num_fixed_sizes - 1;   
        if (FT_Set_Pixel_Sizes(face,  
                               face->available_sizes[ptsize].height,  
                               face->available_sizes[ptsize].width ))
            return NULL;
    }

    cxt = malloc (sizeof(ftyuv));
    memset(cxt, 0, sizeof(ftyuv));
    cxt->lib = lib;
    cxt->face = face;
    cxt->ref_count = 1;
    return cxt;
}

void
ftyuv_unref (ftyuv* cxt)
{
    if (--cxt->ref_count == 0) {
        if (cxt->face)
            FT_Done_Face(cxt->face);

        if (cxt->lib)
            FT_Done_FreeType (cxt->lib);
        free(cxt);
    }
}

void ftyuv_space (ftyuv *cxt,
                  const int hspace, const int vspace)
{
    cxt->hspace = hspace;
    cxt->vspace = vspace;
}

void ftyuv_margin (ftyuv *cxt,
                   const int hmargin, const int vmargin)
{
    cxt->hmargin = hmargin;
    cxt->vmargin = vmargin;
}

void ftyuv_color (ftyuv *cxt,
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
    cxt->font_y=(unsigned char) ( 0.299f*r+0.587f*g+0.114f*b);
    cxt->font_u=(unsigned char) (-0.169f*r-0.331f*g+  0.5f*b + 128);
    cxt->font_v=(unsigned char) (   0.5f*r-0.419f*g-0.081f*b + 128);

    r = (unsigned char)(border_xrgb>>16);
    g = (unsigned char)(border_xrgb>>8);
    b = (unsigned char)(border_xrgb);
    cxt->border_y=(unsigned char) ( 0.299f*r+0.587f*g+0.114f*b);
    cxt->border_u=(unsigned char) (-0.169f*r-0.331f*g+  0.5f*b + 128);
    cxt->border_v=(unsigned char) (   0.5f*r-0.419f*g-0.081f*b + 128);
}

static void
h_line (ftyuv *cxt, struct line *line)
{
    int i;
    FT_GlyphSlot slot = cxt->face->glyph;

    line->width = 0;
    line->height = 0;
    line->bearingY = 0;
    for (i=0;i<line->len; ++i) {
        int height, width;
        FT_Load_Char (cxt->face, line->str[i], FT_LOAD_RENDER);
        height = (slot->metrics.height >> 6);
        width  = (slot->advance.x >> 6);

        if (height > line->height )
            line->height = height;
        line->width += width + cxt->hspace;
        if( slot->metrics.horiBearingY > line->bearingY )
            line->bearingY = slot->metrics.horiBearingY;
    }
    if (line->width > 0)
        line->width -= cxt->hspace;
}
static void
v_line (ftyuv *cxt, struct line *line)
{
    int i;
    FT_GlyphSlot slot = cxt->face->glyph;

    line->width = 0;
    line->height = 0;
    line->bearingX = 0;
    for (i=0;i<line->len; ++i) {
        int height, width;
        FT_Load_Char (cxt->face, line->str[i], FT_LOAD_RENDER|FT_LOAD_VERTICAL_LAYOUT);
        width  = (slot->metrics.width >> 6);
        height = (slot->advance.y >> 6);

        if (width > line->width )
            line->width = width;
        line->height += height + cxt->vspace;
        if( slot->metrics.horiBearingX > line->bearingX )
            line->bearingX = slot->metrics.horiBearingX;
    }
    if (line->height > 0)
        line->height -= cxt->vspace;
}

static void
h_layout (ftyuv *cxt, struct layout *lt)
{
    int i;
    h_line (cxt, lt->line);
    lt->width = lt->line[0].width;
    lt->height = lt->line[0].height;

    for (i=1;i<lt->n_line;++i) {
        h_line (cxt, lt->line + i);
        if (lt->line[i].width > lt->width)
            lt->width = lt->line[i].width;
        lt->height += cxt->vspace + lt->line[i].height;
    }
}
static void
v_layout (ftyuv *cxt, struct layout *lt)
{
    int i;
    v_line (cxt, lt->line);
    lt->width = lt->line[0].width;
    lt->height = lt->line[0].height;

    for (i=1;i<lt->n_line;++i) {
        v_line (cxt, lt->line + i);
        if (lt->line[i].height > lt->height)
            lt->height = lt->line[i].height;
        lt->width += cxt->hspace + lt->line[i].width;
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
h_xor (ftyuv* cxt, struct line *line,
       unsigned char* y_base, unsigned char* u_base,  unsigned char*v_base, int width,
       int x, int y, int dx, int dy, unsigned flags)
{
    int i, offset;
    FT_GlyphSlot slot = cxt->face->glyph;

    (int*)y_base; (int*)u_base; (int*)v_base; (int)flags;
    for (i=offset=0;i<line->len; ++i) {
        FT_Int advance;
        unsigned char* buf, *y_p;
        int j,k,cols,rows;
        int y_off, x_off, b_off;

        FT_Load_Char (cxt->face, line->str[i], FT_LOAD_RENDER);

        advance = slot->advance.x >> 6;
        buf = slot->bitmap.buffer;

        rows = MIN (slot->bitmap.rows, dy);
        cols = slot->bitmap.width;
        if (offset + advance > dx)
            break;

        y_off = y+(line->bearingY >> 6) - (slot->metrics.horiBearingY >> 6);
        x_off = x+(slot->metrics.horiBearingX >> 6);

        if(FT_PIXEL_MODE_GRAY == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off) + (x_off+offset);
                y_p = y_base + b_off;
                for (k=0;k<cols;++k,++y_p)
                    *y_p ^= buf[k];
                buf += slot->bitmap.pitch;
            }
        }
        else if(FT_PIXEL_MODE_MONO == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off) + (x_off+offset);
                y_p = y_base + b_off;
                for (k=0;k<cols;++k,++y_p) {
                    if (buf[k/8]&(0x80>>(k&7)))
                        *y_p ^= 0xFF;
                }
                buf += slot->bitmap.pitch;
            }
        }
        offset += advance + cxt->hspace;
        if (offset>= dx)
            break;
    }
}
static void
h_fill (ftyuv* cxt, struct line *line,
        unsigned char* y_base, unsigned char* u_base,  unsigned char*v_base, int width,
        int x, int y, int dx, int dy, unsigned flags)
{
    int i,offset;
    FT_GlyphSlot slot = cxt->face->glyph;

    for (i=offset=0;i<line->len; ++i) {
        FT_Int advance;
        unsigned char *buf, *y_p, *u_p, *v_p;
        int j,k,cols,rows;
        int y_off, x_off, b_off;

        if (flags & FTYUV_STROKE)
            j =  FT_LOAD_RENDER | FT_LOAD_TARGET_MONO;
        else 
            j =  FT_LOAD_RENDER;

        FT_Load_Char (cxt->face, line->str[i], j);

        advance = slot->advance.x >> 6;
        buf = slot->bitmap.buffer;

        rows = MIN (slot->bitmap.rows, dy);
        cols = slot->bitmap.width;
        if (offset + advance > dx)
            break;

        y_off = y+(line->bearingY >> 6) - (slot->metrics.horiBearingY >> 6);
        x_off = x+(slot->metrics.horiBearingX >> 6);

        if(FT_PIXEL_MODE_GRAY == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off) + (x_off+offset);
                y_p = y_base + b_off;
                u_p = u_base + b_off;
                v_p = v_base + b_off;
                for (k=0;k<cols;++k,++y_p,++u_p,++v_p) {
                    *y_p = (*y_p * (255 - buf[k]) + cxt->font_y * buf[k])>>8;
                    *u_p = (*u_p * (255 - buf[k]) + cxt->font_u * buf[k])>>8;
                    *v_p = (*v_p * (255 - buf[k]) + cxt->font_v * buf[k])>>8;
                }
                buf += slot->bitmap.pitch;
            }
        }
        else if(FT_PIXEL_MODE_MONO == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off) + (x_off+offset);
                y_p = y_base + b_off;
                u_p = u_base + b_off;
                v_p = v_base + b_off;
                for (k=0;k<cols;++k,++y_p,++u_p,++v_p) {
                    if (!(buf[k/8]&(0x80>>(k&7)))) {
                        if (flags & FTYUV_STROKE) {
                            int w = slot->bitmap.pitch;
                            unsigned char *b = slot->bitmap.buffer;
#define _MSK(j,k) (b[(j)*w+(k)/8]&(0x80>>((k)&7)))
                            if ((j>0 && k>0 &&_MSK(j-1, k-1)) ||
                                (j>0 && _MSK(j-1, k)) ||
                                (j>0 && k<cols-1 && _MSK(j-1, k+1)) ||
                                (k>0 && _MSK(j, k-1)) ||
                                (k<cols-1 && _MSK(j, k+1)) ||
                                (j<rows-1 && k>0 && _MSK(j+1, k-1)) ||
                                (j<rows-1 && _MSK(j+1, k)) ||
                                (j<rows-1 && k<cols-1 && _MSK(j+1, k+1))) {
#undef _MSK
                                *y_p = cxt->border_y;
                                *u_p = cxt->border_u;
                                *v_p = cxt->border_v;
                            }
                        }
                    }
                    else {
                        *y_p = cxt->font_y;
                        *u_p = cxt->font_u;
                        *v_p = cxt->font_v;
                    }
                }
                buf += slot->bitmap.pitch;
            }
        }
        offset += advance + cxt->hspace;
        if (offset>= dx)
            break;
    }
}
static void
h_blend (ftyuv *cxt, const struct layout *lt, struct line *line,
         unsigned char* y_base, unsigned char* u_base, unsigned char* v_base,
         const int width, const int height,
         int x, int y, unsigned flags)
{
    int dx, dy;

    if ((flags&FTYUV_LINE_HCENTER) == FTYUV_LINE_HCENTER)
        x += (lt->width - line->width)/2;
    else if (flags&FTYUV_LINE_RIGHT)
        x += lt->width - line->width;
    /* check safe region */
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + line->width > width)
        dx = width - x;
    else
        dx = line->width;
    if (y + line->height > height)
        dy = height - y;
    else
        dy =line->height;

    if (dx <= 0 || dy <= 0)
        return;

    if (flags & FTYUV_BLEND)
        h_fill (cxt, line, y_base, u_base, v_base, width, x, y, dx, dy, flags);
    else
        h_xor (cxt, line, y_base, u_base, v_base, width, x, y, dx, dy, flags);
}
static void
v_xor (ftyuv* cxt, struct line *line,
       unsigned char* y_base, unsigned char* u_base,  unsigned char*v_base, int width,
       int x, int y, int dx, int dy, unsigned flags)
{
    int i, offset;
    FT_GlyphSlot slot = cxt->face->glyph;

    (int*)y_base; (int*)u_base; (int*)v_base; (int)flags;
    for (i=offset=0;i<line->len; ++i) {
        FT_Int advance;
        unsigned char* buf, *y_p;
        int j,k,cols,rows;
        int y_off, x_off, b_off;

        FT_Load_Char (cxt->face, line->str[i], FT_LOAD_RENDER|FT_LOAD_VERTICAL_LAYOUT);

        advance = slot->advance.y >> 6;
        buf = slot->bitmap.buffer;

        cols = MIN (slot->bitmap.width, dx);
        rows = slot->bitmap.rows;
        if (offset + advance > dy)
            break;

        x_off = x+(line->bearingX >> 6) - (slot->metrics.horiBearingX >> 6);
        y_off = y+(slot->metrics.horiBearingY >> 6);

        if(FT_PIXEL_MODE_GRAY == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off+offset) + x_off;
                y_p = y_base + b_off;
                for (k=0;k<cols;++k,++y_p)
                    *y_p ^= buf[k];
                buf += slot->bitmap.pitch;
            }
        }
        else if(FT_PIXEL_MODE_MONO == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off+offset) + x_off;
                y_p = y_base + b_off;
                for (k=0;k<cols;++k,++y_p) {
                    if (buf[k/8]&(0x80>>(k&7)))
                        *y_p ^= 0xFF;
                }
                buf += slot->bitmap.pitch;
            }
        }
        offset += advance + cxt->vspace;
        if (offset>= dy)
            break;
    }
}
static void
v_fill (ftyuv* cxt, struct line *line,
        unsigned char* y_base, unsigned char* u_base,  unsigned char*v_base, int width,
        int x, int y, int dx, int dy, unsigned flags)
{
    int i,offset;
    FT_GlyphSlot slot = cxt->face->glyph;

    for (i=offset=0;i<line->len; ++i) {
        FT_Int advance;
        unsigned char* buf, *y_p, *u_p, *v_p;
        int j,k,cols,rows;
        int y_off, x_off, b_off;

        if (flags & FTYUV_STROKE)
            j =  FT_LOAD_RENDER | FT_LOAD_VERTICAL_LAYOUT | FT_LOAD_TARGET_MONO;
        else 
            j =  FT_LOAD_RENDER | FT_LOAD_VERTICAL_LAYOUT;

        FT_Load_Char (cxt->face, line->str[i], j);

        advance = slot->advance.y >> 6;
        buf = slot->bitmap.buffer;

        cols = MIN (slot->bitmap.width, dx);
        rows = slot->bitmap.rows;
        if (offset + advance > dy)
            break;

        x_off = x+(line->bearingX >> 6) - (slot->metrics.horiBearingX >> 6);
        y_off = y+(slot->metrics.horiBearingY >> 6);

        if(FT_PIXEL_MODE_GRAY == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off+offset) + x_off;
                y_p = y_base + b_off;
                u_p = u_base + b_off;
                v_p = v_base + b_off;
                for (k=0;k<cols;++k,++y_p,++u_p,++v_p) {
                    *y_p = (*y_p * (255 - buf[k]) + cxt->font_y * buf[k])>>8;
                    *u_p = (*u_p * (255 - buf[k]) + cxt->font_u * buf[k])>>8;
                    *v_p = (*v_p * (255 - buf[k]) + cxt->font_v * buf[k])>>8;
                }
                buf += slot->bitmap.pitch;
            }
        }
        else if(FT_PIXEL_MODE_MONO == slot->bitmap.pixel_mode) {
            for (j=0;j<rows;++j) {
                b_off = width*(j+y_off+offset) + x_off;
                y_p = y_base + b_off;
                u_p = u_base + b_off;
                v_p = v_base + b_off;
                for (k=0;k<cols;++k,++y_p,++u_p,++v_p) {
                    if (!(buf[k/8]&(0x80>>(k&7)))) {
                        if (flags & FTYUV_STROKE) {
                            int w = slot->bitmap.pitch;
                            unsigned char *b = slot->bitmap.buffer;
#define _MSK(j,k) (b[(j)*w+(k)/8]&(0x80>>((k)&7)))
                            if ((j>0 && k>0 &&_MSK(j-1, k-1)) ||
                                (j>0 && _MSK(j-1, k)) ||
                                (j>0 && k<cols-1 && _MSK(j-1, k+1)) ||
                                (k>0 && _MSK(j, k-1)) ||
                                (k<cols-1 && _MSK(j, k+1)) ||
                                (j<rows-1 && k>0 && _MSK(j+1, k-1)) ||
                                (j<rows-1 && _MSK(j+1, k)) ||
                                (j<rows-1 && k<cols-1 && _MSK(j+1, k+1))) {
#undef _MSK
                                *y_p = cxt->border_y;
                                *u_p = cxt->border_u;
                                *v_p = cxt->border_v;
                            }
                        }
                    }
                    else {
                        *y_p = cxt->font_y;
                        *u_p = cxt->font_u;
                        *v_p = cxt->font_v;
                    }
                }
                buf += slot->bitmap.pitch;
            }
        }
        offset += advance + cxt->vspace;
        if (offset>= dy)
            break;
    }
}
static void
v_blend (ftyuv *cxt, const struct layout *lt, struct line *line,
         unsigned char* y_base, unsigned char* u_base, unsigned char* v_base,
         const int width, const int height,
         int x, int y, unsigned flags)
{
    int dx, dy;

    if ((flags&FTYUV_LINE_VCENTER) == FTYUV_LINE_VCENTER)
        y += (lt->height - line->height)/2;
    else if (flags&FTYUV_LINE_BOTTOM)
        y += lt->height - line->height;
    /* check safe region */
    if (x <0) x = 0;
    if (y <0) y = 0;
    if (x + line->width > width)
        dx = width - x;
    else
        dx = line->width;
    if (y + line->height > height)
        dy = height - y;
    else
        dy =line->height;
    if (dx <= 0 || dy <= 0)
        return;

    if (flags & FTYUV_BLEND)
        v_fill (cxt, line, y_base, u_base, v_base, width, x, y, dx, dy, flags);
    else
        v_xor (cxt, line, y_base, u_base, v_base, width, x, y, dx, dy, flags); 
}
static void
h_draw (ftyuv *cxt, const struct layout *lt,
        unsigned char* y_base, unsigned char* u_base, unsigned char* v_base,
        int width, int height,
        unsigned flags)
{
    int i, j;

    for (i=j=0;i<lt->n_line;++i) {
        h_blend (cxt, lt, lt->line+i,
                 y_base, u_base, v_base,
                 width, height,
                 lt->x, lt->y+j, flags);
        j += lt->line[i].height + cxt->vspace;
    }
}
static void
v_draw (ftyuv *cxt, const struct layout *lt,
        unsigned char* y_base, unsigned char* u_base, unsigned char* v_base,
        int width, int height,
        unsigned flags)
{
    int i, j;
    for (i=j=0;i<lt->n_line;++i) {
        v_blend (cxt, lt, lt->line+i,
                 y_base, u_base, v_base,
                 width, height,
                 lt->x+j, lt->y, flags);
        j += lt->line[i].width + cxt->hspace;
    }
}

void
ftyuv_draw (ftyuv *cxt,
            const unsigned short *str, const unsigned len,
            unsigned char* i420, const unsigned width, const unsigned height,
            const unsigned flags)
{
    int i, j,w_2,h_2;
    unsigned char *u_base, *v_base;
    unsigned char *u_p, *v_p; 
    struct layout lt;

    if (!cxt || !str || !i420)
        return;

    if ((flags&(FTYUV_HORIZONTAL|FTYUV_VERTICAL))
        == (FTYUV_HORIZONTAL|FTYUV_VERTICAL))
        return;

    if ((flags&(FTYUV_YXOR|FTYUV_BLEND))
        == (FTYUV_YXOR|FTYUV_BLEND))
        return;

    if (flags&FTYUV_VERTICAL) {
        if (!FT_HAS_VERTICAL(cxt->face))
            return;
    }

    cxt->ref_count++;
    lt.n_line = line_break (str, len, NULL);
    lt.line = alloca (sizeof(struct line) * lt.n_line);
    line_break (str, len, &lt);

    if (flags & FTYUV_VERTICAL)
        v_layout (cxt, &lt);
    else 
        h_layout (cxt, &lt);

    /* location (x, y) */
    if( (flags&FTYUV_HCENTER) == FTYUV_HCENTER)
        lt.x = (width - lt.width)/2;
    else if (flags&FTYUV_RIGHT)
        lt.x = width - lt.width - cxt->hmargin;
    else
        lt.x = cxt->hmargin;

    if( (flags&FTYUV_VCENTER) == FTYUV_VCENTER)
        lt.y = (height - lt.height)/2;
    else if (flags&FTYUV_BOTTOM)
        lt.y = height - lt.height - cxt->vmargin;
    else
        lt.y = cxt->vmargin;

    w_2 = width>>1;
    h_2 = height>>1;
    u_p = i420 + width*height;
    v_p = u_p + w_2*h_2;

    /* alloc u v plane */
    if (flags& FTYUV_BLEND) {
        int m,n;
        int dy = lt.y + lt.height;
        int dx = lt.x + lt.width;
        m = width*height;
        if (dy > (int)height) dy = height;
        if (dx > (int)width)  dx = width;
        /* TODO: reduce alloca buf */
        u_base = alloca (m);
        v_base = alloca (m);
        for (i=lt.y;i<dy;++i) {
            for (j=lt.x;j<dx;++j) {
                m = i*width+j;
                n = (i>>1)*w_2 + (j>>1);
                u_base[m] = u_p[n];
                v_base[m] = v_p[n];
            }
        }
    }
    else {
        u_base = u_p;
        v_base = v_p;
    }

    if (flags & FTYUV_VERTICAL)
        v_draw (cxt, &lt, i420, u_base, v_base, width, height, flags);
    else
        h_draw (cxt, &lt, i420, u_base, v_base, width, height, flags);

    if (flags& FTYUV_BLEND) {
        int dy = (lt.y + lt.height)>>1;
        int dx = (lt.x + lt.width)>>1;
        if (dy > h_2) dy = h_2;
        if (dx > w_2) dx = w_2;
        for (i=lt.y>>1;i<dy;++i) {
            for (j=lt.x>>1;j<dx;++j) {
                int m = i*w_2+j;
                int n0= (i<<1)*width + (j<<1);
                int n1= n0 + width;
                int n2= n0 + 1;
                int n3= n0 + width + 1;

                u_p[m] = (u_base[n0] + u_base[n1] + u_base[n2] + u_base[n3])>>2;
                v_p[m] = (v_base[n0] + v_base[n1] + v_base[n2] + v_base[n3])>>2;
            }
        }
    }

    ftyuv_unref (cxt);
}
