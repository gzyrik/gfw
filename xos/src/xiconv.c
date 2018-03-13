#include "config.h"
#include "xstr.h"
#include "xerror.h"
#include "xiconv.h"
#include "xmsg.h"
#include "xquark.h"
#include "xatomic.h"
#include "xmem.h"
#include "xwchar.h"
#include "xhash.h"
#include "xlist.h"
#include "xtest.h"
#include "xslice.h"
#include "xthread.h"
#include "xutil.h"
#include <errno.h>
#include <string.h>

#if HAVE_ICONV_H
#include <iconv.h>
#elif HAVE_WIN_ICONV_H
#include "win_iconv.h"
#else
typedef void* iconv_t;
iconv_t iconv_open (const char* tocode, const char* fromcode) { return NULL; }
int iconv_close (iconv_t cd) { return 0; }
size_t iconv (iconv_t cd, const char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft) { return 0; }
#endif

xQuark
x_convert_domain        (void)
{
    static xQuark quark = 0;
    if (!x_atomic_ptr_get(&quark) && x_once_init_enter(&quark)) {
        quark = x_quark_from_static("xConvert");
        x_once_init_leave (&quark);
    }
    return quark;
}


X_INTERN_FUNC xcstr* 
_x_charset_get_aliases  (xcstr          canonical_name)
{
    return NULL;
}

static xbool
try_conversion          (xcstr          to_codeset,
                         xcstr          from_codeset,
                         iconv_t        *cd)
{
    *cd = iconv_open (to_codeset, from_codeset);

    if (*cd == (iconv_t)-1 && errno == EINVAL)
        return FALSE;
    else
        return TRUE;
}

static xbool
try_to_aliases          (xcstr          *to_aliases,
                         xcstr          from_codeset,
                         iconv_t     *cd)
{
    if (to_aliases) {
        xcstr   *p = to_aliases;
        while (*p) {
            if (try_conversion (*p, from_codeset, cd))
                return TRUE;

            p++;
        }
    }

    return FALSE;
}


xIConv*
x_iconv_open            (xcstr          to_codeset,
                         xcstr          from_codeset)
{
    iconv_t cd;

    if (!try_conversion (to_codeset, from_codeset, &cd))
    {
        xcstr   *to_aliases = _x_charset_get_aliases (to_codeset);
        xcstr   *from_aliases = _x_charset_get_aliases (from_codeset);

        if (from_aliases) {
            xcstr   *p = from_aliases;
            while (*p) {
                if (try_conversion (to_codeset, *p, &cd))
                    goto out;

                if (try_to_aliases (to_aliases, *p, &cd))
                    goto out;

                p++;
            }
        }

        if (try_to_aliases (to_aliases, from_codeset, &cd))
            goto out;
    }

out:
    return (cd == (iconv_t)-1) ? (xIConv*)NULL : (xIConv*)cd;
}

xsize
x_iconv                 (xIConv         *converter,
                         xcptr          inbuf,
                         xsize          *inbytes_left,
                         xptr           outbuf,
                         xsize          *outbytes_left)
{
    iconv_t cd = (iconv_t)converter;
    x_return_val_if_fail (converter != NULL, -1);

    return iconv (cd, (xptr)inbuf, (size_t*)inbytes_left, outbuf, (size_t*)outbytes_left);
}

