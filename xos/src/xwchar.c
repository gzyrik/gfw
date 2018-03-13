#include "config.h"
#include "xwchar.h"
#include "xmem.h"
#include "xmsg.h"
#include "xprintf.h"
#include "xslist.h"
#include "xslice.h"
#include "xtest.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>              /* For tolower() */

#define UTF8_COMPUTE(Char, Mask, Len)					                \
    if (Char < 128)	{									                \
        Len = 1;								                        \
        Mask = 0x7f;							                        \
    } else if ((Char & 0xe0) == 0xc0) {									\
        Len = 2;								                        \
        Mask = 0x1f;							                        \
    } else if ((Char & 0xf0) == 0xe0) {								    \
        Len = 3;								                        \
        Mask = 0x0f;							                        \
    } else if ((Char & 0xf8) == 0xf0) {								    \
        Len = 4;								                        \
        Mask = 0x07;							                        \
    } else if ((Char & 0xfc) == 0xf8) {								    \
        Len = 5;								                        \
        Mask = 0x03;							                        \
    } else if ((Char & 0xfe) == 0xfc) {								    \
        Len = 6;								                        \
        Mask = 0x01;							                        \
    } else Len = -1;

#define UTF8_GET(Result, Chars, Count, Mask, Len)			            \
    (Result) = (Chars)[0] & (Mask);					                    \
for ((Count) = 1; (Count) < (Len); ++(Count)) {						    \
    if (((Chars)[(Count)] & 0xc0) != 0x80) {						    \
        (Result) = -1;						                            \
        break;							                                \
    }								                                    \
    (Result) <<= 6;							                            \
    (Result) |= ((Chars)[(Count)] & 0x3f);				                \
}
#define UNICODE_VALID(Char)                                             \
    ((Char) < 0x110000 &&                                               \
     (((Char) & 0xFFFFF800) != 0xD800) &&                               \
     ((Char) < 0xFDD0 || (Char) > 0xFDEF) &&                            \
     ((Char) & 0xFFFE) != 0xFFFE)

#define ISZEROWIDTHTYPE(Type)	IS ((Type),			                    \
                                    OR (G_UNICODE_NON_SPACING_MARK,	    \
                                        OR (G_UNICODE_ENCLOSING_MARK,	\
                                            OR (G_UNICODE_FORMAT,		0))))

static const xchar utf8_skip_data[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};
static inline xwchar
get_wchar_extended      (xcstr          p,
                         xssize         max_len)
{
    xssize  i, len;
    xwchar  min_code;
    xwchar  wc = (xuchar) *p;

    if (wc < 0x80) {
        return wc;
    }
    else if (X_UNLIKELY (wc < 0xc0)) {
        return (xwchar)-1;
    }
    else if (wc < 0xe0) {
        len = 2;
        wc &= 0x1f;
        min_code = 1 << 7;
    }
    else if (wc < 0xf0) {
        len = 3;
        wc &= 0x0f;
        min_code = 1 << 11;
    }
    else if (wc < 0xf8) {
        len = 4;
        wc &= 0x07;
        min_code = 1 << 16;
    }
    else if (wc < 0xfc) {
        len = 5;
        wc &= 0x03;
        min_code = 1 << 21;
    }
    else if (wc < 0xfe) {
        len = 6;
        wc &= 0x01;
        min_code = 1 << 26;
    }
    else {
        return (xwchar)-1;
    }

    if (X_UNLIKELY (max_len >= 0 && len > max_len)) {
        for (i = 1; i < max_len; i++)
        {
            if ((((xwchar *)p)[i] & 0xc0) != 0x80)
                return (xwchar)-1;
        }
        return (xwchar)-2;
    }

    for (i = 1; i < len; ++i) {
        xwchar ch = ((xuchar *)p)[i];

        if (X_UNLIKELY ((ch & 0xc0) != 0x80)) {
            if (ch)
                return (xwchar)-1;
            else
                return (xwchar)-2;
        }

        wc <<= 6;
        wc |= (ch & 0x3f);
    }

    if (X_UNLIKELY (wc < min_code))
        return (xwchar)-1;

    return wc;
}

const xchar*
x_utf8_skip             (void)
{
    return utf8_skip_data;
}

