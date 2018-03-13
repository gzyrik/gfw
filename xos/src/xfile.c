#include "config.h"
#include "xfile.h"
#include "xiconv.h"
#include "xmem.h"
#include "xstr.h"
#include "xutil.h"
#include "xmsg.h"
#include "xstring.h"
#include "xerror.h"
#include <string.h>
#include <ctype.h>
#ifdef X_OS_WINDOWS
#include <windows.h>
#include <io.h>
#endif /* X_OS_WIN32 */

xbool
x_path_test             (xcstr          filename,
                         xchar          test)
{
#ifdef X_OS_WINDOWS
#  ifndef INVALID_FILE_ATTRIBUTES
#    define INVALID_FILE_ATTRIBUTES -1
#  endif
    int attributes;
    wchar_t *wfilename = x_str_to_utf16 (filename, -1, NULL, NULL, NULL);

    if (wfilename == NULL)
        return FALSE;

    attributes = GetFileAttributesW (wfilename);

    x_free (wfilename);

    if (attributes == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    if (test == 'e')
        return TRUE;

    if (test == 'f')
        return ((attributes
                 & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE)) == 0);
    if (test == 'd')
        return ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

    if (test == 'x') {
        xstr   lastdot = (xstr) strchr (filename, '.');
        xstr   pathext = NULL;
        xstr   p;
        size_t extlen;

        if (lastdot == NULL)
            return FALSE;

        if (_stricmp (lastdot, ".exe") == 0 ||
            _stricmp (lastdot, ".cmd") == 0 ||
            _stricmp (lastdot, ".bat") == 0 ||
            _stricmp (lastdot, ".com") == 0)
            return TRUE;

        /* Check if it is one of the types listed in %PATHEXT% */
        pathext = x_casefold(x_getenv ("PATHEXT"), -1);
        if (pathext == NULL)
            return FALSE;

        lastdot = x_casefold (lastdot, -1);
        extlen  = strlen (lastdot);

        p = pathext;
        while (TRUE) {
            xstr q = strchr (p, ';');
            if (q == NULL)
                q = p + strlen (p);
            if (extlen == (q - p) 
                && memcmp (lastdot, p, extlen) == 0) {
                x_free (pathext);
                x_free (lastdot);
                return TRUE;
            }
            if (*q)
                p = q + 1;
            else
                break;
        }
        x_free (pathext);
        x_free (lastdot);
        return FALSE;
    }
    return FALSE;
#else
    return FALSE;
#endif /* X_OS_WINDOWS */
}
xstr
x_path_get_basename     (xcstr          path)
{
    xssize base;
    xssize last_nonslash;
    xsize len;
    xstr retval;

    x_return_val_if_fail (path != NULL, NULL);

    if (path[0] == '\0')
        return x_strdup (".");

    last_nonslash = strlen (path) - 1;

    while (last_nonslash >= 0 && X_IS_DIR_SEPARATOR (path [last_nonslash]))
        last_nonslash--;

    if (last_nonslash == -1)
        /* string only containing slashes */
        return x_strdup (X_DIR_SEPARATOR);

#ifdef X_OS_WIN32
    if (last_nonslash == 1 &&isalpha (path[0]) && path[1] == ':')
        /* string only containing slashes and a drive */
        return x_strdup (X_DIR_SEPARATOR);
#endif
    base = last_nonslash;

    while (base >=0 && !X_IS_DIR_SEPARATOR (path [base]))
        base--;

#ifdef X_OS_WIN32
    if (base == -1 && isalpha (path[0]) && path[1] == ':')
        base = 1;
#endif /* X_OS_WIN32 */

    len = last_nonslash - base;
    retval = x_malloc (len + 1);
    memcpy (retval, path + base + 1, len);
    retval [len] = '\0';

    return retval;
}
xbool
x_path_is_absolute      (xcstr          path)
{
    x_return_val_if_fail (path != NULL, FALSE);

    if (X_IS_DIR_SEPARATOR (path[0]))
        return TRUE;

#ifdef X_OS_WIN32
    /* Recognize drive letter on native Windows */
    if (isalpha (path[0]) &&
        path[1] == ':' && X_IS_DIR_SEPARATOR (path[2]))
        return TRUE;
#endif

    return FALSE;
}

