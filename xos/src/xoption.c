#include "config.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include "xoption.h"
#include "xmem.h"
#include "xstr.h"
#include "xmsg.h"
#include "xlist.h"
#include "xstring.h"
#include "xutil.h"
#include "xhash.h"
#include "xalloca.h"
#include "xprintf.h"
#include "xtest.h"
#include "xerror.h"
#include "xiconv.h"
#include "xslice.h"
#include "xatomic.h"
#include "xwchar.h"
#include "xquark.h"
#include "xfile.h"
#include "xi18n.h"

#define NO_ARG(entry) ((entry)->arg == X_OPTION_NONE ||       \
                       ((entry)->arg == X_OPTION_CALLBACK &&  \
                        ((entry)->flags & X_OPTION_FUNC_NO_ARG)))

#define OPTIONAL_ARG(entry) ((entry)->arg == X_OPTION_CALLBACK &&  \
                             (entry)->flags & X_OPTION_FUNC_OPTIONAL_ARG)

typedef struct
{
    xchar **ptr;
    xchar *value;
} PendingNull;

typedef struct {
    xOptionArg      arg_type;
    xptr            arg_data;
    union {
        xbool       boolean;
        xint        integer;
        xstr        str;
        xstr        *array;
        xdouble     dbl;
        xint64      int64;
    }               prev;
    union {
        xstr        str;
        struct {
            xint    len;
            xstr    *data;
        }           array;
    }               allocated;
} Change;

struct _xOptionContext
{
    xList           *groups;

    xstr            title;
    xstr            blurb;
    xstr            summary;
    xstr            i18n_domain;

    xuint           help_enabled   : 1;
    xuint           ignore_unknown : 1;

    xOptionGroup    *main_group;

    /* We keep a list of change so we can revert them */
    xList           *changes;

    /* We also keep track of all argv elements
     * that should be NULLed or modified.
     */
    xList           *pending_nulls;
};

struct _xOptionGroup
{
    xstr            name;
    xstr            description;
    xstr            help_description;
    xstr            i18n_domain;

    xFreeFunc       destroy_notify;
    xptr            user_data;

    xOption         *entries;
    xsize           n_entries;

    xOptionParseFunc pre_parse_func;
    xOptionParseFunc post_parse_func;
    xOptionErrorFunc error_func;
};

xQuark
x_option_domain         (void)
{
    static xQuark quark = 0;
    if (!x_atomic_ptr_get(&quark) && x_once_init_enter(&quark)) {
        quark = x_quark_from_static("xOption");
        x_once_init_leave (&quark);
    }
    return quark;
}

xOptionContext*
x_option_context_new        (xcstr          title,
                             xcstr          blurb,
                             xcstr          summary,
                             xcstr          i18n_domain)
{
    xOptionContext  *context;

    context = x_new0 (xOptionContext, 1);
    context->title = x_strdup (title);
    context->blurb = x_strdup (blurb);
    context->summary = x_strdup (summary);
    context->help_enabled = TRUE;
    return context;

}

void
x_option_context_add_group  (xOptionContext *context,
                             xOptionGroup   *group)
{
    xList *list;

    x_return_if_fail (context != NULL);
    x_return_if_fail (group != NULL);
    x_return_if_fail (group->name != NULL);
    x_return_if_fail (group->description != NULL);
    x_return_if_fail (group->help_description != NULL);

    for (list = context->groups; list; list = list->next) {
        xOptionGroup *g = (xOptionGroup *)list->data;

        if (x_strcmp(group->name, g->name) == 0) {
            x_warning ("A group named \"%s\" is "
                       "already part of `%s` xOptionContext",
                       group->name, context->title);
        }
    }

    context->groups = x_list_append (context->groups, group);
}

void
x_option_context_set_data   (xOptionContext *context,
                             xptr           user_data,
                             xFreeFunc      free_func)
{
    x_return_if_fail (context != NULL);

    if (!context->main_group)
        context->main_group = x_option_group_new (NULL, NULL, NULL, context->i18n_domain);

    x_option_group_set_data(context->main_group, user_data, free_func);
}
void
x_option_context_add_entries(xOptionContext *context,
                             const xOption  *entries)
{
    x_return_if_fail (context != NULL);
    x_return_if_fail (entries != NULL);

    if (!context->main_group)
        context->main_group =x_option_group_new (NULL, NULL, NULL, context->i18n_domain);

    x_option_group_add_entries (context->main_group, entries);
}

