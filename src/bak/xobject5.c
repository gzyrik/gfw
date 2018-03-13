#include "config.h"
#include "xobject.h"
#include "xquark.h"
#include "xmem.h"
#include "xslice.h"
#include "xatomic.h"
#include "xthread.h"
#include "xhash.h"
#include "xmsg.h"
#include "xparam.h"
#include "xsignal.h"
#include "xslist.h"
#include "xobject-prv.h"
#include "xassert.h"
#include <string.h>
#define	X_TYPE_FUNDAMENTAL_SHIFT	(2)
#define	X_TYPE_FUNDAMENTAL_MAX		(0xFF << X_TYPE_FUNDAMENTAL_SHIFT)
#define	TYPE_ID_MASK				((xType) ((1 << X_TYPE_FUNDAMENTAL_SHIFT) - 1))
#define TYPE_MAGIC                  0x3d3d3d3d

static xRWLock          rw_type;
X_LOCK_DEFINE_STATIC    (type_ht);
static xHashTbl         *type_ht = NULL;

X_LOCK_DEFINE_STATIC    (ifaces_ht);
static xHashTbl         *ifaces_ht = NULL;

X_LOCK_DEFINE_STATIC    (construction_slist);
static xSList           *construction_slist = NULL;


typedef struct _xTypeInfo       xTypeInfo;
typedef struct _xIFaceSpec      xIFaceSpec;

struct _xTypeInfo
{
    xcstr               name;
    xint                ref;
    xuint               n_supers;
    xuint               n_children;
    xTypePlugin         *plugin;
    xValueTable         value_table;
    xInitFunc           class_init;
    xsize               class_size;
    xptr                class_data;
    xIFaceSpec          *iface_spec;
    xptr                klass;
    xType               *children; 
    xulong              magic;
    xType               supers[1];
};

struct _xIFaceSpec
{
    xType               owner_type;
    xType               iface_type;
    xInitFunc           iface_init;
    xptr                iface_data;
    xIFace              *iface;
    xIFaceSpec          *next;
};

static inline xTypeInfo*
x_type_info             (xType          type)
{
    if (type > X_TYPE_FUNDAMENTAL_MAX) {
        xTypeInfo* info = (xTypeInfo*) (type & ~TYPE_ID_MASK);
        if (info->magic == TYPE_MAGIC)
            return info;
    }
    return NULL;
}

static xuint
type_hash               (xcptr          key)
{
    return X_PTR_TO_UINT(key);
}

static xint
type_cmp                (xcptr          key1,
                         xcptr          key2)
{
    return X_PTR_TO_UINT(key1) - X_PTR_TO_UINT(key2);
}

static xbool
type_complete           (xTypeInfo      *info)
{
    if (!info->class_size && info->plugin) {
        xTypeSpec type_spec = {0};
        x_type_plugin_complete (info->plugin,
                                info->supers[0],
                                &type_spec);
        x_object_unref (X_OBJECT(info->plugin));
        info->plugin = NULL;
        if (type_spec.value_table)
            info->value_table = *type_spec.value_table;
        info->class_size = type_spec.class_size;
        info->class_init = type_spec.class_init;
        info->class_data = type_spec.class_data;
    }
    return info->class_size > 0;
}

xType
x_type_register         (xcstr          type_name,
                         xType          parent_type,
                         xTypeSpec      *type_spec,
                         xTypePlugin    *type_plugin)
{
    xTypeInfo   *info;
    xTypeInfo   *pinfo;
    xuint   n_supers;
    xuint   info_size;
    xType   type;

    x_return_val_if_fail (type_name, X_TYPE_VOID);
    x_return_val_if_fail (type_spec||type_plugin, X_TYPE_VOID);

    pinfo = x_type_info (parent_type);
    n_supers = pinfo ? pinfo->n_supers + 1 : 1;
    info_size = X_STRUCT_OFFSET(xTypeInfo, supers);
    /* self + ancestors + X_TYPE_VOID for ->supers[] */
    info_size += (sizeof (xType) * (1 + n_supers + 1));
    info = x_malloc0 (info_size);
    x_assert ((info & TYPE_ID_MASK) == 0);

    type = X_PTR_TO_UINT (info);
    if (type_spec) {
        if (type_spec->value_table)
            info->value_table = *type_spec->value_table;
        info->class_size = type_spec->class_size;
        info->class_init = type_spec->class_init;
        info->class_data = type_spec->class_data;
    } else {
        info->plugin = x_object_ref (X_OBJECT(type_plugin));
    }
    info->magic      = TYPE_MAGIC;
    info->name       = x_istr_from_sstr (type_name);
    info->iface_spec = NULL;
    info->klass      = NULL;
    info->n_supers   = n_supers;
    info->ref        = 1;
    info->n_children = 0;
    info->supers[0]  = type;
    if (!pinfo) {
        info->supers[1] = parent_type;
        info->supers[2] = X_TYPE_VOID;
    } else {
        x_atomic_int_inc (&pinfo->ref);
        memcpy (info->supers + 1, pinfo->supers,
                sizeof (xType) * (1 + pinfo->n_supers + 1));
        pinfo->n_children++;
        pinfo->children = x_renew (xType, pinfo->children, pinfo->n_children);
        pinfo->children[pinfo->n_children-1] = type;
    }

    X_LOCK (type_ht);
    if (!type_ht) {
        type_ht = x_hashtbl_new_full (type_hash,
                                      type_cmp,
                                      x_free,
                                      NULL);
    }
    x_hashtbl_insert (type_ht, (xptr)info->name, info);
    X_UNLOCK (type_ht);

    return type;
}

