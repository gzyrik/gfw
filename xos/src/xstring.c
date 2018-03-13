#include "config.h"
#include "xstring.h"
#include "xmem.h"
#include "xslice.h"
#include "xmsg.h"
#include "xstr.h"
#include "xwchar.h"
#include "xprintf.h"
#include <ctype.h>
#include <string.h>

#define MY_MAXSIZE ((xsize)-1)

xsize
_x_nearest_power        (xsize          base,
                         xsize          num)
{
    if (num > MY_MAXSIZE / 2) {
        return MY_MAXSIZE;
    }
    else {
        xsize n = MAX(base,1);

        while (n < num)
            n <<= 1;

        return n;
    }
}

static inline void
string_maybe_expand     (xString        *string,
                         xsize          len)
{
    if (string->len + len >= string->allocated_len) {
        string->allocated_len = _x_nearest_power (string->allocated_len,
                                                  string->len + len + 1);
        string->str = x_realloc (string->str, string->allocated_len);
    }
}

xString*
x_string_new            (xcstr          init,
                         xssize         len)
{
    xString *string;

    if (len < 0)
        len = strlen (init);

    string = x_slice_new (xString);
    string->allocated_len = 0;
    string->len   = 0;
    string->str   = NULL;


    if (init != NULL && *init != '\0')
        x_string_insert (string, 0, init, len);
    else {
        string_maybe_expand (string, MAX (len, 2));
        string->str[0] = '\0';
    }

    return string;
}
xstr
x_string_delete         (xString        *string,
                         xbool          free_str)
{
    xstr str = NULL;
    x_return_val_if_fail (string != NULL, NULL);

    if (free_str)
        x_free (string->str);
    else
        str = string->str;

    x_slice_free (xString, string);

    return str;
}
void
x_string_erase          (xString        *string,
                         xsize          pos,
                         xssize         len)
{
    x_return_if_fail (string != NULL);
    x_return_if_fail (pos <= string->len);

    if (len < 0)
        len = string->len - pos;
    else
    {
        x_return_if_fail (pos + len <= string->len);

        if (pos + len < string->len)
            memmove (string->str + pos,
                     string->str + pos + len,
                     string->len - (pos + len));
    }

    string->len -= len;

    string->str[string->len] = 0;
}
void
x_string_insert_char    (xString*       string,
                         xssize         position,
                         xchar          ch)
{
    xsize pos;
    x_return_if_fail (string != NULL);

    string_maybe_expand (string, 1);

    if (position < 0)
        pos = string->len;
    else
        pos = position;

    x_return_if_fail (pos <= string->len);

    /* If not just an append, move the old stuff */
    if (pos < string->len)
        memmove (string->str + pos + 1,
                 string->str + pos,
                 string->len - pos);

    string->str[pos] = ch;
    string->len += 1;
    string->str[string->len] = 0;
}

void
x_string_insert         (xString        *string,
                         xssize         position,
                         xcstr          str,
                         xssize         len)
{
    xsize pos;

    x_return_if_fail (string != NULL);
    x_return_if_fail (len == 0 || str != NULL);

    if (len == 0)
        return;

    if (len < 0)
        len = strlen (str);

    if (position < 0)
        pos = string->len;
    else
        pos = position;

    x_return_if_fail (pos <= string->len);

    /* str可能指向内部为子串 */
    if (str >= string->str && str <= string->str + string->len) {
        xsize offset = str - string->str;
        xsize precount = 0;

        string_maybe_expand (string, len);
        str = string->str + offset;

        if (pos < string->len)
            memmove (string->str + pos + len,
                     string->str + pos,
                     string->len - pos);

        if (offset < pos) {
            precount = MIN (len, (xssize)(pos - offset));
            memcpy (string->str + pos, str, precount);
        }

        /* Move the source part after the gap, if any. */
        if ((xsize)len > precount)
            memcpy (string->str + pos + precount,
                    str + precount + len,
                    len - precount);
    }
    else {
        string_maybe_expand (string, len);

        if (pos < string->len)
            memmove (string->str + pos + len,
                     string->str + pos,
                     string->len - pos);

        if (len == 1)
            string->str[pos] = *str;
        else
            memcpy (string->str + pos, str, len);
    }

    string->len += len;
    string->str[string->len] = 0;
}