xstr
x_path_get_dirname      (xcstr          file_name)
{
    xcstr base;
    xsize len;
    xstr  ret;

    x_return_val_if_fail (file_name != NULL, NULL);

    base = x_strrstr (file_name, X_DIR_SEPARATOR);

#ifdef X_OS_WIN32
    {
        xchar *q;
        q = strrchr (file_name, '/');
        if (base == NULL || (q != NULL && q > base))
            base = q;
    }
#endif

    if (!base) {
#ifdef X_OS_WIN32
        if (isalpha (file_name[0]) && file_name[1] == ':') {
            xchar drive_colon_dot[4];

            drive_colon_dot[0] = file_name[0];
            drive_colon_dot[1] = ':';
            drive_colon_dot[2] = '.';
            drive_colon_dot[3] = '\0';

            return x_strdup (drive_colon_dot);
        }
#endif
        return x_strdup (".");
    }

    while (base > file_name && X_IS_DIR_SEPARATOR (*base))
        base--;

#ifdef X_OS_WIN32
    /* base points to the char before the last slash.
     *
     * In case file_name is the root of a drive (X:\) or a child of the
     * root of a drive (X:\foo), include the slash.
     *
     * In case file_name is the root share of an UNC path
     * (\\server\share), add a slash, returning \\server\share\ .
     *
     * In case file_name is a direct child of a share in an UNC path
     * (\\server\share\foo), include the slash after the share name,
     * returning \\server\share\ .
     */
    if (base == file_name + 1 &&
        isalpha (file_name[0]) &&
        file_name[1] == ':')
        base++;
    else if (X_IS_DIR_SEPARATOR (file_name[0]) &&
             X_IS_DIR_SEPARATOR (file_name[1]) &&
             file_name[2] &&
             !X_IS_DIR_SEPARATOR (file_name[2]) &&
             base >= file_name + 2) {
        const xchar *p = file_name + 2;
        while (*p && !X_IS_DIR_SEPARATOR (*p))
            p++;
        if (p == base + 1) {
            xstr ret;
            len = (xuint) strlen (file_name);
            ret = x_new (xchar, len + strlen(X_DIR_SEPARATOR)+1);
            strncpy (ret, file_name, len);
            strcpy(ret+len, X_DIR_SEPARATOR);
            return ret;
        }
        if (X_IS_DIR_SEPARATOR (*p)) {
            p++;
            while (*p && !X_IS_DIR_SEPARATOR (*p))
                p++;
            if (p == base + 1)
                base++;
        }
    }
#endif

    len = (xuint) 1 + base - file_name;
    ret = x_new (xchar, len + 1);
    strncmp (ret, file_name, len);
    ret[len] = 0;

    return ret;
}
static xstr
build_path_va           (xcstr          separator,
                         xcstr          args[])
{
    xString *result;
    xsize separator_len = strlen (separator);
    xbool is_first = TRUE;
    xbool have_leading = FALSE;
    xcstr single_element = NULL;
    xcstr next_element;
    xcstr last_trailing = NULL;
    xint i = 0;

    result = x_string_new (NULL, 128);

    next_element = args[i++];
    while (TRUE) {
        xcstr element;
        xcstr start;
        xcstr end;

        if (next_element) {
            element = next_element;
            next_element = args[i++];
        }
        else
            break;

        /* Ignore empty elements */
        if (!*element)
            continue;

        start = element;

        if (separator_len) {
            while (strncmp (start, separator, separator_len) == 0)
                start += separator_len;
        }

        end = start + strlen (start);

        if (separator_len) {
            while (end >= start + separator_len &&
                   strncmp (end - separator_len, separator, separator_len) == 0)
                end -= separator_len;

            last_trailing = end;
            while (last_trailing >= element + separator_len &&
                   strncmp (last_trailing - separator_len, separator, separator_len) == 0)
                last_trailing -= separator_len;

            if (!have_leading) {
                /* If the leading and trailing separator strings are in the
                 * same element and overlap, the result is exactly that element
                 */
                if (last_trailing <= start)
                    single_element = element;

                x_string_append_len (result, element, start - element);
                have_leading = TRUE;
            }
            else
                single_element = NULL;
        }

        if (end == start)
            continue;

        if (!is_first)
            x_string_append (result, separator);

        x_string_append_len (result, start, end - start);
        is_first = FALSE;
    }

    if (single_element) {
        x_string_delete (result, TRUE);
        return x_strdup (single_element);
    }
    else {
        if (last_trailing)
            x_string_append (result, last_trailing);

        return x_string_delete (result, FALSE);
    }
}

