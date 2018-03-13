#include "config.h"
#include "xi18n.h"
#include "xoption.h"
#include "xatomic.h"
#include "xmodule.h"
#include "xalloca.h"
#include "xwin.h"
#include <string.h>
#include <locale.h>

static char* (*p_gettext) (const char*);
static char* (*p_dgettext) (const char*, const char*);
static char* (*p_dcgettext) (const char*, const char *, int);
static char* (*p_ngettext) (const char*, const char*, unsigned long int );
static char* (*p_dngettext) (const char*, const char*, const char*, unsigned long int);
static char* (*p_dcngettext) (const char*, const char*, const char*, unsigned long int, int);
static char* (*p_textdomain) (const char*);
static char* (*p_bindtextdomain) (const char*, const char*);
static char* (*p_bind_textdomain_codeset) (const char*, const char*);

static xbool  _i18n_opt_notranslate;

void
x_i18n_bind_domain  (xcstr          domain,
                     xcstr          path)
{
    if (p_bindtextdomain)
        p_bindtextdomain(domain, path);
}

void
x_i18n_bind_codeset (xcstr          domain,
                     xcstr          codeset)
{
    if (p_bind_textdomain_codeset)
        p_bind_textdomain_codeset(domain, codeset);
}

void
x_i18n_domain       (xcstr          domain)
{
    if (p_textdomain)
        p_textdomain(domain);
}

xOptionGroup*
x_i18n_options      (void)
{
    static xOptionGroup *i18n_opts;
    if (!i18n_opts) {
        xOption entries[] =  {
            {"notranslate", '\0', 0, X_OPTION_NONE,
                &_i18n_opt_notranslate,
                "No translate", NULL},
            {NULL}
        };
        i18n_opts = x_option_group_new("i18n", "i18n Help Options:", "Show i18n help options", X_I18N_DOMAIN);
        x_option_group_add_entries(i18n_opts, entries);
    }
    return i18n_opts;
}

static xbool notranslate (void)
{
    static xssize _i18n_initialised = 0;
    if (X_UNLIKELY (x_once_init_enter (&_i18n_initialised))) {
        xError *error = NULL;
        xModule* intl = x_module_open("libintl", &error);
        if (intl) {
            x_module_resident(intl);
            p_gettext = x_module_symbol (intl, "gettext", &error);
            p_dgettext = x_module_symbol (intl, "dgettext", &error);
            p_dcgettext = x_module_symbol (intl, "dcgettext", &error);
            p_ngettext = x_module_symbol (intl, "ngettext", &error);
            p_dngettext = x_module_symbol (intl, "dngettext", &error);
            p_dcngettext = x_module_symbol (intl, "dcngettext", &error);
            p_textdomain = x_module_symbol (intl, "textdomain", &error);
            p_bindtextdomain = x_module_symbol (intl, "bindtextdomain", &error);
            p_bind_textdomain_codeset = x_module_symbol (intl, "bind_textdomain_codeset", &error);
        }
        if (p_textdomain && p_gettext) {
            const char *default_domain     = p_textdomain (NULL);
            const char *translator_comment = p_gettext ("");
#ifndef X_OS_WINDOWS
            const char *translate_locale   = setlocale (LC_MESSAGES, NULL);
#else
            const char *translate_locale   = x_win_getlocale ();
#endif
            /* We should NOT translate only if all the following hold:
             *   - user has called textdomain() and set textdomain to non-default
             *   - default domain has no translations
             *   - locale does not start with "en_" and is not "C"
             *
             * Rationale:
             *   - If text domain is still the default domain, maybe user calls
             *     it later. Continue with old behavior of translating.
             *   - If locale starts with "en_", we can continue using the
             *     translations even if the app doesn't have translations for
             *     this locale.  That is, en_UK and en_CA for example.
             *   - If locale is "C", maybe user calls setlocale(LC_ALL,"") later.
             *     Continue with old behavior of translating.
             */
            if (0 != strcmp (default_domain, "messages") &&
                '\0' == *translator_comment &&
                0 != strncmp (translate_locale, "en_", 3) &&
                0 != strcmp (translate_locale, "C"))
                _i18n_opt_notranslate = TRUE;
        }
        _i18n_initialised = TRUE;
        x_once_init_leave (&_i18n_initialised);
    }
    return _i18n_opt_notranslate;
}
#define NO_TRANSLATE    X_UNLIKELY (notranslate ())
/* i18n API */
xcstr
x_i18n_text     (xcstr          msgid)
{
    if (NO_TRANSLATE || !p_gettext)
        return msgid;
    return p_gettext (msgid);
}
xcstr
x_i18n_ntext    (xcstr          msgid,
                 xcstr          msgid_plural,
                 xulong         n) 
{
    if (NO_TRANSLATE || !p_ngettext)
        return n == 1 ? msgid : msgid_plural;
    return p_ngettext (msgid, msgid_plural, n);
}
xcstr
x_i18n_dtext    (xcstr          domain,
                 xcstr          msgid)
{
    if (NO_TRANSLATE || !p_dgettext)
        return msgid;

    return p_dgettext (domain, msgid);
}

