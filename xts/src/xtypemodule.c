#include "config.h"
#include "xtypemodule.h"
#include "xtypeplugin.h"
#include "xvalue.h"
#include "xparam.h"
static void
type_plugin_init        (xTypePluginClass   *plugin,
                         xptr               data);
X_DEFINE_TYPE_EXTENDED(xTypeModule, x_type_module, X_TYPE_OBJECT,0,
                       X_IMPLEMENT_IFACE(X_TYPE_TYPE_PLUGIN, type_plugin_init));

#define X_TYPE_MODULE_PRIVATE(module) \
    X_INSTANCE_GET_PRIVATE(module, X_TYPE_TYPE_MODULE, xTypeModulePrivate);

struct _xTypeModulePrivate
{
    xint            use_count;
    xSList          *type_infos;
    xSList          *iface_infos;
    xModule         *library;
    xstr            path;
    xstr            entry;
};

typedef struct {
    xbool           loaded;
    xType           type;
    xType           parent;
    xTypeInfo       info;
} ModuleTypeInfo;

typedef struct {
    xbool           loaded;
    xType           type;
    xType           iface_type;
    xIFaceInfo      info;
} ModuleIFaceInfo;

static ModuleTypeInfo*
type_module_find_type_info  (xTypeModule    *module,
                             xType          type)
{
    xSList              *tmp_list;
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(module);

    tmp_list = module_private->type_infos;
    while (tmp_list) {
        ModuleTypeInfo *info = tmp_list->data;
        if (info->type == type)
            return info;

        tmp_list = tmp_list->next;
    }

    return NULL;
}
static ModuleIFaceInfo*
type_module_find_iface_info (xTypeModule    *module,
                             xType          type,
                             xType          iface_type)
{
    xSList              *tmp_list;
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(module);

    tmp_list = module_private->iface_infos;
    while (tmp_list) {
        ModuleIFaceInfo *info = tmp_list->data;
        if (info->type == type && info->iface_type == iface_type)
            return info;

        tmp_list = tmp_list->next;
    }

    return NULL;
}
static void
type_plugin_use         (xTypePlugin    *plugin)
{
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(plugin);

    module_private->use_count++;
    if (module_private->use_count == 1) {
        xSList *tmp_list;

        if (!X_TYPE_MODULE_GET_CLASS (plugin)->load (X_TYPE_MODULE (plugin))) {
            module_private->use_count--;
            x_error ("plugin '%s' failed to load.",module_private->path);
            return;
        }

        tmp_list = module_private->type_infos;
        while (tmp_list) {
            ModuleTypeInfo *type_info = tmp_list->data;
            if (!type_info->loaded) {
                x_error ("plugin '%s' failed to register type '%s'\n",
                         module_private->path, x_type_name (type_info->type));
                module_private->use_count--;
                return;
            }

            tmp_list = tmp_list->next;
        }
    }
}
static void
type_plugin_unuse       (xTypePlugin    *plugin)
{
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(plugin);

    x_return_if_fail (module_private->use_count > 0);

    module_private->use_count--;
    if (module_private->use_count == 0) {
        xSList *tmp_list;

        X_TYPE_MODULE_GET_CLASS (plugin)->unload (X_TYPE_MODULE (plugin));

        tmp_list = module_private->type_infos;
        while (tmp_list) {
            ModuleTypeInfo *type_info = tmp_list->data;
            type_info->loaded = FALSE;

            tmp_list = tmp_list->next;
        }
    }
}
static void
type_plugin_type_info   (xTypePlugin    *plugin,
                         xType          type,
                         xTypeInfo      *info,
                         xValueTable    *value_table)
{
    xTypeModule         *module;
    ModuleTypeInfo      *module_type_info;

    module = X_TYPE_MODULE (plugin);
    x_return_if_fail (module);

    module_type_info = type_module_find_type_info (module, type);
    x_return_if_fail (module_type_info);
    *info = module_type_info->info;
    if (module_type_info->info.value_table)
        *value_table = *module_type_info->info.value_table;
}
static void
type_plugin_iface_info  (xTypePlugin    *plugin,
                         xType          type,
                         xType          iface,
                         xIFaceInfo     *info)
{
    xTypeModule         *module;
    ModuleIFaceInfo     *module_iface_info;

    module = X_TYPE_MODULE (plugin);
    x_return_if_fail (module);

    module_iface_info = type_module_find_iface_info (module, type, iface);
    x_return_if_fail (module_iface_info);
    *info = module_iface_info->info;
}
static void
type_plugin_init        (xTypePluginClass   *plugin,
                         xptr               data)
{
    plugin->type_info   = type_plugin_type_info;
    plugin->iface_info  = type_plugin_iface_info;
    plugin->use         = type_plugin_use;
    plugin->unuse       = type_plugin_unuse;
}
xType
x_type_module_register  (xTypeModule        *module,
                         xcstr              name,
                         xType              parent,
                         const xTypeInfo    *info,
                         xTypeFlags         flags)
{
    xTypeModulePrivate  *module_private;
    ModuleTypeInfo      *type_info = NULL;
    xType               type;

    x_return_val_if_fail (module, X_TYPE_INVALID);
    x_return_val_if_fail (name, X_TYPE_INVALID);
    x_return_val_if_fail (info, X_TYPE_INVALID);

    module_private = X_TYPE_MODULE_PRIVATE(module);
    x_return_val_if_fail (module_private, 0);

    type = x_type_from_name (name);
    if (type) {
        xTypePlugin *old_plugin = x_type_plugin (type);

        if (old_plugin != X_TYPE_PLUGIN (module)) {
            x_warning ("Two different plugins tried to register '%s'.",
                       name);
            return X_TYPE_INVALID;
        }
    }

    if (type) {
        type_info = type_module_find_type_info (module, type);

        if (type_info->parent != parent) {
            xcstr parent_name = x_type_name (parent);

            x_warning ("Type '%s' recreated with different parent type.\n"
                       "(was '%s', now '%s')",
                       name,
                       x_type_name (type_info->parent),
                       parent_name ? parent_name : "(unknown)");
            return X_TYPE_INVALID;
        }
    } else {
        type_info = x_slice_new (ModuleTypeInfo);

        type_info->parent = parent;
        type_info->type = x_type_register_dynamic (name, parent,
                                                   X_TYPE_PLUGIN (module),
                                                   flags);

        module_private->type_infos = 
            x_slist_prepend (module_private->type_infos, type_info);
    }

    type_info->info = *info;
    type_info->loaded = TRUE;
    return type_info->type;
}
void
x_type_module_add_iface (xTypeModule        *module,
                         xType              type,
                         xType              iface_type,
                         const xIFaceInfo   *iface_info)
{
    ModuleIFaceInfo     *module_iface_info;
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(module);

    x_return_if_fail (iface_info != NULL);
    if (x_type_is (type, iface_type)) {
        xTypePlugin* old_plugin = x_type_iface_plugin (type, iface_type);
        if (!old_plugin) {
            x_warning ("Interface '%s' for '%s' was previously "
                       "registered statically or for a parent type.",
                       x_type_name (iface_type)
                       , x_type_name (type));
            return;
        }
        else if (old_plugin != X_TYPE_PLUGIN (module)) {
            x_warning ("Two different plugins tried to "
                       "register interface '%s' for '%s'.",
                       x_type_name (iface_type),
                       x_type_name (type));
            return;
        }

        module_iface_info = type_module_find_iface_info (module,
                                                         type,
                                                         iface_type);

        x_assert (module_iface_info);
    }
    else {
        module_iface_info = x_new (ModuleIFaceInfo, 1);
        module_iface_info->type = type;
        module_iface_info->iface_type = iface_type;
        x_type_add_iface_dynamic (type, iface_type,
                                  X_TYPE_PLUGIN (module));
        module_private->iface_infos = 
            x_slist_prepend(module_private->iface_infos,
                            module_iface_info);
    }
    module_iface_info->loaded = TRUE;
    module_iface_info->info = *iface_info;
}

