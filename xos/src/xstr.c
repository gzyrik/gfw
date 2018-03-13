#include "config.h"
#include "xstr.h"
#include "xmem.h"
#include "xmsg.h"
#include "xprintf.h"
#include "xslist.h"
#include "xslice.h"
#include "xatomic.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <locale.h>
#include <errno.h>
#ifdef HAVE_XLOCALE_H
/* Needed on BSD/OS X for e.g. strtod_l */
#include <xlocale.h>
#define HAVE_XLOCALE           1
#endif

/* For tolower() */
#define ISSPACE(c)              ((c) == ' ' || (c) == '\f' || (c) == '\n' || \
                                 (c) == '\r' || (c) == '\t' || (c) == '\v')
#define ISUPPER(c)              ((c) >= 'A' && (c) <= 'Z')
#define ISLOWER(c)              ((c) >= 'a' && (c) <= 'z')
#define ISALPHA(c)              (ISUPPER (c) || ISLOWER (c))
#define TOUPPER(c)              (ISLOWER (c) ? (c) - 'a' + 'A' : (c))
#define TOLOWER(c)              (ISUPPER (c) ? (c) - 'A' + 'a' : (c))

#ifdef HAVE_XLOCALE
static locale_t
get_C_locale            (void)
{
    static locale_t C_locale = NULL;

    if (!x_atomic_ptr_get(&C_locale) && x_once_init_enter(&C_locale)) {
        C_locale = newlocale (LC_ALL_MASK, "C", NULL);
        x_once_init_leave (&C_locale);
    }

    return C_locale;
}
#else
static xuint64
parse_long_long         (xcstr          nptr,
                         xcstr          *endptr,
                         xuint          base,
                         xbool          *negative)
{
    /* this code is based on on the strtol(3) code from GNU libc released under
     * the GNU Lesser General Public License.
     *
     * Copyright (C) 1991,92,94,95,96,97,98,99,2000,01,02
     *        Free Software Foundation, Inc.
     */
    xbool overflow;
    xuint64 cutoff;
    xuint64 cutlim;
    xuint64 ui64;
    xcstr s, save;
    xuchar c;

    x_return_val_if_fail (nptr != NULL, 0);

    *negative = FALSE;
    if (base == 1 || base > 36) {
        errno = EINVAL;
        if (endptr)
            *endptr = nptr;
        return 0;
    }

    save = s = nptr;

    /* Skip white space.  */
    while (ISSPACE (*s))
        ++s;

    if (X_UNLIKELY (!*s))
        goto noconv;

    /* Check for a sign.  */
    if (*s == '-') {
        *negative = TRUE;
        ++s;
    }
    else if (*s == '+')
        ++s;

    /* Recognize number prefix and if BASE is zero, figure it out ourselves.  */
    if (*s == '0') {
        if ((base == 0 || base == 16) && TOUPPER (s[1]) == 'X') {
            s += 2;
            base = 16;
        }
        else if (base == 0)
            base = 8;
    }
    else if (base == 0)
        base = 10;

    /* Save the pointer so we can check later if anything happened.  */
    save = s;
    cutoff = X_MAXUINT64 / base;
    cutlim = X_MAXUINT64 % base;

    overflow = FALSE;
    ui64 = 0;
    c = *s;
    for (; c; c = *++s) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (ISALPHA (c))
            c = TOUPPER (c) - 'A' + 10;
        else
            break;
        if (c >= base)
            break;
        /* Check for overflow.  */
        if (ui64 > cutoff || (ui64 == cutoff && c > cutlim))
            overflow = TRUE;
        else {
            ui64 *= base;
            ui64 += c;
        }
    }

    /* Check if anything actually happened.  */
    if (s == save)
        goto noconv;

    /* Store in ENDPTR the address of one character
       past the last character we converted.  */
    if (endptr)
        *endptr = s;

    if (X_UNLIKELY (overflow)) {
        errno = ERANGE;
        return X_MAXUINT64;
    }

    return ui64;

noconv:
    /* We must handle a special case here: the base is 0 or 16 and the
       first two characters are '0' and 'x', but the rest are no
       hexadecimal digits.  This is no error case.  We return 0 and
       ENDPTR points to the `x`.  */
    if (endptr) {
        if (save - nptr >= 2 && TOUPPER (save[-1]) == 'X'
            && save[-2] == '0')
            *endptr = &save[-1];
        else
            /*  There was no number to convert.  */
            *endptr = nptr;
    }
    return 0;
}
#endif /* !USE_XLOCALE */