static xsize
calculate_max_length        (xOptionGroup   *group)
{
    xOption   *entry;
    xsize i, len, max_length;

    max_length = 0;

    for (i = 0; i < group->n_entries; i++) {
        entry = &group->entries[i];

        if (entry->flags & X_OPTION_HELP_HIDDEN)
            continue;

        len = x_str_width (entry->long_name);

        if (entry->short_name)
            len += 4;

        if (!NO_ARG (entry) && entry->arg_description)
            len += 1 + x_str_width (x_i18n_dtext (group->i18n_domain, entry->arg_description));

        max_length = MAX (max_length, len);
    }

    return max_length;
}
static xbool
context_has_h_entry         (xOptionContext *context)
{
    xsize i;
    xList *list;

    if (context->main_group) {
        for (i = 0; i < context->main_group->n_entries; i++) {
            if (context->main_group->entries[i].short_name == 'h')
                return TRUE;
        }
    }

    for (list = context->groups; list != NULL; list = x_list_next (list)) {
        xOptionGroup *group;

        group = (xOptionGroup*)list->data;
        for (i = 0; i < group->n_entries; i++) {
            if (group->entries[i].short_name == 'h')
                return TRUE;
        }
    }
    return FALSE;
}
static xbool
group_has_visible_entries   (xOptionContext *context,
                             xOptionGroup   *group,
                             xbool          main_entries)
{
    xuint reject_filter = X_OPTION_HELP_HIDDEN;
    xOption *entry;
    xsize i, l;
    xbool   main_group = group == context->main_group;

    if (!main_entries)
        reject_filter |= X_OPTION_IN_MAIN_HELP;

    for (i = 0, l = (group ? group->n_entries : 0); i < l; i++) {
        entry = &group->entries[i];

        if (main_entries && !main_group && !(entry->flags & X_OPTION_IN_MAIN_HELP))
            continue;
        if (entry->long_name[0] == 0) /* ignore rest entry */
            continue;
        if (!(entry->flags & reject_filter))
            return TRUE;
    }
    return FALSE;
}
static xbool
group_list_has_visible_entries  (xOptionContext *context,
                                 xList          *group_list,
                                 xbool          main_entries)
{
    while (group_list) {
        if (group_has_visible_entries (context, group_list->data, main_entries))
            return TRUE;

        group_list = group_list->next;
    }

    return FALSE;
}
static void
print_entry                 (xOptionGroup   *group,
                             xsize          max_length,
                             const xOption  *entry,
                             xString        *string)
{
    xString *str;

    if (entry->flags & X_OPTION_HELP_HIDDEN)
        return;

    if (entry->long_name[0] == 0)
        return;

    str = x_string_new (NULL, 32);

    if (entry->short_name)
        x_string_append_printf (str, "  -%c, --%s", entry->short_name, entry->long_name);
    else
        x_string_append_printf (str, "  --%s", entry->long_name);

    if (entry->arg_description)
        x_string_append_printf (str, "=%s", x_i18n_dtext (group->i18n_domain, entry->arg_description));

    x_string_append_printf (string, "%s%*s %s\n", str->str,
                            (int) (max_length + 4 - x_str_width (str->str)), "",
                            entry->description ? x_i18n_dtext (group->i18n_domain, entry->description) : "");
    x_string_delete (str, TRUE);
}
xstr
x_option_context_get_help   (xOptionContext *context,
                             xbool          main_help,
                             xOptionGroup   *group)
{
    xList       *list;
    xsize        max_length, len;
    xsize        i;
    xOption     *entry;
    xHashTable  *shadow_map;
    xbool       seen[256];
    const xchar *rest_description;
    xString *string;
    xuchar token;

    string = x_string_new (NULL, 1024);

    if (context->title) {
        x_string_append (string, x_i18n_dtext (context->i18n_domain, context->title));
        x_string_append (string, "\n\n");
    }

    rest_description = NULL;
    if (context->main_group) {
        for (i = 0; i < context->main_group->n_entries; i++) {
            entry = &context->main_group->entries[i];
            if (entry->long_name[0] == 0) {
                rest_description = x_i18n_dtext (context->main_group->i18n_domain, entry->arg_description);
                break;
            }
        }
    }

    x_string_append_printf (string, "%s\n  %s %s",
                            _("Usage:"), x_get_prgname(), _("[OPTION...]"));

    if (rest_description) {
        x_string_append (string, " ");
        x_string_append (string, rest_description);
    }

    x_string_append (string, "\n\n");

    if (context->blurb) {
        x_string_append (string, x_i18n_dtext (context->i18n_domain, context->blurb));
        x_string_append (string, "\n\n");
    }

    memset (seen, 0, sizeof (xbool) * 256);
    shadow_map = x_hash_table_new (x_str_hash, x_str_cmp);

    if (context->main_group) {
        for (i = 0; i < context->main_group->n_entries; i++) {
            entry = &context->main_group->entries[i];
            x_hash_table_insert (shadow_map,
                                 (xptr)entry->long_name,
                                 entry);

            if (seen[(xuchar)entry->short_name])
                entry->short_name = 0;
            else
                seen[(xuchar)entry->short_name] = TRUE;
        }
    }

    list = context->groups;
    while (list != NULL) {
        xOptionGroup *g = list->data;
        for (i = 0; i < g->n_entries; i++) {
            entry = &g->entries[i];
            if (x_hash_table_lookup (shadow_map, entry->long_name) &&
                !(entry->flags & X_OPTION_MUST_NOALIAS))
                entry->long_name = x_strdup_printf ("%s-%s", g->name, entry->long_name);
            else
                x_hash_table_insert (shadow_map, (xptr)entry->long_name, entry);

            if (seen[(xuchar)entry->short_name] &&
                !(entry->flags & X_OPTION_MUST_NOALIAS))
                entry->short_name = 0;
            else
                seen[(xuchar)entry->short_name] = TRUE;
        }
        list = list->next;
    }

    x_hash_table_unref (shadow_map);

    list = context->groups;

    max_length = x_str_width ("-?, --help");

    if (list)
    {
        len = x_str_width ("--help-all");
        max_length = MAX (max_length, len);
    }

    if (context->main_group)
    {
        len = calculate_max_length (context->main_group);
        max_length = MAX (max_length, len);
    }

    while (list != NULL) {
        xOptionGroup *g = list->data;

        /* First, we check the --help-<groupname> options */
        len = x_str_width ("--help-") + x_str_width (g->name);
        max_length = MAX (max_length, len);

        /* Then we go through the entries */
        len = calculate_max_length (g);
        max_length = MAX (max_length, len);

        list = list->next;
    }

    /* Add a bit of padding */
    max_length += 4;

    if (!group) {
        list = context->groups;

        token = context_has_h_entry (context) ? '?' : 'h';

        x_string_append_printf (string, "%s\n  -%c, --%-*s %s\n",
                                _("Help Options:"), token, max_length - 4, "help",
                                _("Show help options"));

        /* We only want --help-all when there are groups */
        if (list)
            x_string_append_printf (string, "  --%-*s %s\n",
                                    max_length, "help-all",
                                    _("Show all help options"));

        while (list) {
            xOptionGroup *g = list->data;

            if (group_has_visible_entries (context, g, FALSE))
                x_string_append_printf (string, "  --help-%-*s %s\n",
                                        max_length - 5, g->name,
                                        x_i18n_dtext (g->i18n_domain, g->help_description));

            list = list->next;
        }

        x_string_append (string, "\n");
    }

    if (group) {
        /* Print a certain group */

        if (group_has_visible_entries (context, group, FALSE)) {
            x_string_append (string, x_i18n_dtext (group->i18n_domain, group->description));
            x_string_append (string, "\n");
            for (i = 0; i < group->n_entries; i++)
                print_entry (group, max_length, &group->entries[i], string);
            x_string_append (string, "\n");
        }
    }
    else if (!main_help) {
        /* Print all groups */

        list = context->groups;

        while (list) {
            xOptionGroup *g = list->data;

            if (group_has_visible_entries (context, g, FALSE)) {
                x_string_append (string, g->description);
                x_string_append (string, "\n");
                for (i = 0; i < g->n_entries; i++)
                    if (!(g->entries[i].flags & X_OPTION_IN_MAIN_HELP))
                        print_entry (g, max_length, &g->entries[i], string);

                x_string_append (string, "\n");
            }

            list = list->next;
        }
    }

    /* Print application options if --help or --help-all has been specified */
    if ((main_help || !group) &&
        (group_has_visible_entries (context, context->main_group, TRUE) ||
         group_list_has_visible_entries (context, context->groups, TRUE)))
    {
        list = context->groups;

        x_string_append (string,  _("Application Options:"));
        x_string_append (string, "\n");
        if (context->main_group)
            for (i = 0; i < context->main_group->n_entries; i++)
                print_entry (context->main_group, max_length,
                             &context->main_group->entries[i], string);

        while (list != NULL)
        {
            xOptionGroup *g = list->data;

            /* Print main entries from other groups */
            for (i = 0; i < g->n_entries; i++)
                if (g->entries[i].flags & X_OPTION_IN_MAIN_HELP)
                    print_entry (g, max_length, &g->entries[i], string);

            list = list->next;
        }

        x_string_append (string, "\n");
    }

    if (context->summary)
    {
        x_string_append (string, x_i18n_dtext (context->i18n_domain, context->summary));
        x_string_append (string, "\n");
    }

    return x_string_delete (string, FALSE);
}
static void
print_help                  (xOptionContext *context,
                             xbool          main_help,
                             xOptionGroup   *group)
{
    xstr help;

    help = x_option_context_get_help (context, main_help, group);
    x_printf ("%s", help);
    x_free (help);

    exit (0);
}
static Change *
get_change (xOptionContext *context,
            xOptionArg      arg_type,
            xptr            arg_data)
{
    xList *list;
    Change *change = NULL;

    for (list = context->changes; list != NULL; list = list->next)
    {
        change = list->data;

        if (change->arg_data == arg_data)
            goto found;
    }

    change = x_new0 (Change, 1);
    change->arg_type = arg_type;
    change->arg_data = arg_data;

    context->changes = x_list_prepend (context->changes, change);

found:

    return change;
}
static xbool
parse_int               (const xchar    *arg_name,
                         const xchar    *arg,
                         xint           *result,
                         xError         **error)
{
    xchar *end;
    xlong tmp;

    errno = 0;
    tmp = strtol (arg, &end, 0);

    if (*arg == '\0' || *end != '\0') {
        x_set_error (error,
                     x_option_domain(), -1,
                     _("Cannot parse integer value '%s' for %s"),
                     arg, arg_name);
        return FALSE;
    }

    *result = tmp;
    if (*result != tmp || errno == ERANGE) {
        x_set_error (error,
                     x_option_domain(), -1,
                     _("Integer value '%s' for %s out of range"),
                     arg, arg_name);
        return FALSE;
    }

    return TRUE;
}
static xbool
parse_double (xcstr         arg_name,
              xcstr         arg,
              xdouble       *result,
              xError        **error)
{
    xcstr end;
    xdouble tmp;

    errno = 0;
    tmp = x_strtod (arg, &end);

    if (*arg == '\0' || *end != '\0') {
        x_set_error (error,
                     x_option_domain(), -1,
                     _("Cannot parse double value '%s' for %s"),
                     arg, arg_name);
        return FALSE;
    }
    if (errno == ERANGE) {
        x_set_error (error,
                     x_option_domain(), -1,
                     _("Double value '%s' for %s out of range"),
                     arg, arg_name);
        return FALSE;
    }

    *result = tmp;

    return TRUE;
}