static xuint
iface_hash              (xcptr          key)
{
    const xIFaceSpec *spec = key;
    return spec->iface_type + spec->owner_type;
}

static xint
iface_cmp               (xcptr          key1,
                         xcptr          key2)
{
    const xIFaceSpec *spec1 = key1;
    const xIFaceSpec *spec2 = key2;
    if(spec1->owner_type != spec2->owner_type)
        return spec1->owner_type - spec2->owner_type;
    return spec1->iface_type - spec2->iface_type;
}

xint
x_type_add_iface        (xType          owner_type,
                         xType          iface_type,
                         xInitFunc      iface_init,
                         xptr           iface_data)
{
    xIFaceSpec *spec;
    xTypeInfo* info;

    if (x_type_fundamental(iface_type) != X_TYPE_IFACE)
        return -1;

    info = x_type_info(owner_type);
    x_return_val_if_fail (info, -1);

    spec = x_slice_new (xIFaceSpec);
    spec->owner_type = owner_type;
    spec->iface_type = iface_type;
    spec->iface_init = iface_init;
    spec->iface_data = iface_data;
    spec->iface      = NULL;
    spec->next       = info->iface_spec;
    info->iface_spec = spec;

    X_LOCK (ifaces_ht);
    if (!ifaces_ht)
        ifaces_ht = x_hashtbl_new (iface_hash,
                                   iface_cmp);
    x_hashtbl_add (ifaces_ht, spec);

    X_UNLOCK (ifaces_ht);

    return 0;
}

xint
x_type_ref              (xType          type)
{
    xTypeInfo* info = x_type_info(type);

    x_return_val_if_fail (info, 0);

    return x_atomic_int_inc (&info->ref);
}

xint
x_type_unref            (xType          type)
{
    xint ref;
    xTypeInfo* info = x_type_info(type);

    x_return_val_if_fail (info, 0);

    ref = x_atomic_int_dec (&info->ref);
    if (!ref) {
        x_type_unref (info->supers[1]);
        info->magic = 0;
        X_LOCK (type_ht);
        x_hashtbl_remove (type_ht, (xptr)info->name);
        X_UNLOCK (type_ht);
    }
    return ref;
}

xcstr
x_type_name             (xType          type)
{
    xTypeInfo* info = x_type_info(type);

    x_return_val_if_fail (info, NULL);

    return info->name;
}

xTypePlugin*
x_type_plugin           (xType          type)
{
    xTypeInfo* info = x_type_info(type);

    x_return_val_if_fail (info, NULL);

    return info->plugin;
}

xType
x_type_from_name        (xcstr          name)
{
    xcstr istr;
    xTypeInfo* info;
    x_return_val_if_fail (type_ht, X_TYPE_VOID);

    istr = x_istr_try_str (name);
    x_return_val_if_fail (istr, X_TYPE_VOID);

    X_LOCK (type_ht);
    info = x_hashtbl_lookup (type_ht, istr);
    X_UNLOCK (type_ht);

    return X_PTR_TO_UINT (info);
}

xType 
x_type_parent           (xType          type)
{
    xTypeInfo* info;

    info = x_type_info(type);
    x_return_val_if_fail (info, X_TYPE_VOID);

    return info->supers[1];
}

xType
x_type_fundamental      (xType          type)
{
    xTypeInfo* info;

    info = x_type_info(type);
    x_return_val_if_fail (info, type);

    return info->supers[info->n_supers];
}

xint
x_type_children         (xType          type,
                         xType          *children,
                         xsize          *n_children)
{
    xTypeInfo* info;

    info = x_type_info(type);
    x_return_val_if_fail (info, -1);

    *n_children = MIN (*n_children, info->n_children);
    if (children)
        memcpy (children, info->children, (*n_children)*sizeof(xType));
    return 0;
}

xint
x_type_ifaces           (xType          type,
                         xType          *ifaces,
                         xsize          *n_ifaces)
{
    return 0;
}