#define CHAR_IS_SAFE(wc) (!((wc < 0x20 \
                             && wc != '\t' && wc != '\n' && wc != '\r') || \
                            (wc == 0x7f) || \
                            (wc >= 0x80 && wc < 0xa0)))
void
x_string_escape_invalid (xString        *string)
{
    const xchar *p;
    xwchar      wc;

    x_return_if_fail (string != NULL);

    p = string->str;
    while (p < string->str + string->len) {
        xbool safe;

        wc = x_str_get_wchar_s(p, -1);
        if (wc == (xwchar)-1 || wc == (xwchar)-2)  {
            xchar *tmp;
            xssize pos;

            pos = p - string->str;

            /* Emit invalid UTF-8 as hex escapes 
            */
            tmp = x_strdup_printf ("\\x%02x", (xuint)(xuchar)*p);
            x_string_erase (string, pos, 1);
            x_string_insert (string, pos, tmp, -1);

            p = string->str + (pos + 4); /* Skip over escape sequence */

            x_free (tmp);
            continue;
        }
        if (wc == '\r') {
            safe = *(p + 1) == '\n';
        }
        else {
            safe = CHAR_IS_SAFE (wc);
        }

        if (!safe) {
            xchar *tmp;
            xssize pos;

            pos = p - string->str;

            /* Largest char we escape is 0x0a, so we don't have to worry
             * about 8-digit \Uxxxxyyyy
             */
            tmp = x_strdup_printf ("\\u%04x", wc); 
            x_string_erase (string, pos, x_str_next_wchar (p) - p);
            x_string_insert (string, pos, tmp, -1);
            x_free (tmp);

            p = string->str + (pos + 6); /* Skip over escape sequence */
        }
        else
            p = x_str_next_wchar (p);
    }
}
void
x_string_append_printf  (xString        *string,
                         xcstr          format,
                         ...)
{
    va_list args;

    va_start (args, format);
    x_string_append_vprintf(string, format, args);
    va_end (args);
}
void
x_string_append_vprintf (xString        *string,
                         xcstr          format,
                         va_list        args)
{
    xchar *buf;
    xint len;

    x_return_if_fail (string != NULL);
    x_return_if_fail (format != NULL);

    len = x_vasprintf (&buf, format, args);

    if (len >= 0) {
        string_maybe_expand (string, len);
        memcpy (string->str + string->len, buf, len + 1);
        string->len += len;
        x_free (buf);
    }
}
static inline xbool
wchar_ok (xwchar c)
{
    return
        (c != (xwchar) -2) &&
        (c != (xwchar) -1);
}
static xbool
is_valid (char        c,
          const char *reserved_chars_allowed)
{
    if (isalnum (c) ||
        c == '-' ||
        c == '.' ||
        c == '_' ||
        c == '~')
        return TRUE;

    if (reserved_chars_allowed &&
        strchr (reserved_chars_allowed, c) != NULL)
        return TRUE;

    return FALSE;
}
void
x_string_append_uri_escaped (xString    *string,
                             xcstr      unescaped,
                             xcstr      reserved_chars_allowed,
                             xbool      allow_utf8)
{
    unsigned char c;
    const xchar *end;
    static const xchar hex[16] = "0123456789ABCDEF";

    x_return_if_fail (string != NULL);
    x_return_if_fail (unescaped != NULL);

    end = unescaped + strlen (unescaped);

    while ((c = *unescaped) != 0) {
        if (c >= 0x80 && allow_utf8 &&
            wchar_ok (x_str_get_wchar_s (unescaped, end - unescaped)))
        {
            int len = x_utf8_skip() [c];
            x_string_append_len (string, unescaped, len);
            unescaped += len;
        }
        else if (is_valid (c, reserved_chars_allowed)) {
            x_string_append_char (string, c);
            unescaped++;
        }
        else {
            x_string_append_char (string, '%');
            x_string_append_char (string, hex[((xuchar)c) >> 4]);
            x_string_append_char (string, hex[((xuchar)c) & 0xf]);
            unescaped++;
        }
    }
}