void
x_iconv_close           (xIConv         *converter)
{
    iconv_t cd = (iconv_t)converter;
    x_return_if_fail (converter != NULL);

    iconv_close (cd);
}
typedef struct {
    xstr        key;
    xint        ref_count;
    xbool       used;
    xIConv      *cd;
} iconv_bucket;
#define NUL_TERMINATOR_LENGTH   4
#define ICONV_CACHE_SIZE        16
X_LOCK_DEFINE_STATIC (iconv_cache);
static xList        *_iconv_cache_list;
static xHashTable   *_iconv_cache;
static xHashTable   *_iconv_open_hash;
static xsize        _iconv_cache_size = 0;
static void inline
iconv_cache_init        (void)
{
    if (!x_atomic_ptr_get (&_iconv_cache)
        && x_once_init_enter (&_iconv_cache) ){
        _iconv_cache_list = NULL;
        _iconv_open_hash = x_hash_table_new (x_direct_hash, x_direct_cmp);
        _iconv_cache = x_hash_table_new (x_str_hash, x_str_cmp);
        x_once_init_leave (&_iconv_cache);
    }
}
static iconv_bucket*
iconv_bucket_new        (xstr           key,
                         xIConv         *cd)
{
    iconv_bucket *bucket;
    bucket = x_new (iconv_bucket, 1);
    bucket->key = key;
    bucket->ref_count = 1;
    bucket->used = TRUE;
    bucket->cd = cd;

    x_hash_table_insert (_iconv_cache, bucket->key, bucket);

    /* FIXME: if we sorted the list so items with few refcounts were
       first, then we could expire them faster in iconv_cache_expire_unused () */
    _iconv_cache_list = x_list_prepend (_iconv_cache_list, bucket);

    _iconv_cache_size++;

    return bucket;
}
static void
iconv_bucket_expire     (xList          *node,
                         iconv_bucket   *bucket)
{
    x_hash_table_remove (_iconv_cache, bucket->key);

    if (node == NULL)
        node = x_list_find (_iconv_cache_list, bucket);

    x_assert (node != NULL);

    if (node->prev) {
        node->prev->next = node->next;
        if (node->next)
            node->next->prev = node->prev;
    }
    else {
        _iconv_cache_list = node->next;
        if (node->next)
            node->next->prev = NULL;
    }

    x_list_free1 (node);

    x_free (bucket->key);
    x_iconv_close (bucket->cd);
    x_free (bucket);

    _iconv_cache_size--;
}
static void
iconv_cache_expire      (void)
{
    iconv_bucket *bucket;
    xList *node, *next;

    node = _iconv_cache_list;
    while (node && _iconv_cache_size >= ICONV_CACHE_SIZE) {
        next = node->next;

        bucket = node->data;
        if (bucket->ref_count == 0)
            iconv_bucket_expire (node, bucket);

        node = next;
    }
}