xcstr
x_i18n_dctext   (xcstr          domain,
                 xcstr          msgid,
                 xint           category)
{
    if ((NO_TRANSLATE && domain) || !p_dcgettext)
        return msgid;

    return p_dcgettext (domain, msgid, category);
}
xcstr
x_i18n_dntext   (xcstr          domain,
                 xcstr          msgid,
                 xcstr          msgid_plural,
                 xulong         n)
{
    if ((NO_TRANSLATE && domain) || !p_dngettext)
        return n == 1 ? msgid : msgid_plural;

    return p_dngettext (domain, msgid, msgid_plural, n);

}
xcstr
x_i18n_dcntext  (xcstr          domain,
                 xcstr          msgid,
                 xcstr          msgid_plural,
                 xulong         n,
                 xint           category)
{
    if ((NO_TRANSLATE && domain) || !p_dcngettext)
        return n == 1 ? msgid : msgid_plural;

    return p_dcngettext (domain, msgid, msgid_plural, n, category);
}

xcstr
x_i18n_dptext   (xcstr          domain,
                 xcstr          msg_ctxt_id,
                 xsize          msgid_offset)
{
    xcstr translation;
    xchar *sep;

    translation = x_i18n_dtext (domain, msg_ctxt_id);

    if (translation == msg_ctxt_id) {/* failed */
        if (msgid_offset > 0) /* invalid call */
            return msg_ctxt_id + msgid_offset;

        /* try use '|' to context sep */
        sep = strchr (msg_ctxt_id, '|');
        if (sep) {
            /* try with '\004' instead of '|', in case
             * xgettext -kQ_:1g was used
             */
            xstr tmp = x_alloca (strlen (msg_ctxt_id) + 1);
            strcpy (tmp, msg_ctxt_id);
            tmp[sep - msg_ctxt_id] = '\004';

            translation = x_i18n_dtext (domain, tmp);

            if (translation == tmp)
                return sep + 1;
        }
    }
    return translation;
}

xcstr
x_i18n_dptext2  (xcstr          domain,
                 xcstr          context,
                 xcstr          msgid)
{
    xsize msgctxt_len = strlen (context) + 1;
    xsize msgid_len = strlen (msgid) + 1;
    xcstr translation;
    xstr msg_ctxt_id;

    msg_ctxt_id = x_alloca (msgctxt_len + msgid_len);

    memcpy (msg_ctxt_id, context, msgctxt_len - 1);
    msg_ctxt_id[msgctxt_len - 1] = '\004';
    memcpy (msg_ctxt_id + msgctxt_len, msgid, msgid_len);

    translation = x_i18n_dtext (domain, msg_ctxt_id);

    if (translation == msg_ctxt_id)
    {
        /* try the old way of doing message contexts, too */
        msg_ctxt_id[msgctxt_len - 1] = '|';
        translation = x_i18n_dtext (domain, msg_ctxt_id);

        if (translation == msg_ctxt_id)
            return msgid;
    }
    return translation;

}

xcstr
x_i18n_strip    (xcstr          msgid,
                 xcstr          msgval)
{
    if (msgval == msgid) {
        xcstr c = strchr (msgid, '|');
        if (c != NULL)
            return c + 1;
    }
    return msgval;
}