static xbool
parse_int64             (xcstr          arg_name,
                         xcstr          arg,
                         xint64         *result,
                         xError         **error)
{
    xcstr       end;
    xint64      tmp;

    errno = 0;
    tmp = x_strtoll (arg, &end, 0);

    if (*arg == '\0' || *end != '\0') {
        x_set_error (error,
                     x_option_domain(), -1,
                     _("Cannot parse integer value '%s' for %s"),
                     arg, arg_name);
        return FALSE;
    }
    if (errno == ERANGE) {
        x_set_error (error,
                     x_option_domain(), -1,
                     _("Integer value '%s' for %s out of range"),
                     arg, arg_name);
        return FALSE;
    }

    *result = tmp;

    return TRUE;
}
static xbool
parse_arg               (xOptionContext *context,
                         xOptionGroup   *group,
                         xOption        *entry,
                         const xchar    *value,
                         const xchar    *option_name,
                         xError         **error)

{
    Change *change;

    x_assert (value || OPTIONAL_ARG (entry) || NO_ARG (entry));

    switch (entry->arg)
    {
    case X_OPTION_NONE: {
        change = get_change (context, X_OPTION_NONE,
                             entry->arg_data);

        *(xbool *)entry->arg_data = !(entry->flags & X_OPTION_REVERSE_VALUE);
        break;
    }
    case X_OPTION_STR: {
        xchar *data;

        data = x_locale_to_str (value, -1, NULL, NULL, error);

        if (!data)
            return FALSE;

        change = get_change (context, X_OPTION_STR,
                             entry->arg_data);
        x_free (change->allocated.str);

        change->prev.str = *(xchar **)entry->arg_data;
        change->allocated.str = data;

        *(xchar **)entry->arg_data = data;
        break;
    }
    case X_OPTION_STR_ARRAY: {
        xchar *data;

        data = x_locale_to_str (value, -1, NULL, NULL, error);

        if (!data)
            return FALSE;

        change = get_change (context, X_OPTION_STR_ARRAY,
                             entry->arg_data);

        if (change->allocated.array.len == 0) {
            change->prev.array = *(xchar ***)entry->arg_data;
            change->allocated.array.data = x_new (xstr, 2);
        }
        else
            change->allocated.array.data =
                x_renew (xstr, change->allocated.array.data,
                         change->allocated.array.len + 2);

        change->allocated.array.data[change->allocated.array.len] = data;
        change->allocated.array.data[change->allocated.array.len + 1] = NULL;

        change->allocated.array.len ++;

        *(xchar ***)entry->arg_data = change->allocated.array.data;

        break;
    }
    case X_OPTION_PATH: {
        xchar *data;

#ifdef X_OS_WIN32
        data = x_locale_to_str (value, -1, NULL, NULL, error);

        if (!data)
            return FALSE;
#else
        data = x_strdup (value);
#endif
        change = get_change (context, X_OPTION_PATH,
                             entry->arg_data);
        x_free (change->allocated.str);

        change->prev.str = *(xchar **)entry->arg_data;
        change->allocated.str = data;

        *(xchar **)entry->arg_data = data;
        break;
    }

    case X_OPTION_PATH_ARRAY: {
        xchar *data;

#ifdef X_OS_WIN32
        data = x_locale_to_str (value, -1, NULL, NULL, error);

        if (!data)
            return FALSE;
#else
        data = x_strdup (value);
#endif
        change = get_change (context, X_OPTION_PATH_ARRAY,
                             entry->arg_data);

        if (change->allocated.array.len == 0)
        {
            change->prev.array = *(xchar ***)entry->arg_data;
            change->allocated.array.data = x_new (xstr, 2);
        }
        else
            change->allocated.array.data =
                x_renew (xstr, change->allocated.array.data,
                         change->allocated.array.len + 2);

        change->allocated.array.data[change->allocated.array.len] = data;
        change->allocated.array.data[change->allocated.array.len + 1] = NULL;

        change->allocated.array.len ++;

        *(xstr**)entry->arg_data = change->allocated.array.data;

        break;
    }
    case X_OPTION_INT: {
        xint data;

        if (!parse_int (option_name, value,
                        &data,
                        error))
            return FALSE;

        change = get_change (context, X_OPTION_INT,
                             entry->arg_data);
        change->prev.integer = *(xint *)entry->arg_data;
        *(xint *)entry->arg_data = data;
        break;
    }
    case X_OPTION_CALLBACK: {
        xchar *data;
        xbool retval;

        if (!value && entry->flags & X_OPTION_FUNC_OPTIONAL_ARG)
            data = NULL;
        else if (entry->flags & X_OPTION_FUNC_NO_ARG)
            data = NULL;
        else if (entry->flags & X_OPTION_FUNC_PATH_ARG)
        {
#ifdef X_OS_WIN32
            data = x_locale_to_str (value, -1, NULL, NULL, error);
#else
            data = x_strdup (value);
#endif
        }
        else
            data = x_locale_to_str (value, -1, NULL, NULL, error);

        if (!(entry->flags & (X_OPTION_FUNC_NO_ARG|X_OPTION_FUNC_OPTIONAL_ARG)) &&
            !data)
            return FALSE;

        retval = ((xOptionCallback)entry->arg_data)(option_name, data, group->user_data, error);

        if (!retval && error != NULL && *error == NULL)
            x_set_error (error,
                         x_option_domain(), -1,
                         _("Error parsing option %s"), option_name);

        x_free (data);

        return retval;

        break;
    }
    case X_OPTION_DOUBLE: {
        xdouble data;

        if (!parse_double (option_name, value,
                           &data,
                           error)) {
            return FALSE;
        }

        change = get_change (context, X_OPTION_DOUBLE,
                             entry->arg_data);
        change->prev.dbl = *(xdouble *)entry->arg_data;
        *(xdouble *)entry->arg_data = data;
        break;
    }
    case X_OPTION_INT64: {
        xint64 data;

        if (!parse_int64 (option_name, value,
                          &data,
                          error)) {
            return FALSE;
        }

        change = get_change (context, X_OPTION_INT64,
                             entry->arg_data);
        change->prev.int64 = *(xint64 *)entry->arg_data;
        *(xint64 *)entry->arg_data = data;
        break;
    }
    default:
        x_assert_not_reached ();
    }

    return TRUE;
}
static void
add_pending_null        (xOptionContext *context,
                         xchar          **ptr,
                         xchar          *value)
{
    PendingNull *n;

    n = x_new0 (PendingNull, 1);
    n->ptr = ptr;
    n->value = value;

    context->pending_nulls = x_list_prepend (context->pending_nulls, n);
}
static void
free_pending_nulls (xOptionContext *context,
                    xbool        perform_nulls)
{
    xList *list;

    for (list = context->pending_nulls; list != NULL; list = list->next)
    {
        PendingNull *n = list->data;

        if (perform_nulls)
        {
            if (n->value)
            {
                /* Copy back the short options */
                *(n->ptr)[0] = '-';
                strcpy (*n->ptr + 1, n->value);
            }
            else
                *n->ptr = NULL;
        }

        x_free (n->value);
        x_free (n);
    }

    x_list_free (context->pending_nulls);
    context->pending_nulls = NULL;
}
static xbool
parse_short_option  (xOptionContext *context,
                     xOptionGroup   *group,
                     xsize          idx,
                     xsize          *new_idx,
                     xchar          arg,
                     xsize          *argc,
                     xstrv          *argv,
                     xError         **error,
                     xbool          *parsed)
{
    xsize j;

    for (j = 0; j < group->n_entries; j++)
    {
        if (arg == group->entries[j].short_name)
        {
            xchar *option_name;
            xchar *value = NULL;

            option_name = x_strdup_printf ("-%c", group->entries[j].short_name);

            if (NO_ARG (&group->entries[j]))
                value = NULL;
            else
            {
                if (*new_idx > idx)
                {
                    x_set_error (error,
                                 x_option_domain(), -1,
                                 _("Error parsing option %s"), option_name);
                    x_free (option_name);
                    return FALSE;
                }

                if (idx < *argc - 1)
                {
                    if (!OPTIONAL_ARG (&group->entries[j]))
                    {
                        value = (*argv)[idx + 1];
                        add_pending_null (context, &((*argv)[idx + 1]), NULL);
                        *new_idx = idx + 1;
                    }
                    else
                    {
                        if ((*argv)[idx + 1][0] == '-')
                            value = NULL;
                        else
                        {
                            value = (*argv)[idx + 1];
                            add_pending_null (context, &((*argv)[idx + 1]), NULL);
                            *new_idx = idx + 1;
                        }
                    }
                }
                else if (idx >= *argc - 1 && OPTIONAL_ARG (&group->entries[j]))
                    value = NULL;
                else
                {
                    x_set_error (error,
                                 x_option_domain(), -1,
                                 _("Missing argument for %s"), option_name);
                    x_free (option_name);
                    return FALSE;
                }
            }

            if (!parse_arg (context, group, &group->entries[j],
                            value, option_name, error))
            {
                x_free (option_name);
                return FALSE;
            }

            x_free (option_name);
            *parsed = TRUE;
        }
    }

    return TRUE;
}
static xbool
parse_long_option           (xOptionContext *context,
                             xOptionGroup   *group,
                             xsize          *idx,
                             xstr           arg,
                             xbool          aliased,
                             xsize          *argc,
                             xstrv          *argv,
                             xError         **error,
                             xbool          *parsed)
{
    xsize j;

    for (j = 0; j < group->n_entries; j++) {
        if (*idx >= *argc)
            return TRUE;

        if (aliased && (group->entries[j].flags & X_OPTION_MUST_NOALIAS))
            continue;

        if (NO_ARG (&group->entries[j]) &&
            strcmp (arg, group->entries[j].long_name) == 0) {
            xchar *option_name;
            xbool retval;

            option_name = x_strcat ("--", group->entries[j].long_name, NULL);
            retval = parse_arg (context, group, &group->entries[j],
                                NULL, option_name, error);
            x_free (option_name);

            add_pending_null (context, &((*argv)[*idx]), NULL);
            *parsed = TRUE;

            return retval;
        }
        else {
            xsize len = strlen (group->entries[j].long_name);

            if (strncmp (arg, group->entries[j].long_name, len) == 0 &&
                (arg[len] == '=' || arg[len] == 0)) {
                xchar *value = NULL;
                xchar *option_name;

                add_pending_null (context, &((*argv)[*idx]), NULL);
                option_name = x_strcat ("--", group->entries[j].long_name, NULL);

                if (arg[len] == '=')
                    value = arg + len + 1;
                else if (*idx < *argc - 1) {
                    if (!OPTIONAL_ARG (&group->entries[j])) {
                        value = (*argv)[*idx + 1];
                        add_pending_null (context, &((*argv)[*idx + 1]), NULL);
                        (*idx)++;
                    }
                    else {
                        if ((*argv)[*idx + 1][0] == '-') {
                            xbool retval;
                            retval = parse_arg (context, group, &group->entries[j],
                                                NULL, option_name, error);
                            *parsed = TRUE;
                            x_free (option_name);
                            return retval;
                        }
                        else {
                            value = (*argv)[*idx + 1];
                            add_pending_null (context, &((*argv)[*idx + 1]), NULL);
                            (*idx)++;
                        }
                    }
                }
                else if (*idx >= *argc - 1 && OPTIONAL_ARG (&group->entries[j])) {
                    xbool    retval;
                    retval = parse_arg (context, group, &group->entries[j],
                                        NULL, option_name, error);
                    *parsed = TRUE;
                    x_free (option_name);
                    return retval;
                }
                else {
                    x_set_error (error,
                                 x_option_domain(), -1,
                                 _("Missing argument for %s"), option_name);
                    x_free (option_name);
                    return FALSE;
                }

                if (!parse_arg (context, group, &group->entries[j],
                                value, option_name, error)) {
                    x_free (option_name);
                    return FALSE;
                }

                x_free (option_name);
                *parsed = TRUE;
            }
        }
    }

    return TRUE;
}
static xbool
parse_remaining_arg (xOptionContext *context,
                     xOptionGroup   *group,
                     xsize          *idx,
                     xsize          *argc,
                     xstrv          *argv,
                     xError         **error,
                     xbool          *parsed)
{
    xsize j;

    for (j = 0; j < group->n_entries; j++)
    {
        if (*idx >= *argc)
            return TRUE;

        if (group->entries[j].long_name[0])
            continue;

        x_return_val_if_fail (group->entries[j].arg == X_OPTION_CALLBACK ||
                              group->entries[j].arg == X_OPTION_STR_ARRAY ||
                              group->entries[j].arg == X_OPTION_PATH_ARRAY, FALSE);

        add_pending_null (context, &((*argv)[*idx]), NULL);

        if (!parse_arg (context, group, &group->entries[j], (*argv)[*idx], "", error))
            return FALSE;

        *parsed = TRUE;
        return TRUE;
    }

    return TRUE;
}
static void
free_changes_list (xOptionContext *context,
                   xbool        revert)
{
    xList *list;

    for (list = context->changes; list != NULL; list = list->next)
    {
        Change *change = list->data;

        if (revert)
        {
            switch (change->arg_type)
            {
            case X_OPTION_NONE:
                *(xbool*)change->arg_data = change->prev.boolean;
                break;
            case X_OPTION_INT:
                *(xint *)change->arg_data = change->prev.integer;
                break;
            case X_OPTION_STR:
            case X_OPTION_PATH:
                x_free (change->allocated.str);
                *(xchar **)change->arg_data = change->prev.str;
                break;
            case X_OPTION_STR_ARRAY:
            case X_OPTION_PATH_ARRAY:
                x_strv_free (change->allocated.array.data);
                *(xchar ***)change->arg_data = change->prev.array;
                break;
            case X_OPTION_DOUBLE:
                *(xdouble *)change->arg_data = change->prev.dbl;
                break;
            case X_OPTION_INT64:
                *(xint64 *)change->arg_data = change->prev.int64;
                break;
            default:
                x_assert_not_reached ();
            }
        }

        x_free (change);
    }

    x_list_free (context->changes);
    context->changes = NULL;
}
xbool
x_option_context_parse      (xOptionContext *context,
                             xsize          *argc,
                             xstrv          *argv,
                             xError         **error)
{
    xsize i, j, k;
    xList *list;
    xOptionParseFunc parse_func;
    /* Set program name */
    if (!x_get_prgname()) {
        xstr    prgname;

        if (argc && argv && *argc)
            prgname = x_path_get_basename ((*argv)[0]);
        else
            prgname = NULL;//platform_get_argv0 ();

        if (prgname)
            x_set_prgname (prgname);
        else
            x_set_prgname ("<unknown>");

        x_free (prgname);
    }

    /* Call pre-parse hooks */
    list = context->groups;
    while (list) {
        xOptionGroup *group = list->data;
        parse_func = group->pre_parse_func;
        if (parse_func) {
            if (!parse_func (context,
                             group,
                             group->user_data,
                             error))
                goto fail;
        }

        list = list->next;
    }

    if (context->main_group && context->main_group->pre_parse_func) {
        parse_func = context->main_group->pre_parse_func;
        if (!parse_func (context,
                         context->main_group,
                         context->main_group->user_data,
                         error))
            goto fail;
    }

    if (argc && argv) {
        xbool stop_parsing = FALSE;
        xbool has_unknown = FALSE;
        xsize separator_pos = 0;

        for (i = 1; i < *argc; i++) {
            xchar *arg, *dash;
            xbool parsed = FALSE;

            if ((*argv)[i][0] == '-' &&
                (*argv)[i][1] != '\0' && !stop_parsing) {
                if ((*argv)[i][1] == '-') {
                    /* -- option */

                    arg = (*argv)[i] + 2;

                    /* '--' terminates list of arguments */
                    if (*arg == 0) {
                        separator_pos = i;
                        stop_parsing = TRUE;
                        continue;
                    }

                    /* Handle help options */
                    if (context->help_enabled) {
                        if (strcmp (arg, "help") == 0)
                            print_help (context, TRUE, NULL);
                        else if (strcmp (arg, "help-all") == 0)
                            print_help (context, FALSE, NULL);
                        else if (strncmp (arg, "help-", 5) == 0) {
                            list = context->groups;

                            while (list) {
                                xOptionGroup *group = list->data;

                                if (strcmp (arg + 5, group->name) == 0)
                                    print_help (context, FALSE, group);

                                list = list->next;
                            }
                        }
                    }

                    if (context->main_group &&
                        !parse_long_option (context, context->main_group, &i, arg,
                                            FALSE, argc, argv, error, &parsed))
                        goto fail;

                    if (parsed)
                        continue;

                    /* Try the groups */
                    list = context->groups;
                    while (list) {
                        xOptionGroup *group = list->data;

                        if (!parse_long_option (context, group, &i, arg,
                                                FALSE, argc, argv, error, &parsed))
                            goto fail;

                        if (parsed)
                            break;

                        list = list->next;
                    }

                    if (parsed)
                        continue;

                    /* Now look for --<group>-<option> */
                    dash = strchr (arg, '-');
                    if (dash) {
                        /* Try the groups */
                        list = context->groups;
                        while (list) {
                            xOptionGroup *group = list->data;

                            if (strncmp (group->name, arg, dash - arg) == 0) {
                                if (!parse_long_option (context, group, &i, dash + 1,
                                                        TRUE, argc, argv, error, &parsed))
                                    goto fail;

                                if (parsed)
                                    break;
                            }

                            list = list->next;
                        }
                    }

                    if (context->ignore_unknown)
                        continue;
                }
                else { /* short option */
                    xsize new_i = i, arg_length;
                    xbool *nulled_out = NULL;
                    xbool has_h_entry = context_has_h_entry (context);
                    arg = (*argv)[i] + 1;
                    arg_length = strlen (arg);
                    nulled_out = x_newa (xbool, arg_length);
                    memset (nulled_out, 0, arg_length * sizeof (xbool));
                    for (j = 0; j < arg_length; j++) {
                        if (context->help_enabled && (arg[j] == '?' ||
                                                      (arg[j] == 'h' && !has_h_entry)))
                            print_help (context, TRUE, NULL);
                        parsed = FALSE;
                        if (context->main_group &&
                            !parse_short_option (context, context->main_group,
                                                 i, &new_i, arg[j],
                                                 argc, argv, error, &parsed))
                            goto fail;
                        if (!parsed) {
                            /* Try the groups */
                            list = context->groups;
                            while (list) {
                                xOptionGroup *group = list->data;
                                if (!parse_short_option (context, group, i, &new_i, arg[j],
                                                         argc, argv, error, &parsed))
                                    goto fail;
                                if (parsed)
                                    break;
                                list = list->next;
                            }
                        }

                        if (context->ignore_unknown && parsed)
                            nulled_out[j] = TRUE;
                        else if (context->ignore_unknown)
                            continue;
                        else if (!parsed)
                            break;
                        /* !context->ignore_unknown && parsed */
                    }
                    if (context->ignore_unknown) {
                        xchar *new_arg = NULL;
                        xint arg_index = 0;
                        for (j = 0; j < arg_length; j++) {
                            if (!nulled_out[j]) {
                                if (!new_arg)
                                    new_arg = x_malloc (arg_length + 1);
                                new_arg[arg_index++] = arg[j];
                            }
                        }
                        if (new_arg)
                            new_arg[arg_index] = '\0';
                        add_pending_null (context, &((*argv)[i]), new_arg);
                    }
                    else if (parsed) {
                        add_pending_null (context, &((*argv)[i]), NULL);
                        i = new_i;
                    }
                }

                if (!parsed)
                    has_unknown = TRUE;

                if (!parsed && !context->ignore_unknown) {
                    x_set_error (error,
                                 x_option_domain(), -1,
                                 _("Unknown option %s"), (*argv)[i]);
                    goto fail;
                }
            }
            else {
                /* Collect remaining args */
                if (context->main_group &&
                    !parse_remaining_arg (context, context->main_group, &i,
                                          argc, argv, error, &parsed))
                    goto fail;

                if (!parsed && (has_unknown || (*argv)[i][0] == '-'))
                    separator_pos = 0;
            }
        }

        if (separator_pos > 0)
            add_pending_null (context, &((*argv)[separator_pos]), NULL);

    }

    /* Call post-parse hooks */
    list = context->groups;
    while (list) {
        xOptionGroup *group = list->data;

        if (group->post_parse_func) {
            if (!(* group->post_parse_func) (context, group,
                                             group->user_data, error))
                goto fail;
        }

        list = list->next;
    }

    if (context->main_group && context->main_group->post_parse_func) {
        if (!(* context->main_group->post_parse_func) (context, context->main_group,
                                                       context->main_group->user_data, error))
            goto fail;
    }

    if (argc && argv)
    {
        free_pending_nulls (context, TRUE);

        for (i = 1; i < *argc; i++) {
            for (k = i; k < *argc; k++)
                if ((*argv)[k] != NULL)
                    break;

            if (k > i) {
                k -= i;
                for (j = i + k; j < *argc; j++) {
                    (*argv)[j-k] = (*argv)[j];
                    (*argv)[j] = NULL;
                }
                *argc -= k;
            }
        }
    }

    return TRUE;

fail:

    /* Call error hooks */
    list = context->groups;
    while (list) {
        xOptionGroup *group = list->data;

        if (group->error_func)
            group->error_func (context, group,
                               group->user_data, error);

        list = list->next;
    }

    if (context->main_group && context->main_group->error_func)
        context->main_group->error_func (context, context->main_group,
                                         context->main_group->user_data, error);

    free_changes_list (context, TRUE);
    free_pending_nulls (context, FALSE);

    return FALSE;
}
void
x_option_context_delete     (xOptionContext *context)
{
    x_return_if_fail (context != NULL);

    if (context->main_group)
        x_option_group_delete (context->main_group);

    free_changes_list (context, FALSE);
    free_pending_nulls (context, FALSE);

    x_free (context->title);
    x_free (context->blurb);
    x_free (context->summary);
    x_free (context->i18n_domain);

    x_free (context);
}
xOptionGroup*
x_option_group_new          (xcstr          name,
                             xcstr          description,
                             xcstr          help_description,
                             xcstr          translate_domain)
{
    xOptionGroup *group;

    group = x_new0 (xOptionGroup, 1);
    group->name = x_strdup (name);
    group->description = x_strdup (description);
    group->help_description = x_strdup (help_description);
    group->i18n_domain = x_strdup(translate_domain);

    return group;
}