xwchar
x_str_get_wchar         (xcstr          str)
{
    xint i, mask = 0, len;
    xwchar result;
    xuchar c = (xuchar) *str;

    UTF8_COMPUTE (c, mask, len);
    if (len == -1)
        return (xwchar)-1;
    UTF8_GET (result, str, i, mask, len);

    return result;
}
xbool
x_wchar_validate        (xwchar         ch)
{
    return UNICODE_VALID (ch);
}

xwchar
x_str_get_wchar_s       (xcstr          str,
                         xssize         max_len)
{
    xwchar result;

    if (max_len == 0)
        return (xwchar)-2;

    result = get_wchar_extended (str, max_len);

    if (result & 0x80000000)
        return result;
    else if (!UNICODE_VALID (result))
        return (xwchar)-1;
    else
        return result;
}
struct Interval
{
    xwchar start, end;
};
static int
interval_compare (const void *key, const void *elt)
{
    xwchar c = (xwchar)X_PTR_TO_SIZE (key);
    struct Interval *interval = (struct Interval *)elt;

    if (c < interval->start)
        return -1;
    if (c > interval->end)
        return +1;

    return 0;
}
xbool
x_wchar_iswide          (xwchar         c)
{
    /* See NOTE earlier for how to update this table. */
    static const struct Interval wide[] = {
        {0x1100, 0x115F}, {0x11A3, 0x11A7}, {0x11FA, 0x11FF}, {0x2329, 0x232A},
        {0x2E80, 0x2E99}, {0x2E9B, 0x2EF3}, {0x2F00, 0x2FD5}, {0x2FF0, 0x2FFB},
        {0x3000, 0x303E}, {0x3041, 0x3096}, {0x3099, 0x30FF}, {0x3105, 0x312D},
        {0x3131, 0x318E}, {0x3190, 0x31BA}, {0x31C0, 0x31E3}, {0x31F0, 0x321E},
        {0x3220, 0x3247}, {0x3250, 0x32FE}, {0x3300, 0x4DBF}, {0x4E00, 0xA48C},
        {0xA490, 0xA4C6}, {0xA960, 0xA97C}, {0xAC00, 0xD7A3}, {0xD7B0, 0xD7C6},
        {0xD7CB, 0xD7FB}, {0xF900, 0xFAFF}, {0xFE10, 0xFE19}, {0xFE30, 0xFE52},
        {0xFE54, 0xFE66}, {0xFE68, 0xFE6B}, {0xFF01, 0xFF60}, {0xFFE0, 0xFFE6},
        {0x1B000, 0x1B001}, {0x1F200, 0x1F202}, {0x1F210, 0x1F23A},
        {0x1F240, 0x1F248}, {0x1F250, 0x1F251}, {0x20000, 0x2FFFD},
        {0x30000, 0x3FFFD}
    };

    if (bsearch (X_SIZE_TO_PTR (c), wide, X_N_ELEMENTS (wide), sizeof wide[0],
                 interval_compare))
        return TRUE;

    return FALSE;
}

xsize
x_wchar_width           (xwchar         c)
{
    if (X_UNLIKELY (c == 0x00AD))
        return 1;

    //    if (X_UNLIKELY (ISZEROWIDTHTYPE (TYPE (c))))
    //        return 0;

    if (X_UNLIKELY ((c >= 0x1160 && c < 0x1200) ||
                    c == 0x200B))
        return 0;

    if (x_wchar_iswide (c))
        return 2;

    return 1;
}

xsize
x_str_width             (xcstr          p)
{
    xsize len = 0;
    x_return_val_if_fail (p != NULL, 0);

    while (*p) {
        len += x_wchar_width (x_str_get_wchar (p));
        p = x_str_next_wchar (p);
    }

    return len;
}

xsize
x_wchar_to_utf8         (xwchar         c,
                         xstr           outbuf)
{
    int len = 0;    
    int first;
    int i;

    if (c < 0x80) {
        first = 0;
        len = 1;
    }
    else if (c < 0x800) {
        first = 0xc0;
        len = 2;
    }
    else if (c < 0x10000) {
        first = 0xe0;
        len = 3;
    }
    else if (c < 0x200000) {
        first = 0xf0;
        len = 4;
    }
    else if (c < 0x4000000) {
        first = 0xf8;
        len = 5;
    }
    else {
        first = 0xfc;
        len = 6;
    }

    if (outbuf) {
        for (i = len - 1; i > 0; --i)
        {
            outbuf[i] = (c & 0x3f) | 0x80;
            c >>= 6;
        }
        outbuf[0] = c | first;
    }

    return len;
}