xbool
x_type_cast             (xType          type,
                         xType          ancestor)
{
    xint        i;
    xTypeInfo   *info;

    info = x_type_info(type);
    x_return_val_if_fail (info, FALSE);

    i = info->n_supers;
    while (i >= 0) {
        if (info->supers[i] == ancestor)
            return TRUE;
        --i;
    }

    if (x_type_fundamental (ancestor) == X_TYPE_IFACE)
        return x_class_iface(type, ancestor) != NULL;

    return FALSE;
}

xptr
x_class_peek            (xType          type)
{
    xTypeInfo* info;

    info = x_type_info(type);
    x_return_val_if_fail (info, NULL);

    return info->klass;
}

xptr
x_class_ref             (xType          type)
{
    xTypeInfo   *info;

    info = x_type_info (type);
    x_return_val_if_fail (info, NULL);

    if (!x_atomic_ptr_get(&info->klass)
        && x_once_init_enter(&info->klass)) {
        xint        i;
        xIFaceSpec  *iface_spec;

        if (info->n_supers > 1)
            x_class_ref (info->supers[1]);

        iface_spec = info->iface_spec;
        while (iface_spec) {
            xTypeInfo   *iface_info;
            iface_info  = x_type_info (iface_spec->iface_type);
            if (type_complete (iface_info)) {
                xIFace      *iface;
                iface = x_slice_alloc0 (iface_info->class_size);
                iface->type  = iface_spec->iface_type;
                iface->owner_type  = iface_spec->owner_type;
                i = iface_info->n_supers - 1;
                while (i >= 0) {
                    xTypeInfo   *pinfo = x_type_info (iface_info->supers[i]);
                    if (pinfo->class_init)
                        pinfo->class_init (iface, pinfo->class_data);
                    --i;
                }
                if (iface_spec->iface_init)
                    iface_spec->iface_init (iface,
                                            iface_spec->iface_data);
                iface_spec->iface = iface;
            }
            iface_spec = iface_spec->next;
        }

        if (type_complete (info))
        {
            xClass  *klass;
            klass = x_malloc0 (info->class_size);
            klass->type = type;
            klass->ref = 1;
            i = info->n_supers - 1;
            while (i >= 0) {
                xTypeInfo   *pinfo = x_type_info (info->supers[i]);
                if (pinfo->class_init)
                    pinfo->class_init(klass, pinfo->class_data);
                --i;
            }
            info->klass = klass;
        }

        x_once_init_leave(&info->klass);
    } else {
        x_atomic_int_inc(&X_CLASS(info->klass)->ref);
    }

    return X_CLASS(info->klass);
}

void
x_class_unref           (xType          type)
{
    xTypeInfo   *info;

    info = x_type_info(type);
    x_return_if_fail (info);

    if (info->klass && !x_atomic_int_dec(&X_CLASS(info->klass)->ref)) {
        xIFaceSpec  *iface_spec;
        xint        i;

        iface_spec = info->iface_spec;
        while (iface_spec) {
            xTypeInfo   *iface_info;

            if (iface_spec->iface && iface_spec->iface->iface_final) {
                iface_spec->iface->iface_final (iface_spec->iface,
                                                iface_spec->iface_data);
            }

            iface_info = x_type_info (iface_spec->iface_type);
            x_slice_free1 (iface_info->class_size, iface_spec->iface);
            iface_spec->iface = NULL;

            iface_spec = iface_spec->next;
        }

        i = info->n_supers - 1;
        while (i >= 0) {
            xTypeInfo   *pinfo = x_type_info (info->supers[i]);
            if (X_CLASS(pinfo->klass)->class_final)
                X_CLASS(pinfo->klass)->class_final(info->klass, pinfo->class_data);
            --i;
        }
        x_slice_free1 (info->class_size, info->klass);
        info->klass=NULL;

        if (info->n_supers > 1)
            x_class_unref (info->supers[1]);
    }
}

xIFace*
x_class_iface           (xType          owner_type,
                         xType          iface_type)
{
    xIFaceSpec key, *spec;
    x_return_val_if_fail (ifaces_ht, NULL);

    key.iface_type = iface_type;
    key.owner_type = owner_type;

    X_LOCK (ifaces_ht);
    spec = x_hashtbl_lookup (ifaces_ht, &key);
    X_UNLOCK (ifaces_ht); 

    x_return_val_if_fail (spec, NULL);

    return spec->iface;
}

xptr
x_instance_new          (xType          type,
                         ...)
{
    va_list argv;
    xInstance* instance;

    va_start(argv, type);
    instance = x_instance_new_valist (type, argv);
    va_end(argv);

    return instance;
}