xstr
x_stpcpy                (xstr           dest,
                         xcstr          src)
{
#ifdef HAVE_STPCPY
    x_return_val_if_fail (dest != NULL, NULL);
    x_return_val_if_fail (src != NULL, NULL);
    return stpcpy (dest, src);
#else
    register xchar *d = dest;
    register const xchar *s = src;

    x_return_val_if_fail (dest != NULL, NULL);
    x_return_val_if_fail (src != NULL, NULL);
    do{
        *d++ = *s;
    }while (*s++ != '\0');

    return d - 1;
#endif
}
xint
x_strcmp                (xcstr          str1,
                         xcstr          str2)
{
    if (!str1)
        return -(str1 != str2);
    if (!str2)
        return str1 != str2;
    return strcmp (str1, str2);
}
xstr
x_strndup               (xcstr          str,
                         xssize          length)
{
    xstr new_str;
    if (!str) str = "";

    if (length == -1)
        length = strlen (str);
    ++length;
    new_str = x_new (xchar, length);
    memcpy (new_str, str, length);
    new_str[length-1] = '\0';

    return new_str;
}

xstr
x_strdup_vprintf        (xcstr          format,
                         va_list        args)
{
    xstr str = NULL;

    x_vasprintf (&str, format, args);

    return str;
}

xstr
x_strdup_printf         (xcstr          format,
                         ...)
{
    xstr buffer;
    va_list args;

    va_start (args, format);
    buffer = x_strdup_vprintf (format, args);
    va_end (args);

    return buffer;
}

xstr
x_strcat                (xcstr          string1,
                         ...)
{
    xstr ret;
    va_list args;
    if (!string1)
        return NULL;

    va_start (args, string1);
    ret = x_strcatv(string1, args);
    va_end (args);

    return ret;
}
xstr
x_strcatv               (xcstr          string1,
                         va_list        args)
{
    va_list args2;
    xsize l;
    xcstr s;
    xstr concat, ptr;

    if (!string1)
        return NULL;

    l = 1 + strlen (string1);
    va_copy(args2, args);
    s = va_arg (args2, xcstr);
    while (s) {
        l += strlen (s);
        s = va_arg (args2, xcstr);
    }
    va_end (args2);

    concat = x_new (xchar, l);
    ptr = concat;

    ptr = x_stpcpy (ptr, string1);
    s = va_arg (args, xcstr);
    while (s) {
        ptr = x_stpcpy (ptr, s);
        s = va_arg (args, xcstr);
    }

    return concat;
}
xstr
x_stresc                (xcstr          string1,
                         ...)
{
    return NULL;
}
xstr
x_strchug               (xstr           str)
{
    xuchar *start;

    x_return_val_if_fail (str, NULL);

    for (start = (xuchar*) str;
         *start && isspace (*start);
         start++)
        ;

    memmove (str, start, strlen ((xchar *) start) + 1);

    return str;
}

xstr
x_strchomp              (xstr           str)
{
    xsize len;

    x_return_val_if_fail (str, NULL);

    len = strlen (str);
    while (len--) {
        if (isspace ((xuchar) str[len]))
            str[len] = '\0';
        else
            break;
    }

    return str;
}

xstr
x_str_suffix            (xcstr          str,
                         xcstr          suffix)
{
    xsize str_len;
    xsize suffix_len;

    x_return_val_if_fail (str != NULL, NULL);
    x_return_val_if_fail (suffix != NULL, NULL);

    str_len = strlen (str);
    suffix_len = strlen (suffix);

    if (str_len < suffix_len)
        return NULL;

    str += str_len - suffix_len;
    if (strcmp (str, suffix) == 0)
        return (xstr)str;
    return NULL;
}

xstr
x_str_prefix            (xcstr          str,
                         xcstr          prefix)
{
    return NULL;
}
xstr
x_str_ntoupper           (xcstr          str,
                          xssize         length)
{
    xstr new_str;

    if (str) {
        xssize i;
        if (length == -1)
            length = strlen (str);
        ++length;
        new_str = x_new (xchar, length);
        for (i=0; i<length; ++i)
            new_str[i] = toupper(str[i]);
        new_str[length-1] = '\0';
    }
    else
        new_str = NULL;

    return new_str;
}
xstr
x_strdelimit            (xstr           str,
                         xcstr          delimiters,
                         xchar	        new_delim)
{
    xchar *c;

    x_return_val_if_fail (str != NULL, NULL);

    if (!delimiters)
        delimiters = "_-|> <.";

    for (c = str; *c; c++) {
        if (strchr (delimiters, *c))
            *c = new_delim;
    }

    return str;
}
xstrv
x_strsplit              (xcstr          str,
                         xcstr          delimiter,
                         xssize         max_tokens)
{
    xSList  *string_list = NULL, *slist;
    xstrv  str_array;
    xstr   s;
    xuint   n = 0;
    xcstr   remainder;

    x_return_val_if_fail (str != NULL, NULL);

    if (max_tokens < 1)
        max_tokens = X_MAXINT;

    remainder = str;
    s = strstr (remainder, delimiter);
    if (s) {
        xsize delimiter_len = strlen (delimiter);

        while (--max_tokens && s) {
            xsize len;

            len = s - remainder;
            string_list = x_slist_prepend (string_list,
                                           x_strndup (remainder, len));
            n++;
            remainder = s + delimiter_len;
            s = strstr (remainder, delimiter);
        }
    }
    if (*str) {
        n++;
        string_list = x_slist_prepend (string_list, x_strdup (remainder));
    }

    str_array = x_new (xstr, n + 1);

    str_array[n--] = NULL;
    for (slist = string_list; slist; slist = slist->next)
        str_array[n--] = slist->data;

    x_slist_free (string_list);

    return str_array;
}