static xIConv*
open_converter          (xcstr          to_codeset,
                         xcstr          from_codeset,
                         xError         **error)
{
    iconv_bucket *bucket;
    xchar *key, *dyn_key, auto_key[80];
    xIConv *cd;
    xsize len_from_codeset, len_to_codeset;

    /* create our key */
    len_from_codeset = strlen (from_codeset);
    len_to_codeset = strlen (to_codeset);
    if (len_from_codeset + len_to_codeset + 2 < sizeof (auto_key)) {
        key = auto_key;
        dyn_key = NULL;
    }
    else
        key = dyn_key = x_malloc (len_from_codeset + len_to_codeset + 2);
    memcpy (key, from_codeset, len_from_codeset);
    key[len_from_codeset] = ':';
    strcpy (key + len_from_codeset + 1, to_codeset);

    X_LOCK (iconv_cache);

    /* make sure the cache has been initialized */
    iconv_cache_init ();

    bucket = x_hash_table_lookup (_iconv_cache, key);
    if (bucket) {
        x_free (dyn_key);

        if (bucket->used) {
            cd = x_iconv_open (to_codeset, from_codeset);
            if (!cd)
                goto error;
        }
        else {
            /* Apparently iconv on Solaris <= 7 segfaults if you pass in
             * NULL for anything but inbuf; work around that. (NULL outbuf
             * or NULL *outbuf is allowed by Unix98.)
             */
            xsize inbytes_left = 0;
            xchar *outbuf = NULL;
            xsize outbytes_left = 0;

            cd = bucket->cd;
            bucket->used = TRUE;

            /* reset the descriptor */
            x_iconv (cd, NULL, &inbytes_left, &outbuf, &outbytes_left);
        }

        bucket->ref_count++;
    }
    else {
        cd = x_iconv_open (to_codeset, from_codeset);
        if (!cd) {
            x_free (dyn_key);
            goto error;
        }

        iconv_cache_expire ();

        bucket = iconv_bucket_new (dyn_key ? dyn_key : x_strdup (key), cd);
    }

    x_hash_table_insert (_iconv_open_hash, cd, bucket->key);

    X_UNLOCK (iconv_cache);

    return cd;

error:

    X_UNLOCK (iconv_cache);

    /* Something went wrong.  */
    if (error)
    {
        if (errno == EINVAL)
            x_set_error (error, x_convert_domain(), -1,
                         "Conversion from character set '%s' to '%s' is not supported",
                         from_codeset, to_codeset);
        else
            x_set_error (error, x_convert_domain(), -1,
                         "Could not open converter from '%s' to '%s'",
                         from_codeset, to_codeset);
    }

    return cd;
}
static int
close_converter         (xIConv         *converter)
{
    iconv_bucket *bucket;
    xcstr   key;
    xIConv  *cd;

    cd = converter;

    if (!cd)
        return 0;

    X_LOCK (iconv_cache);

    key = x_hash_table_lookup (_iconv_open_hash, cd);
    if (key) {
        x_hash_table_remove (_iconv_open_hash, cd);

        bucket = x_hash_table_lookup (_iconv_cache, key);
        x_assert (bucket);

        bucket->ref_count--;

        if (cd == bucket->cd)
            bucket->used = FALSE;
        else
            x_iconv_close (cd);

        if (!bucket->ref_count && _iconv_cache_size > ICONV_CACHE_SIZE)
        {
            /* expire this cache bucket */
            iconv_bucket_expire (NULL, bucket);
        }
        X_UNLOCK (iconv_cache);
    }
    else
    {
        X_UNLOCK (iconv_cache);

        x_warning ("This iconv context wasn't opened using open_converter");

        x_iconv_close (converter);
    }

    return 0;
}

