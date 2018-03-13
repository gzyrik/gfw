#include "config.h"
#include "xclass.h"
#include "xquark.h"
#include "xmem.h"
#include "xslice.h"
#include "xatomic.h"
#include "xthread.h"
#include "xhash.h"
#include "xmsg.h"
#include "xparam.h"
#include "xslist.h"
#include "xclass-prv.h"
#include <string.h>

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
    xInitFunc      class_init;
    xsize          class_size;
    xptr           class_data;
    xInitFunc      insta_init;
    xsize          insta_size;
    xptr           insta_data;
    xIFaceSpec     *iface_spec;
    xcstr          name;
    xType          parent;
    xClass         *klass;
};

struct _xIFaceSpec
{
    xType          owner_type;
    xType          iface_type;
    xInitFunc      iface_init;
    xptr           iface_data;
    xIFace         *iface;
    xIFaceSpec     *next;
};

static inline xTypeInfo*
x_type_info             (xType          type)
{
    if (type > X_TYPE_INSTANCE) 
        return (xTypeInfo*) type;
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

static void
type_delete             (xptr v)
{
}

xType
x_type_register         (xcstr          type_name,
                         xType          parent_type,
                         xInitFunc      class_init,
                         xsize          class_size,
                         xptr           class_data,
                         xInitFunc      insta_init,
                         xsize          insta_size,
                         xptr           insta_data)
{
    xTypeInfo *info;

    info = x_slice_new (xTypeInfo);
    info->class_init = class_init;
    info->class_size = class_size;
    info->class_data = class_data;
    info->class_size = class_size;
    info->insta_init = insta_init;
    info->insta_size = insta_size;
    info->insta_data = insta_data;
    info->iface_spec = NULL;
    info->name       = x_internal_str (type_name);
    info->parent     = parent_type;
    info->klass      = NULL;

    X_LOCK (type_ht);
    if (!type_ht)
        type_ht = x_hashtbl_new_full (type_hash,
                                      type_cmp,
                                      type_delete,
                                      NULL);
    x_hashtbl_insert (type_ht, (xptr)info->name, info);
    X_UNLOCK (type_ht);

    return X_PTR_TO_UINT (info);
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

xcstr
x_type_name             (xType          type)
{
    xTypeInfo* info = x_type_info(type);

    x_return_val_if_fail (info, NULL);

    return info->name;
}

xType
x_type_from_name        (xcstr          name)
{
    xTypeInfo* info;
    x_return_val_if_fail (type_ht, X_TYPE_VOID);

    X_LOCK (type_ht);
    info = x_hashtbl_lookup (type_ht, x_internal_str (name));
    X_UNLOCK (type_ht);

    return X_PTR_TO_UINT (info);
}

xType
x_type_parent           (xType          type)
{
    xTypeInfo* info = x_type_info(type);
    if (!info) return X_TYPE_VOID;
    return info->parent;
}

xint
x_type_children         (xType          type,
                         xType          *children,
                         xsize          *n_children)
{
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
x_instance_is           (xInstance      *instance,
                         xType          type)
{
    return x_class_is (instance->klass, type);
}

xInstance*
x_instance_cast         (xInstance      *instance,
                         xType          type)
{
    if (x_instance_is (instance, type))
        return instance;
    return NULL;
}

xbool
x_class_is              (xClass         *klass,
                         xType          type)
{
    xType parent=klass->type;
    while (parent > X_TYPE_INSTANCE) {
        if(parent == type)
            return TRUE;
        parent = x_type_parent (parent);
    }
    return parent == type;
}

xClass*
x_class_cast            (xClass         *klass,
                         xType          type)
{
    if (x_class_is (klass, type))
        return klass;
    return NULL;
}

xClass*
x_class_peek            (xType          type)
{
    xTypeInfo* info = x_type_info(type);
    if (!info) return NULL;
    return info->klass;
}

static void
x_class_init            (xType          type,
                         xptr           klass)
{
    xIFaceSpec  *iface_spec;
    xTypeInfo   *iface_info;
    xptr        iface_class;

    xTypeInfo* info = x_type_info (type);

    if (info->parent > X_TYPE_INSTANCE)
        x_class_init(info->parent, klass);

    iface_spec = info->iface_spec;
    while (iface_spec) {
        iface_info  = x_type_info (iface_spec->iface_type);
        iface_class = x_class_ref (iface_spec->iface_type);

        iface_spec->iface = x_slice_alloc0 (iface_info->class_size);
        iface_spec->iface->owner_type  = type;
        memcpy (iface_spec->iface, iface_class, iface_info->class_size);

        if (iface_spec->iface_init) {
            iface_spec->iface_init (iface_spec->iface,
                                    iface_spec->iface_data);
        }
        iface_spec = iface_spec->next;
    }
    if (info->class_init)
        info->class_init(klass, info->class_data);
}

xClass*
x_class_ref             (xType          type)
{
    xTypeInfo* info = x_type_info (type);
    if (!info) return NULL;

    if (!x_atomic_ptr_get(&info->klass)
        && x_once_init_enter(&info->klass)) {
        if (info->parent > X_TYPE_INSTANCE)
            x_class_ref (info->parent);

        info->klass = x_malloc0 (info->class_size);
        info->klass->type = type;
        info->klass->ref = 1;
        x_class_init (type, info->klass);

        x_once_init_leave(&info->klass);
    } else {
        x_atomic_int_inc(&info->klass->ref);
    }
    return (xClass*)info->klass;
}

static void
x_class_final           (xType          type,
                         xClass         *klass)
{
    xIFaceSpec  *iface_spec;
    xTypeInfo   *iface_info;

    xTypeInfo *info=x_type_info(type);

    if(klass->class_final)
        klass->class_final(klass, info->class_data);

    iface_spec = info->iface_spec;
    while (iface_spec) {

        if (iface_spec->iface && iface_spec->iface->iface_final) {
            iface_spec->iface->iface_final (iface_spec->iface,
                                            iface_spec->iface_data);
        }

        iface_info = x_type_info (iface_spec->iface_type);
        x_slice_free1 (iface_info->class_size, iface_spec->iface);
        iface_spec->iface = NULL;

        x_class_unref (iface_spec->iface_type);
    }
    if (type > X_TYPE_INSTANCE)
        x_class_final(info->parent, klass);
}

void
x_class_unref           (xType          type)
{
    xClass* klass;
    xTypeInfo *info = x_type_info(type);
    if (!info) return;

    klass = x_atomic_ptr_get(&info->klass);
    if (klass) {
        if (!x_atomic_int_dec(&klass->ref)) {

            info->klass=NULL;
            x_class_final (type, klass);
            x_slice_free1 (info->class_size, klass);

            if (info->parent > X_TYPE_INSTANCE)
                x_class_unref (info->parent);
        }
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

static void 
x_instance_init         (xType          type,
                         xInstance      *instance)
{
    xTypeInfo *info = x_type_info(type);
    if (info->parent > X_TYPE_INSTANCE)
        x_instance_init (info->parent, instance);
    if (info->insta_init)
        info->insta_init (instance, info->insta_data);
}


xInstance*
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

xInstance*
x_instance_new_valist   (xType          type,
                         va_list        argv)
{
    xInstance* instance;
    xTypeInfo* info;

    x_return_val_if_fail (type > X_TYPE_INSTANCE, NULL);

    info = x_type_info(type);
    instance = x_slice_alloc0 (info->insta_size);
    x_return_val_if_fail (instance, NULL);

    X_LOCK (construction_slist);
    construction_slist = x_slist_prepend (construction_slist,
                                          instance);
    X_UNLOCK (construction_slist);

    instance->klass = x_class_ref(type);
    x_instance_init(type, instance);

    x_param_set_valist(instance, argv);

    X_LOCK (construction_slist);
    construction_slist = x_slist_remove (construction_slist,
                                         instance);
    X_UNLOCK (construction_slist);
    return instance;
}

static void
x_instance_final        (xType          type,
                         xInstance      *instance)
{
    xTypeInfo *info = x_type_info(type);
    x_return_if_fail (info && info->klass);

    if (info->klass->insta_final)
        info->klass->insta_final (instance, info->insta_data);

    if (type > X_TYPE_INSTANCE)
        x_instance_final (info->parent, instance);
}

void
x_instance_delete       (xInstance      *instance)
{
    xType type;
    xTypeInfo *info;

    type = X_INSTANCE_TYPE(instance);
    x_return_if_fail (instance);

    x_return_if_fail (type > X_TYPE_INSTANCE);
    x_instance_final  (type, instance);

    info = x_type_info (type);
    x_slice_free1 (info->insta_size, instance);

    x_class_unref (type);
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

X_DEFINE_TYPE(xObject, x_object, X_TYPE_INSTANCE);

xint
x_object_ref            (xObject        *obj)
{
    x_return_val_if_fail (X_IS_OBJECT(obj), -1);

    return x_atomic_int_inc (&obj->ref);
}

xint
x_object_unref          (xObject        *obj)
{
    xint ref;
    x_return_val_if_fail (X_IS_OBJECT(obj), -1);

    ref = x_atomic_int_dec (&obj->ref);
    if (!ref)
        x_instance_delete (X_INSTANCE(obj));
    return ref;
}

static void
x_object_class_init     (xObjectClass   *klass,
                         xptr           class_data)
{
}

static void
x_object_init           (xObject        *object,
                         xptr           insta_data)
{
    object->ref = 1;
}

/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