xstr
x_build_path            (xcstr          separator,
                         ...)
{
    xstr    str;
    va_list argv;

    va_start (argv, separator);
    str = x_build_pathv (separator, argv);
    va_end (argv);

    return str;
}

xstr
x_build_patha           (xcstr          separator,
                         xcstr          args[])
{
    x_return_val_if_fail (args != NULL, NULL);

    if (!separator)
        separator = X_DIR_SEPARATOR;

    return build_path_va (separator, args);
}

xstr
x_build_pathv           (xcstr          separator,
                         va_list        argv)
{
    xstr ret;
    xstrv strv;
    if (!separator)
        separator = X_DIR_SEPARATOR;
    strv = x_strv_newv("", argv);
    ret = build_path_va (separator, (xcstr*)strv);
    x_strv_free(strv);
    return ret;
}
typedef struct {
    xUri        *uri;
    xcstr       str;
    xstr        buf;
    xError      **error;
    xbool       failed;
} UriParseState;
static int
uri_unescape            (const char     *s)
{
    int first_digit;
    int second_digit;

    first_digit = x_xdigit_value (*s++);
    if (first_digit < 0)
        return -1;

    second_digit = x_xdigit_value (*s++);
    if (second_digit < 0)
        return -1;

    return (first_digit << 4) | second_digit;
}

