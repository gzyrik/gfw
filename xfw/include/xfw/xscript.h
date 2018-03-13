#ifndef __X_SCRIPT_H__
#define __X_SCRIPT_H__
#include "xtype.h"
X_BEGIN_DECLS

typedef xptr (*xScriptEnter)(xptr object, xcstr val, xcstr group, xQuark parent);
typedef void (*xScriptVisit)(xptr object, xcstr key, xcstr val);
typedef void (*xScriptLeave)(xptr object, xptr parent, xcstr group);

void        x_script_import         (xCache         *cache,
                                     xcstr          group,
                                     xptr           object);

void        x_script_import_file    (xFile          *file,
                                     xcstr          group,
                                     xptr           object);

void        x_script_import_str     (xcstr          str,
                                     xcstr          group,
                                     xptr           object);

xScript*    x_script_set            (xScript        *parent,
                                     xQuark         name,
                                     xScriptEnter   on_enter,
                                     xScriptVisit   on_visit,
                                     xScriptLeave   on_leave);

void        x_script_link           (xScript        *parent,
                                     xQuark         name,
                                     xScript        *child);

xScript*    x_script_get            (xcstr          path);

void        x_script_begin          (xFile         *file,
                                     xcstr          key,
                                     xcstr          format,
                                     ...)X_PRINTF (3, 4);

void        x_script_begin_valist   (xFile         *file,
                                     xcstr          key,
                                     xcstr          format,
                                     va_list        argv);

void        x_script_end            (xFile         *file);

void        x_script_write          (xFile         *file,
                                     xcstr          key,
                                     xcstr          format,
                                     ...)X_PRINTF (3, 4);

void        x_script_write_valist   (xFile         *file,
                                     xcstr          key,
                                     xcstr          format,
                                     va_list        argv);

void        x_script_comment        (xFile         *file,
                                     xcstr          comment);

X_END_DECLS
#endif /* __X_SCRIPT_H__ */
