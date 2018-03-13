#ifndef __X_OPTION_H__
#define __X_OPTION_H__
#include "xtype.h"
X_BEGIN_DECLS

/* option argument type */
typedef enum {
    X_OPTION_NONE,
    X_OPTION_STR,
    X_OPTION_INT,
    X_OPTION_DOUBLE,
    X_OPTION_INT64,
    X_OPTION_PATH,
    X_OPTION_STR_ARRAY,
    X_OPTION_PATH_ARRAY,
    X_OPTION_CALLBACK,
} xOptionArg;
typedef xbool   (*xOptionCallback)  (xcstr          option_name,
                                     xcstr          value,
                                     xptr           group_data,
                                     xError         **error);
enum {
    /* hidden in '--help' */
    X_OPTION_HELP_HIDDEN		= 1 << 0,
    /* appears in main section */
    X_OPTION_IN_MAIN_HELP		= 1 << 1,
    /* only for X_OPTION_NONE */
    X_OPTION_REVERSE_VALUE		= 1 << 2,
    
    X_OPTION_FUNC_NO_ARG		= 1 << 3,
    X_OPTION_FUNC_PATH_ARG	    = 1 << 4,
    X_OPTION_FUNC_OPTIONAL_ARG  = 1 << 5,
    /* turn off conflict resolution */
    X_OPTION_MUST_NOALIAS	    = 1 << 6
};

typedef xbool   (*xOptionParseFunc) (xOptionContext *context,
                                     xOptionGroup   *group,
                                     xptr	        data,
                                     xError         **error);

typedef void    (*xOptionErrorFunc) (xOptionContext *context,
                                     xOptionGroup   *group,
                                     xptr           data,
                                     xError         **error);



xQuark          x_option_domain();

xOptionContext* x_option_context_new        (xcstr          title,
                                             xcstr          description,
                                             xcstr          summary,
                                             xcstr          i18n_domain);

void            x_option_context_add_group  (xOptionContext *context,
                                             xOptionGroup   *group);

void            x_option_context_add_entries(xOptionContext *context,
                                             const xOption  *entries);

void            x_option_context_set_data   (xOptionContext *context,
                                             xptr           user_data,
                                             xFreeFunc      free_func);

xstr            x_option_context_get_help   (xOptionContext *context,
                                             xbool          main_help,
                                             xOptionGroup   *group);

xbool           x_option_context_parse      (xOptionContext *context,
                                             xsize          *argc,
                                             xstrv          *argv,
                                             xError         **error);

void            x_option_context_delete     (xOptionContext *context);

xOptionGroup*   x_option_group_new          (xcstr          name,
                                             xcstr          description,
                                             xcstr          help_description,
                                             xcstr          i18n_domain);

void            x_option_group_add_entries  (xOptionGroup   *group,
                                             const xOption  *entries);

void            x_option_group_set_data     (xOptionGroup   *group,
                                             xptr           user_data,
                                             xFreeFunc      free_func);

void            x_option_group_delete       (xOptionGroup   *group);

/**
 * X_OPTION_REMAINING:
 * 
 * If a long option in the main group has this name, it is not treated as a 
 * regular option. Instead it collects all non-option arguments which would
 * otherwise be left in <literal>argv</literal>. The option must be of type
 * %X_OPTION_CALLBACK, %X_OPTION_STR_ARRAY,or %X_OPTION_PATH_ARRAY.
 * 
 * 
 * Using #X_OPTION_REMAINING instead of simply scanning <literal>argv</literal>
 * for leftover arguments has the advantage that GOption takes care of 
 * necessary encoding conversions for strings or filenames.
 * 
 */
#define X_OPTION_REMAINING ""

struct _xOption
{
    xcstr       long_name;
    xchar       short_name;
    xuint       flags;

    xOptionArg  arg;
    xptr        arg_data;

    xcstr       description;
    xcstr       arg_description;
};

X_END_DECLS
#endif /* __X_OPTION_H__ */