void
x_option_group_set_data     (xOptionGroup   *group,
                             xptr           user_data,
                             xFreeFunc      free_func)
{
    x_return_if_fail (group != NULL);

    if (group->destroy_notify && group->user_data)
        group->destroy_notify(group->user_data);
    group->user_data = user_data;
    group->destroy_notify = free_func;
}


void
x_option_group_add_entries  (xOptionGroup   *group,
                             const xOption  *entries)
{
    xsize i, n_entries;

    x_return_if_fail (entries != NULL);


    for (n_entries = 0; entries[n_entries].long_name != NULL; n_entries++) ;

    group->entries = x_renew (xOption, group->entries,
                              group->n_entries + n_entries);

    memcpy (group->entries + group->n_entries, entries,
            sizeof (xOption) * n_entries);

    for (i = group->n_entries; i < group->n_entries + n_entries; i++) {
        xchar c = group->entries[i].short_name;

        if (c == '-' || (c != 0 && !isprint (c))) {
            x_warning ("%s: ignoring invalid short option '%c' (%d) "
                       "in entry %s:%s",
                       X_STRLOC, c, c,
                       group->name, group->entries[i].long_name);
            group->entries[i].short_name = '\0';
        }

        if (group->entries[i].arg != X_OPTION_NONE &&
            (group->entries[i].flags & X_OPTION_REVERSE_VALUE) != 0) {
            x_warning ("%s: ignoring reverse flag on option of "
                       "arg-type %d in entry %s:%s",
                       X_STRLOC, group->entries[i].arg,
                       group->name, group->entries[i].long_name);

            group->entries[i].flags &= ~X_OPTION_REVERSE_VALUE;
        }

        if (group->entries[i].arg != X_OPTION_CALLBACK &&
            (group->entries[i].flags & (X_OPTION_FUNC_NO_ARG|
                                        X_OPTION_FUNC_OPTIONAL_ARG|
                                        X_OPTION_FUNC_PATH_ARG)) != 0) {
            x_warning ("%s: ignoring no-arg, optional-arg or filename flags "
                       "(%d) on option of arg-type %d in entry %s:%s",
                       X_STRLOC,group->entries[i].flags,
                       group->entries[i].arg,
                       group->name,
                       group->entries[i].long_name);

            group->entries[i].flags &= ~(X_OPTION_FUNC_NO_ARG|
                                         X_OPTION_FUNC_OPTIONAL_ARG|
                                         X_OPTION_FUNC_PATH_ARG);
        }
    }

    group->n_entries += n_entries;
}

void
x_option_group_delete       (xOptionGroup   *group)
{
    x_return_if_fail (group != NULL);

    x_free (group->name);
    x_free (group->description);
    x_free (group->help_description);

    x_free (group->entries);

    if (group->destroy_notify)
        group->destroy_notify(group->user_data);

    x_free (group);
}