#define CONTINUATION_CHAR                                   \
    X_STMT_START {                                          \
        if ((*(xbyte *)p & 0xc0) != 0x80) /* 10xxxxxx */   \
        goto error;                                         \
        val <<= 6;                                          \
        val |= (*(xbyte *)p) & 0x3f;                       \
    } X_STMT_END

static const xchar *
fast_validate (const xchar *str)
{
    xwchar val = 0;
    xwchar min = 0;
    const xchar *p;

    for (p = str; *p; p++)
    {
        if (*(xbyte*)p < 128)
            /* done */;
        else 
        {
            const xchar *last;

            last = p;
            if ((*(xbyte *)p & 0xe0) == 0xc0) /* 110xxxxx */
            {
                if (X_UNLIKELY ((*(xbyte *)p & 0x1e) == 0))
                    goto error;
                p++;
                if (X_UNLIKELY ((*(xbyte *)p & 0xc0) != 0x80)) /* 10xxxxxx */
                    goto error;
            }
            else
            {
                if ((*(xbyte *)p & 0xf0) == 0xe0) /* 1110xxxx */
                {
                    min = (1 << 11);
                    val = *(xbyte *)p & 0x0f;
                    goto TWO_REMAINING;
                }
                else if ((*(xbyte *)p & 0xf8) == 0xf0) /* 11110xxx */
                {
                    min = (1 << 16);
                    val = *(xbyte *)p & 0x07;
                }
                else
                    goto error;

                p++;
                CONTINUATION_CHAR;
TWO_REMAINING:
                p++;
                CONTINUATION_CHAR;
                p++;
                CONTINUATION_CHAR;

                if (X_UNLIKELY (val < min))
                    goto error;

                if (X_UNLIKELY (!UNICODE_VALID(val)))
                    goto error;
            } 
            continue;
error:
            return last;
        }
    }
    return p;
}
static const xchar *
fast_validate_len (const xchar *str,
                   xssize      max_len)

{
    xwchar val = 0;
    xwchar min = 0;
    const xchar *p;

    x_assert (max_len >= 0);

    for (p = str; ((p - str) < max_len) && *p; p++)
    {
        if (*(xbyte*)p < 128)
            /* done */;
        else 
        {
            const xchar *last;

            last = p;
            if ((*(xbyte *)p & 0xe0) == 0xc0) /* 110xxxxx */
            {
                if (X_UNLIKELY (max_len - (p - str) < 2))
                    goto error;

                if (X_UNLIKELY ((*(xbyte *)p & 0x1e) == 0))
                    goto error;
                p++;
                if (X_UNLIKELY ((*(xbyte *)p & 0xc0) != 0x80)) /* 10xxxxxx */
                    goto error;
            }
            else
            {
                if ((*(xbyte *)p & 0xf0) == 0xe0) /* 1110xxxx */
                {
                    if (X_UNLIKELY (max_len - (p - str) < 3))
                        goto error;

                    min = (1 << 11);
                    val = *(xbyte *)p & 0x0f;
                    goto TWO_REMAINING;
                }
                else if ((*(xbyte *)p & 0xf8) == 0xf0) /* 11110xxx */
                {
                    if (X_UNLIKELY (max_len - (p - str) < 4))
                        goto error;

                    min = (1 << 16);
                    val = *(xbyte *)p & 0x07;
                }
                else
                    goto error;

                p++;
                CONTINUATION_CHAR;
TWO_REMAINING:
                p++;
                CONTINUATION_CHAR;
                p++;
                CONTINUATION_CHAR;

                if (X_UNLIKELY (val < min))
                    goto error;
                if (X_UNLIKELY (!UNICODE_VALID(val)))
                    goto error;
            } 

            continue;

error:
            return last;
        }
    }
    return p;
}

xbool
x_str_validate          (xcstr          str,
                         xssize         max_len,
                         xcstr          *end)
{
    const xchar *p;

    if (max_len < 0)
        p = fast_validate (str);
    else
        p = fast_validate_len (str, max_len);

    if (end)
        *end = p;

    if ((max_len >= 0 && p != str + max_len) ||
        (max_len < 0 && *p != '\0'))
        return FALSE;
    else
        return TRUE;
}