static xstr
uri_alloc               (UriParseState  *state,
                         xcstr          p)
{
    int     c;
    xstr    ret;
    xstr    r = state->buf;
    xcstr   s = state->str;
    if (s >= p) return NULL;

    while (s < p) {
        c = *s++;
        if (c == '%') {
            if (p - s < 2) {
                state->failed = TRUE;
                x_set_error (state->error, 0, -1,
                             "no enough uri escape char at %s", s);
                return NULL;
            }
            c = uri_unescape (s);
            if (c <= 0) {
                state->failed = TRUE;
                x_set_error (state->error, 0, -1,
                             "invalid uri escape char at %s", s);
                return NULL;
            }
            s += 2;
        }
        *r++ = c;
    }
    *r++ = '\0';
    ret = state->buf;
    state->buf = r;
    state->str = s;
    return ret;
}
static void 
uri_parse_scheme        (UriParseState  *state)
{
    const xchar *p = state->str;
    xchar   c = *p++;
    if (!isalpha(c)) 
        return;
    while (TRUE) {
        if (!isprint(c = *p))
            return;
        if (c == ':')
            break;
        if (!(isalnum(c) || c == '+' || c == '-' || c == '.'))
            return;
        ++p;
    }
    state->uri->scheme = uri_alloc (state, p);
}
static void
uri_parse_authority     (UriParseState  *state)
{
    const xchar *colon=NULL;
    const xchar *at=NULL;
    const xchar *p = state->str;
    xchar       c;

    /* scheme ':' or // */
    if (state->uri->scheme) {
        if (*p != ':') 
            return;
        state->str = ++p;
    }
    if (!(p[0] == '/' && p[1] == '/'))
        return;

    state->str = (p += 2);
    while (isprint(c = *p)) {
        if (c == '@')
            at = p,colon = NULL;
        else if (c == ':')
            colon = p;        
        else if (c == '/' || c == '?' || c == '&' || c == '#')
            break;
        ++p;
    }
    if (at) {
        state->uri->user = uri_alloc (state, at);
        state->str = at + 1;
    }
    if (colon) {
        state->uri->host = uri_alloc (state, colon);
        state->str = colon + 1;
        state->uri->port = uri_alloc (state, p);
    }
    else {
        state->uri->host = uri_alloc (state, p);
    }
}
static void
uri_parse_path          (UriParseState  *state)
{
    const xchar *p = state->str;
    xchar   c = *p++;
    /* ':' path or ://user@host:port path */
    if (!state->uri->user && !state->uri->host && !state->uri->port){
        if (c == ':'){
            state->str = p;
            c = *p++;
        }
    }
    if (!(c == '/' || c == '.' || isalnum(c)))
        return;
    while (isprint(c = *p)) {
        if (c == '@') {
            state->failed = TRUE;
            x_set_error (state->error, 0, -1,
                         "invalid char '@' at uri.path %s", p);
            return;
        }
        if (c == '?' || c == '#' || c == '&')
            break;
        ++p;
    }
    state->uri->path = uri_alloc (state, p);
}
static void
uri_parse_query         (UriParseState  *state)
{
    const xchar *p = state->str;
    xchar   c = *p++;
    if (c != '?')
        return;
    state->str = p;
    while (isprint(c = *p)) {
        if (c == ':' || c == '@' || c == '/' || c == '?') {
            state->failed = TRUE;
            x_set_error (state->error, 0, -1,
                         "invalid char %c at uri.query %s", c, p);
            return;
        }
        if (c == '#')
            break;
        ++p;
    }
    state->uri->query = uri_alloc (state, p);
}
static void
uri_parse_fragment      (UriParseState  *state)
{
    const xchar *p = state->str;
    xchar   c = *p++;
    if (c != '#')
        return;
    state->str = p;
    while (isprint(c = *p)) {
        if (c == ':' || c == '@' || c == '/'
            || c == '?' || c == '&' || c == '#') {
            state->failed = TRUE;
            x_set_error (state->error, 0, -1,
                         "invalid char %c at uri.fragment %s", c, p);
            return;
        }
        ++p;
    }
    state->uri->fragment = uri_alloc (state, p);
}
xUri*
x_uri_parse             (xcstr          str,
                         xError         **error)
{
    UriParseState state;
    xchar    *uri;
    xcstr    escape_str;
    xint     escape_num;

    x_return_val_if_fail (str != NULL, NULL);

    escape_num = 0;
    escape_str = str;
    while ((escape_str = strchr (escape_str, '%'))) {
        ++escape_num;
        ++escape_str;
    }

    uri = x_malloc0 (sizeof(xUri) + strlen (str) - (escape_num * 2) + 1);
    state.str = str;
    state.buf = uri + sizeof(xUri);
    state.uri = (xUri*)uri;
    state.error = error;
    state.failed = FALSE;

    uri_parse_scheme (&state);
    if (state.failed) goto failed;

    uri_parse_authority (&state);
    if (state.failed) goto failed;

    uri_parse_path (&state);
    if (state.failed) goto failed;

    uri_parse_query (&state);
    if (state.failed) goto failed;

    uri_parse_fragment (&state);
    if (state.failed) goto failed;

    return state.uri;
failed:
    x_free (uri);
    return NULL;
}

xstr
x_uri_build             (xUri           *uri,
                         xbool          allow_utf8)
{
    xString *string;
    x_return_val_if_fail (uri != NULL, NULL);

    string = x_string_new (NULL, 0);
    if (uri->scheme){
        x_string_append_uri_escaped (string, uri->scheme,
                                     NULL, FALSE);
        x_string_append_char (string, ':');
    }
    if (uri->user  || uri->host ) {
        x_string_append (string, "//");
        if (uri->user) {
            x_string_append_uri_escaped (string, uri->user,
                                         ":", allow_utf8);
            x_string_append_char (string, '@');
        }
        if (uri->host) {
            x_string_append_uri_escaped (string, uri->host,
                                         NULL, FALSE);
            if (uri->port) {
                x_string_append_char (string, ':');
                x_string_append_uri_escaped (string, uri->port,
                                             NULL, FALSE);
            }
        }
    }
    if (uri->path)
        x_string_append_uri_escaped (string, uri->path,
                                     "/:@", allow_utf8);
    if (uri->query) {
        x_string_append_char (string, '?');
        x_string_append_uri_escaped (string, uri->query,
                                     "=&", allow_utf8);
    }
    if (uri->fragment) {
        x_string_append_char (string, '#');
        x_string_append_uri_escaped (string, uri->fragment,
                                     NULL, allow_utf8);
    }
    return x_string_delete (string, FALSE);
}
