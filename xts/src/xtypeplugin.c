#include "config.h"
#include "xtypeplugin.h"
xType
x_type_plugin_type      (void)
{
    static xType type = 0;
    if ( !type){
        xTypeInfo info = {sizeof(xTypePluginClass),};
        type  = x_type_register_static ("xTypePlugin", X_TYPE_IFACE, &info,0);
    }
    return type;
}

void
x_type_plugin_use       (xTypePlugin    *plugin)
{
    xTypePluginClass    *klass;

    x_return_if_fail (X_IS_TYPE_PLUGIN (plugin));

    klass = X_TYPE_PLUGIN_GET_CLASS (plugin);
    klass->use(plugin); 
}

void
x_type_plugin_unuse     (xTypePlugin    *plugin)
{
    xTypePluginClass    *klass;

    x_return_if_fail (X_IS_TYPE_PLUGIN (plugin));

    klass = X_TYPE_PLUGIN_GET_CLASS (plugin);
    klass->unuse(plugin); 
}

void
x_type_plugin_type_info (xTypePlugin    *plugin,
                         xType          type,
                         xTypeInfo      *info,
                         xValueTable    *value_table)
{
    xTypePluginClass    *klass;

    x_return_if_fail (info != NULL);
    x_return_if_fail (value_table != NULL);
    x_return_if_fail (X_IS_TYPE_PLUGIN (plugin));

    klass = X_TYPE_PLUGIN_GET_CLASS (plugin);
    klass->type_info(plugin, type, info, value_table);
}

void
x_type_plugin_iface_info(xTypePlugin   *plugin,
                         xType          instance_type,
                         xType          iface_type,
                         xIFaceInfo     *info)
{
    xTypePluginClass    *klass;

    x_return_if_fail (info != NULL);
    x_return_if_fail (X_IS_TYPE_PLUGIN (plugin));

    klass = X_TYPE_PLUGIN_GET_CLASS (plugin);
    klass->iface_info(plugin, instance_type, iface_type, info);
}
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