xint
x_strv_cmp              (xcstrv         strv0,
                         xcstrv         strv1)
{
    xint ret;

    if (!strv0)
        return -(strv0 != strv1);
    if (!strv1)
        return strv0 != strv1;

    while (!(ret = x_strcmp(*strv0, *strv1))) {
        if (*strv0) ++strv0;
        if (*strv1) ++strv1;
    }
    return 0;
}
void
x_strv_free             (xstrv          str_array)
{
    if (str_array) {
        int i;

        for (i = 0; str_array[i] != NULL; i++)
            x_free (str_array[i]);

        x_free (str_array);
    }
}

xint
x_strv_find             (xcstrv         str_array,
                         xcstr          str)
{
    if (str_array) {
        xint i;
        for(i=0; str_array[i]; ++i) {
            if (strcmp(str_array[i], str)==0)
                return i;
        }
    }
    return -1;
}
xstrv
x_strv_new              (xcstr          str,
                         ...)
{
    xstrv ret;
    va_list args;

    va_start (args, str);
    ret = x_strv_newv (str, args);
    va_end (args);
    return ret;
}
xstrv
x_strv_newv             (xcstr          string1,
                         va_list        args)
{
    va_list args2;
    xsize   l;
    xcstr s;
    xstr    *retval;

    if (!string1)
        return NULL;

    l = 1;
    va_copy(args2, args);
    s = va_arg (args2, xcstr);
    while (s) {
        ++l;
        s = va_arg (args2, xcstr);
    }
    va_end (args2);

    retval = x_new (xstr, l + 1);

    retval[0] = x_strdup (string1);
    l = 1;
    s = va_arg (args, xcstr);
    while (s) {
        retval[l] = x_strdup (s);
        ++l;
        s = va_arg (args, xcstr);
    }
    retval[l] = NULL;

    return retval;
}
xstrv
x_strv_dup              (xcstrv         str_array)
{
    xint i;
    xstr *retval;

    if (!str_array)
        return NULL;
    i = 0;
    while (str_array[i])
        ++i;

    retval = x_new (xstr, i + 1);

    i = 0;
    while (str_array[i]) {
        retval[i] = x_strdup (str_array[i]);
        ++i;
    }
    retval[i] = NULL;

    return retval;
}

xstr
x_casefold              (xcstr          str,
                         xssize         len)
{
    return NULL;
}


xdouble
x_strtod                (xcstr          nptr,
                         xcstr          *endptr)
{
#ifdef HAVE_XLOCALE
    return strtod_l (nptr, (char**)endptr, get_C_locale());
#else
    x_assert(0);
    return 0;
#endif
}

xint64
x_strtoll               (xcstr          nptr,
                         xcstr          *endptr,
                         xint           base)
{
#ifdef HAVE_XLOCALE
    return strtoll_l (nptr, (char**)endptr, base, get_C_locale ());
#else
    xbool negative;
    xuint64 result;

    result = parse_long_long (nptr, endptr, base, &negative);

    if (negative && result > (xuint64) X_MININT64) {
        errno = ERANGE;
        return X_MININT64;
    }
    else if (!negative && result > (xuint64) X_MAXINT64) {
        errno = ERANGE;
        return X_MAXINT64;
    }
    else if (negative)
        return - (xint64) result;
    else
        return (xint64) result;
#endif
}

int
x_xdigit_value          (xchar          c)
{
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return x_digit_value (c);
}

int
x_digit_value           (xchar          c)
{
    if (isdigit (c))
        return c - '0';
    return -1;
}  

xcstr
x_stristr               (xcstr          str,
                         xcstr          sub)
{
    xcstr cp = str;
    xcstr s1, s2;

    x_return_val_if_fail (str != NULL, NULL);
    x_return_val_if_fail (sub != NULL, NULL);

    if ( !*sub )
        return((xcstr)sub);

    while (*cp){
        s1 = cp;
        s2 = sub;

        while ( *s1 && *s2 && !(tolower(*s1)-tolower(*s2)) )
            s1++, s2++;

        if (!*s2)
            return cp;

        cp++;
    }

    return NULL;
}
xcstr
x_strrstr               (xcstr      str,
                         xcstr      sub)
{
    return NULL;
}
