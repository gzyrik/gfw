#ifndef __X_TYPE_PLUGIN_H__
#define __X_TYPE_PLUGIN_H__
#include "xclass.h"
X_BEGIN_DECLS
#define X_TYPE_TYPE_PLUGIN          (x_type_plugin_type ())

#define X_TYPE_PLUGIN(obj)          (X_INSTANCE_CAST ((obj), X_TYPE_TYPE_PLUGIN, xTypePlugin))

#define X_TYPE_PLUGIN_CLASS(klass)  (X_CLASS_CAST ((klass), X_TYPE_TYPE_PLUGIN, xTypePluginClass))

#define X_IS_TYPE_PLUGIN(obj)       (X_INSTANCE_IS_TYPE ((obj), X_TYPE_TYPE_PLUGIN))

#define X_IS_TYPE_PLUGIN_CLASS(klass) (X_CLASS_IS_TYPE ((klass), X_TYPE_TYPE_PLUGIN))

#define X_TYPE_PLUGIN_GET_CLASS(obj)  (X_INSTANCE_GET_IFACE ((obj), X_TYPE_TYPE_PLUGIN, xTypePluginClass))

xType       x_type_plugin_type      (void);

void        x_type_plugin_use       (xTypePlugin    *plugin);

void        x_type_plugin_unuse     (xTypePlugin    *plugin);

void        x_type_plugin_type_info (xTypePlugin    *plugin,
                                     xType          type,
                                     xTypeInfo      *info,
                                     xValueTable    *value_table);

void        x_type_plugin_iface_info(xTypePlugin   *plugin,
                                     xType          instance_type,
                                     xType          iface_type,
                                     xIFaceInfo     *info);

struct _xTypePluginClass
{
    xIFace              parent;
    /*< protected >*/
    void (*use)         (xTypePlugin*);
    void (*unuse)       (xTypePlugin*);
    void (*type_info)   (xTypePlugin*, xType, xTypeInfo*, xValueTable*);
    void (*iface_info)  (xTypePlugin*, xType, xType, xIFaceInfo*);
};

X_END_DECLS
#endif /* __X_TYPE_PLUGIN_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