xstr
x_convert               (xcptr          str,
                         xssize         len,            
                         xcstr          to_codeset,
                         xcstr          from_codeset,
                         xsize          *bytes_read,     
                         xsize          *bytes_written,  
                         xError         **error)
{
    xchar *res;
    xIConv *cd;

    x_return_val_if_fail (str != NULL, NULL);
    x_return_val_if_fail (to_codeset != NULL, NULL);
    x_return_val_if_fail (from_codeset != NULL, NULL);

    cd = open_converter (to_codeset, from_codeset, error);

    if (!cd) {
        if (bytes_read)
            *bytes_read = 0;

        if (bytes_written)
            *bytes_written = 0;

        return NULL;
    }

    res = x_convert_with_iconv (str, len, cd,
                                bytes_read, bytes_written,
                                error);

    close_converter (cd);

    return res;
}
xstr
x_convert_with_iconv    (xcptr          str,
                         xssize         len,
                         xIConv         *converter,
                         xsize          *bytes_read,     
                         xsize          *bytes_written,  
                         xError         **error)
{
    xchar *dest;
    xchar *outp;
    const xchar *p;
    xsize inbytes_remaining;
    xsize outbytes_remaining;
    xsize err;
    xsize outbuf_size;
    xbool have_error = FALSE;
    xbool done = FALSE;
    xbool reset = FALSE;

    x_return_val_if_fail (converter != NULL, NULL);

    if (len < 0)
        len = strlen (str);

    p = str;
    inbytes_remaining = len;
    outbuf_size = len + NUL_TERMINATOR_LENGTH;

    outbytes_remaining = outbuf_size - NUL_TERMINATOR_LENGTH;
    outp = dest = x_malloc (outbuf_size);

    while (!done && !have_error) {
        if (reset)
            err = x_iconv (converter, NULL, &inbytes_remaining, &outp, &outbytes_remaining);
        else
            err = x_iconv (converter, (char **)&p, &inbytes_remaining, &outp, &outbytes_remaining);

        if (err == (xsize) -1) {
            switch (errno) {
            case EINVAL:
                /* Incomplete text, do not report an error */
                done = TRUE;
                break;
            case E2BIG:
                {
                    xsize used = outp - dest;

                    outbuf_size *= 2;
                    dest = x_realloc (dest, outbuf_size);

                    outp = dest + used;
                    outbytes_remaining = outbuf_size - used - NUL_TERMINATOR_LENGTH;
                }
                break;
            case EILSEQ:
                x_set_error (error, x_convert_domain(), -1,
                             "Invalid byte sequence in conversion input");
                have_error = TRUE;
                break;
            default:
                {
                    int errsv = errno;

                    x_set_error (error, x_convert_domain(), -1,
                                 "Error during conversion: %s",
                                 strerror (errsv));
                }
                have_error = TRUE;
                break;
            }
        }
        else {
            if (!reset) {
                /* call x_iconv with NULL inbuf to cleanup shift state */
                reset = TRUE;
                inbytes_remaining = 0;
            }
            else
                done = TRUE;
        }
    }

    memset (outp, 0, NUL_TERMINATOR_LENGTH);

    if (bytes_read)
        *bytes_read = p - (const xchar *)str;
    else {
        if ((p - (const xchar *)str) != len) {
            if (!have_error) {
                x_set_error (error, x_convert_domain(), -1,
                             "Partial character sequence at end of input");
                have_error = TRUE;
            }
        }
    }

    if (bytes_written)
        *bytes_written = outp - dest;	/* Doesn't include '\0' */

    if (have_error) {
        x_free (dest);
        return NULL;
    }
    else
        return dest;
}
xutf16
x_str_to_utf16          (xcstr          str,
                         xssize         len,
                         xsize          *items_read,
                         xsize          *items_written,
                         xError         **error)
{
    xuint16 *result = NULL;
    xint n16;
    const xchar *in;
    xint i;

    x_return_val_if_fail (str != NULL, NULL);

    in = str;
    n16 = 0;
    while ((len < 0 || str + len - in > 0) && *in) {
        xwchar wc = x_str_get_wchar_s (in, len < 0 ? 6 : str + len - in);
        if (wc & 0x80000000) {
            if (wc == (xwchar)-2)
            {
                if (items_read)
                    break;
                else
                    x_set_error (error, x_convert_domain(), -1,
                                 "Partial character sequence at end of input");
            }
            else
                x_set_error (error, x_convert_domain(), -1,
                             "Invalid byte sequence in conversion input");

            goto err_out;
        }

        if (wc < 0xd800)
            n16 += 1;
        else if (wc < 0xe000) {
            x_set_error (error, x_convert_domain(), -1,
                         "Invalid sequence in conversion input");

            goto err_out;
        }
        else if (wc < 0x10000)
            n16 += 1;
        else if (wc < 0x110000)
            n16 += 2;
        else
        {
            x_set_error (error, x_convert_domain(), -1,
                         "Character out of range for UTF-16");

            goto err_out;
        }

        in = x_str_next_wchar (in);
    }

    result = x_new (xuint16, n16 + 1);

    in = str;
    for (i = 0; i < n16;) {
        xwchar wc = x_str_get_wchar (in);

        if (wc < 0x10000) {
            result[i++] = wc;
        }
        else {
            result[i++] = (wc - 0x10000) / 0x400 + 0xd800;
            result[i++] = (wc - 0x10000) % 0x400 + 0xdc00;
        }

        in = x_str_next_wchar (in);
    }

    result[i] = 0;

    if (items_written)
        *items_written = n16;

err_out:
    if (items_read)
        *items_read = in - str;

    return result;
}

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