xTypeModule*
x_type_moudle_load      (xcstr              filename)
{
    xTypeModule *module;

    x_return_val_if_fail (filename != NULL, NULL);

    module = x_object_new (X_TYPE_TYPE_MODULE, "path", filename, NULL);
    x_return_val_if_fail (module != NULL, NULL);
    
    x_type_plugin_use(X_TYPE_PLUGIN(module));
    return module;
}

static xbool
type_module_load        (xTypeModule    *module)
{
    xTypeModuleEntry load_func;
    xError              *error = NULL;
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(module);

    module_private->library = x_module_open (module_private->path, &error);
    if (!module_private->library) {
        x_warning ("%s", error->message);
        x_error_delete (error);
        return FALSE;
    }

    /* extract symbols from the lib */
    load_func = x_module_symbol (module_private->library,
                                 module_private->entry, &error);
    if (!load_func) {
        x_warning ("%s", error->message);
        x_error_delete (error);
        x_module_close (module_private->library);
        module_private->library = NULL;
        return FALSE;
    }

    /* symbol can still be NULL even though g_module_symbol
     * returned TRUE */
    if (!load_func(module)) {
        x_warning ("Symbol '%s()' return FALSE", module_private->entry);
        x_module_close (module_private->library);
        module_private->library = NULL;
        return FALSE;
    }

    return TRUE;
}

static void
type_module_unload      (xTypeModule    *module)
{
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(module);

    if (module_private->library) {
        x_module_close (module_private->library);
        module_private->library = NULL;
    }
}
static void
x_type_module_init      (xTypeModule   *module)
{
    module->module_private = x_instance_private(module,X_TYPE_TYPE_MODULE);
}
static void
type_module_finalize    (xObject        *object)
{
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(object);

    x_free (module_private->path);
    x_free (module_private->entry);
}

enum { PROP_0, PROP_PATH, PROP_ENTRY, N_PROPERTIES};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(object);

    switch(property_id){
    case PROP_PATH:
        module_private->path =  x_value_dup_str (value);
        break;
    case PROP_ENTRY:
        module_private->entry =  x_value_dup_str (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    xTypeModulePrivate  *module_private;
    module_private = X_TYPE_MODULE_PRIVATE(object);

    switch(property_id){
    case PROP_PATH:
        x_value_set_str (value, module_private->path);
        break;
    case PROP_ENTRY:
        x_value_set_str (value, module_private->entry);
        break;
    }
}
static void
x_type_module_class_init(xTypeModuleClass   *klass)
{
    xObjectClass    *oclass;
    xParam *param;

    x_class_set_private(klass,sizeof(xTypeModulePrivate));

    klass->load = type_module_load;
    klass->unload = type_module_unload;

    oclass = X_OBJECT_CLASS(klass);
    oclass->set_property = set_property;
    oclass->get_property = get_property;
    oclass->finalize = type_module_finalize;

    param = x_param_str_new ("path","Path","Path", NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_PATH,param);

    param = x_param_str_new ("entry","entry","entry", "type_module_main",
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_ENTRY,param);
}