xptr
x_instance_new_valist   (xType          type,
                         va_list        argv)
{
    xInstance   *instance;
    xTypeInfo   *info;
    xClass      *klass;
    xint        i;


    info = x_type_info(type);
    x_return_val_if_fail (info, NULL);

    klass = x_class_ref(type);
    x_return_val_if_fail (klass, NULL);

    if (!klass->instance_size) {
        x_class_unref(type);
        return NULL;
    }

    instance = x_slice_alloc0 (klass->instance_size + klass->private_offset);
    x_return_val_if_fail (instance, NULL);

    instance->klass = klass;
    X_LOCK (construction_slist);
    construction_slist = x_slist_prepend (construction_slist,
                                          instance);
    X_UNLOCK (construction_slist);

    i = info->n_supers - 1;
    while (i >= 0) {
        xTypeInfo   *pinfo = x_type_info (info->supers[i]);
        xClass      *pclass= pinfo->klass;
        if (pclass->instance_init)
            pclass->instance_init (instance, pclass->instance_data);
        --i;
    }

    x_param_set_valist(instance, argv);

    X_LOCK (construction_slist);
    construction_slist = x_slist_remove (construction_slist,
                                         instance);
    X_UNLOCK (construction_slist);

    return instance;
}

void
x_instance_delete       (xptr           instance)
{
    xTypeInfo   *info;
    xClass      *klass;
    xint        i;

    x_return_if_fail (instance);

    klass = X_INSTANCE_CLASS(instance);
    x_return_if_fail (klass);

    info = x_type_info (X_INSTANCE_TYPE(instance));
    x_return_if_fail (info);

    i = info->n_supers - 1;
    while (i >= 0) {
        xTypeInfo   *pinfo = x_type_info (info->supers[i]);
        xClass      *pclass= pinfo->klass;
        if (pclass->instance_final)
            pclass->instance_final (instance, pclass->instance_data);
        --i;
    }
    x_signal_clean (instance);
    x_slice_free1 (klass->instance_size, instance);
    x_class_unref (X_CLASS_TYPE(klass));
}

xptr
x_instance_private      (xInstance      *instance)
{
    xTypeInfo   *info;
    xClass      *klass;
    xsize       private_offset;

    x_return_val_if_fail (instance, NULL);

    klass = X_INSTANCE_CLASS(instance);
    x_return_val_if_fail (klass, NULL);

    info = x_type_info (X_INSTANCE_TYPE(instance));
    x_return_val_if_fail (info, NULL);

    private_offset = klass->instance_size;
    if (info->n_supers > 1) {
        xTypeInfo   *pinfo = x_type_info (info->supers[info->n_supers-1]);
        private_offset += X_CLASS (pinfo->klass)->private_offset;
    }
    return  X_STRUCT_MEMBER_P(instance, private_offset);
}
xbool
x_is_constructing       (xInstance      *instance)
{
    xbool constructing;

    X_LOCK (construction_slist);
    constructing = x_slist_find (construction_slist, instance) != NULL;
    X_UNLOCK (construction_slist);

    return constructing;
}

xptr
x_object_ref            (xObject        *object)
{
    x_return_val_if_fail (object, NULL);
    x_atomic_int_inc (&object->ref);
    return object;
}

xint
x_object_unref          (xObject        *object)
{
    xint ref = x_atomic_int_dec (&object->ref);
    if (!ref) {
        x_instance_delete (X_INSTANCE(object));
    }
    return ref;
}

X_DEFINE_TYPE(xTypePlugin, x_type_plugin, X_TYPE_IFACE);

static void
x_type_plugin_class_init(xTypePluginClass   *klass,
                         xptr               klass_data)
{

}

void
x_type_plugin_complete  (xTypePlugin    *plugin,
                         xType          type,
                         xTypeSpec      *type_spec)
{
    xTypePluginClass* klass = X_TYPE_PLUGIN_GET_CLASS(plugin);

    x_return_if_fail (klass);
    klass->complete(plugin, type, type_spec);
}

X_DEFINE_TYPE(xObject, x_object, X_TYPE_INSTANCE);

static xint
object_set_property     (xInstance      *self,
                         xParam         *param,
                         xValue         *value)
{
    return -1;
}

static xint
object_get_property     (xInstance      *self,
                         xParam         *param,
                         xValue         *value)
{
    return -1;
}

static void
object_on_nofity        (xObject        *object,
                         xParam         *param)
{
}

static void 
x_object_init           (xObject        *object,
                         xptr           obj_data)
{
    object->ref = 1;
}

static void
x_object_class_init     (xObjectClass   *klass,
                         xptr           klass_data)
{
    X_CLASS(klass)->set_property    = object_set_property;
    X_CLASS(klass)->get_property    = object_get_property;
    X_CLASS(klass)->property_notify = X_CALLBACK (object_on_nofity);
    X_CLASS(klass)->instance_init   = x_object_init;
}

/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