xstr
x_utf16_to_str          (xcutf16        str,
                         xssize         len,
                         xsize          *bytes_read,
                         xsize          *items_written,
                         xError         **error)
{
    const xuint16 *in;
    xchar *out;
    xchar *result = NULL;
    xint n_bytes;
    xwchar high_surrogate;

    x_return_val_if_fail (str != NULL, NULL);

    n_bytes = 0;
    in = str;
    high_surrogate = 0;
    while ((len < 0 || in - str < len) && *in) {
        xuint16 c = *in;
        xwchar wc;

        if (c >= 0xdc00 && c < 0xe000) {/* low surrogate */
            if (high_surrogate) {
                wc = SURROGATE_VALUE (high_surrogate, c);
                high_surrogate = 0;
            }
            else {
                x_set_error (error, x_convert_domain(), -1,
                             "Invalid sequence in conversion input");
                goto err_out;
            }
        }
        else {
            if (high_surrogate) {
                x_set_error (error, x_convert_domain(), -1,
                             "Invalid sequence in conversion input");
                goto err_out;
            }

            if (c >= 0xd800 && c < 0xdc00) {/* high surrogate */
                high_surrogate = c;
                goto next1;
            }
            else
                wc = c;
        }

        /********** DIFFERENT for UTF8/UCS4 **********/
        n_bytes += x_wchar_utf8_len (wc);

next1:
        in++;
    }

    if (high_surrogate && !bytes_read) {
        x_set_error (error, x_convert_domain(), -1,
                     "Partial character sequence at end of input");
        goto err_out;
    }

    /* At this point, everything is valid, and we just need to convert
    */
    /********** DIFFERENT for UTF8/UCS4 **********/
    result = x_malloc (n_bytes + 1);

    high_surrogate = 0;
    out = result;
    in = str;
    while (out < result + n_bytes) {
        xuint16 c = *in;
        xwchar wc;

        if (c >= 0xdc00 && c < 0xe000) { /* low surrogate */
            wc = SURROGATE_VALUE (high_surrogate, c);
            high_surrogate = 0;
        }
        else if (c >= 0xd800 && c < 0xdc00) { /* high surrogate */
            high_surrogate = c;
            goto next2;
        }
        else
            wc = c;

        /********** DIFFERENT for UTF8/UCS4 **********/
        out += x_wchar_to_utf8 (wc, out);

next2:
        in++;
    }

    /********** DIFFERENT for UTF8/UCS4 **********/
    *out = '\0';

    if (items_written)
        /********** DIFFERENT for UTF8/UCS4 **********/
        *items_written = out - result;

err_out:
    if (bytes_read)
        *bytes_read = (in - str)*sizeof(xuint16);

    return result;
}
static xstr
strdup_len              (xcstr          string,
                         xssize         len,
                         xsize          *bytes_written,
                         xsize          *bytes_read,
                         xError         **error)
{
    xsize real_len;

    if (!x_str_validate (string, len, NULL)) {
        if (bytes_read)
            *bytes_read = 0;
        if (bytes_written)
            *bytes_written = 0;

        x_set_error (error, x_convert_domain(), -1,
                     "Invalid byte sequence in conversion input");
        return NULL;
    }

    if (len < 0)
        real_len = strlen (string);
    else {
        real_len = 0;

        while (real_len < (xsize)len && string[real_len])
            real_len++;
    }

    if (bytes_read)
        *bytes_read = real_len;
    if (bytes_written)
        *bytes_written = real_len;

    return x_strndup (string, real_len);
}

xptr
x_str_to_locale         (xcstr          str,
                         xssize         len,
                         xsize          *bytes_read,
                         xsize          *items_written,
                         xError         **error)
{
    xcstr charset = x_get_charset ();

    if (!charset) /* default is utf8 */
        return strdup_len (str, len, bytes_read, items_written, error);
    else
        return x_convert (str, len,
                          charset, "UTF-8", bytes_read, items_written, error);
}

xstr
x_locale_to_str         (xcptr          str,
                         xssize         len,
                         xsize          *bytes_read,
                         xsize          *items_written,
                         xError         **error)
{
    xcstr charset = x_get_charset();

    if (!charset) /* default is utf8 */
        return strdup_len (str, len, bytes_read, items_written, error);
    else
        return x_convert (str, len, 
                          "UTF-8", charset, bytes_read, items_written, error);
}
