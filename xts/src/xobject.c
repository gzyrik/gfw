#include "config.h"
#include "xobject.h"
#include "xsignal.h"
#include "xvalue.h"
#include "xparam.h"
#include "xclass-prv.h"
#include "xvalist.h"
#include <string.h>

#define OBJECT_FLOATING_FLAG            0x2

#define CLASS_HAS_DERIVED_CLASS_FLAG    0x2
#define CLASS_HAS_PROPS_FLAG            0x1

#define CLASS_HAS_PROPS(klass)                                          \
    ((klass)->flags & CLASS_HAS_PROPS_FLAG)

#define CLASS_HAS_DERIVED_CLASS(klass)                                  \
    ((klass)->flags & CLASS_HAS_DERIVED_CLASS_FLAG)

#define CLASS_HAS_CUSTOM_CONSTRUCTOR(klass)                             \
    ((klass)->constructor != object_constructor)

#define CLASS_HAS_CUSTOM_CONSTRUCTED(klass)                             \
    ((klass)->constructed != object_constructed)
enum {
    NOTIFY,
    LAST_SIGNAL
};
typedef struct {
    xObject         *object;
    xuint           n_weak_refs;
    struct {
        xCallback   notify;
        xptr        data;
    } weak_refs[1];  /* flexible array */
} WeakRefStack;
static xQuark   _quark_weak_refs;
static void
weak_refs_notify (xptr data)
{
    WeakRefStack *wstack = data;
    xuint i;

    for (i = 0; i < wstack->n_weak_refs; i++)
        wstack->weak_refs[i].notify (wstack->weak_refs[i].data, wstack->object);
    x_free (wstack);
}

static xuint	        _object_signals[LAST_SIGNAL] = { 0, };
/* 构造中对象链表 */
X_LOCK_DEFINE_STATIC    (construction_objects);
X_LOCK_DEFINE_STATIC    (weak_refs_mutex);
static xSList           *construction_objects;
static inline xbool
object_in_construction  (xObject        *object)
{
    xbool   in_construction;
    X_LOCK (construction_objects);
    in_construction = x_slist_find (construction_objects, object) != NULL;
    X_UNLOCK (construction_objects);
    return in_construction;
}
static xbool
object_floating         (xObject        *object,
                         xint           job)
{
    xptr oldvalue;
    switch (job) {
    case +1:    /* force floating if possible */
        do {
            oldvalue = x_atomic_ptr_get (&object->qdata);
        } while (!x_atomic_ptr_cas ((void**) &object->qdata, oldvalue,
                                    (xptr) ((xsize) oldvalue | OBJECT_FLOATING_FLAG)));
        return (xsize) oldvalue & OBJECT_FLOATING_FLAG;
    case -1:    /* sink if possible */
        do
            oldvalue = x_atomic_ptr_get (&object->qdata);
        while (!x_atomic_ptr_cas ((void**) &object->qdata, oldvalue,
                                  (xptr) ((xsize) oldvalue & ~(xsize) OBJECT_FLOATING_FLAG)));
        return (xsize) oldvalue & OBJECT_FLOATING_FLAG;
    default:    /* check floating */
        return 0 != ((xsize) x_atomic_ptr_get (&object->qdata) & OBJECT_FLOATING_FLAG);
    }
}

/* 属性池 */
X_LOCK_DEFINE_STATIC    (param_ht);
static xHashTable       *_param_ht;
static xuint
param_hash              (xcptr          key)
{
    const xParam *param = key;
    return param->owner_type + X_PTR_TO_SIZE(param->name);
}
static xint
param_cmp               (xcptr          key1,
                         xcptr          key2)
{
    const xParam *param1 = key1;
    const xParam *param2 = key2;
    if(param1->owner_type != param2->owner_type)
        return param1->owner_type - param2->owner_type;
    return param1->name - param2->name;
}
static void
param_pool_insert       (xParam         *pspec,
                         xType          owner_type)
{
    x_return_if_fail (pspec);
    x_return_if_fail (owner_type > 0);
    x_return_if_fail (pspec->owner_type == 0);

    pspec->owner_type = owner_type;
    x_param_sink (pspec);
    X_LOCK (param_ht);
    x_hash_table_add (_param_ht, pspec);
    X_UNLOCK (param_ht);
}
static void
param_pool_remove       (xParam         *pspec)
{
    x_return_if_fail (pspec);

    X_LOCK (param_ht);
    if (x_hash_table_remove (_param_ht, pspec))
        x_param_unref (pspec);
    else
        x_warning ("%s: attempt to remove unknown pspec `%s'",
                   X_STRLOC, x_quark_str(pspec->name));
    X_UNLOCK (param_ht);
}
static xParam*
param_pool_lookup_L     (xQuark         param_name,
                         xType          owner_type,
                         xbool          walk_ancestors)
{
    xParam  param, *ret;
    param.name = param_name;
    param.owner_type = owner_type;

    if (walk_ancestors) {
        do {
            ret = x_hash_table_lookup (_param_ht,  &param);
            if (ret ) return ret;
            param.owner_type = x_type_parent (param.owner_type);
        } while (param.owner_type);
    }
    else {
        ret = x_hash_table_lookup (_param_ht,  &param);
    }
    return ret;
}
#define param_pool_lookup(name, type, ancestors)                        \
    param_pool_lookup_id (x_quark_try(name), type, ancestors);
static xParam*
param_pool_lookup_id    (xQuark         param_name,
                         xType          owner_type,
                         xbool          walk_ancestors)
{
    xParam  *ret;
    if (!param_name)
        return NULL;

    X_LOCK (param_ht);
    ret = param_pool_lookup_L (param_name, owner_type, walk_ancestors);
    X_UNLOCK (param_ht); 

    return ret;
}
static void
param_depth_list_for_iface (xptr        key,
                            xptr        value,
                            xptr        user_data)
{
    xParam          *pspec = value;
    xptr            *data = user_data;
    xSList          **slists = data[0];
    xType           owner_type = (xType) data[1];

    if (pspec->owner_type == owner_type)
        slists[0] = x_slist_prepend (slists[0], pspec);
}
static void
param_depth_list        (xptr           key,
                         xptr           value,
                         xptr           user_data)
{
    xParam          *pspec = value;
    xptr            *data   = user_data;
    xSList          **slists = data[0];
    xType           owner_type = (xType) data[1];

    if (x_type_is (owner_type, pspec->owner_type)) {
        if (X_TYPE_IS_IFACE(pspec->owner_type)) {
            slists[0] = x_slist_prepend (slists[0], pspec);
        }
        else {
            xsize d = x_type_depth (pspec->owner_type);

            slists[d - 1] = x_slist_prepend (slists[d - 1], pspec);
        }
    }
}
static inline xSList*
param_list_remove_L     (xSList             *plist,
                         xType              owner_type,
                         xsize              *n_p)
{
    xSList *rlist = NULL;

    while (plist) {
        xSList *tmp     = plist->next;
        xParam *pspec   = plist->data;
        xParam *found;
        xbool   remove = FALSE;

        if (x_param_get_redirect (pspec))
            remove = TRUE;
        else {
            found = param_pool_lookup_L (pspec->name, owner_type, TRUE);
            if (found != pspec) {
                xParam *redirect = x_param_get_redirect (found);
                if (redirect != pspec)
                    remove = TRUE;
            }
        }

        if (remove) {
            x_slist_free1 (plist);
        }
        else {
            plist->next = rlist;
            rlist = plist;
            *n_p += 1;
        }
        plist = tmp;
    }
    return rlist;
}
static xint
param_id_cmp            (xcptr          a,
                         xcptr          b,
                         ...)
{
    const xParam    *pspec1 = a, *pspec2 = b;

    if (pspec1->param_id < pspec2->param_id)
        return -1;

    if (pspec1->param_id > pspec2->param_id)
        return 1;

    return pspec1->name - pspec2->name;
}

static xParam**
param_pool_list         (xType          owner_type,
                         xsize          *n_pspecs_p)
{
    xParam  **pspecs, **p;
    xSList  **slists, *node;
    xptr    data[2];
    xCallback callback;
    xuint d, i;

    x_return_val_if_fail (owner_type > 0, NULL);
    x_return_val_if_fail (n_pspecs_p != NULL, NULL);

    X_LOCK (param_ht);
    *n_pspecs_p = 0;
    d = x_type_depth (owner_type);
    slists = x_new0 (xSList*, d);
    data[0] = slists;
    data[1] = (xptr) owner_type;

    callback = X_CALLBACK(X_TYPE_IS_IFACE (owner_type) ? param_depth_list_for_iface : param_depth_list);
    x_hash_table_foreach (_param_ht, callback, &data);

    for (i = 0; i < d; i++)
        slists[i] = param_list_remove_L (slists[i], owner_type, n_pspecs_p);
    pspecs = x_new (xParam*, *n_pspecs_p + 1);
    p = pspecs;
    for (i = 0; i < d; i++)
    {
        slists[i] = x_slist_sort (slists[i], param_id_cmp);
        for (node = slists[i]; node; node = node->next)
            *p++ = node->data;
        x_slist_free (slists[i]);
    }
    *p++ = NULL;
    x_free (slists);
    X_UNLOCK (param_ht);

    return pspecs;
}
static xbool
owner_param_remove (xParam *pspec, xptr value, xptr owner_type)
{
    return pspec->owner_type == (xType)owner_type;
}

static void
param_pool_destroy      (xType          owner_type)
{
    X_LOCK (param_ht);
    x_hash_table_foreach_remove (_param_ht, X_WALKFUNC(owner_param_remove), (xptr)owner_type);
    X_UNLOCK (param_ht);
}
static inline void
install_property        (xType          type,
                         xuint          property_id,
                         xParam         *pspec)
{
    if (param_pool_lookup_id (pspec->name, type, FALSE)) {
        x_warning ("When installing property: type `%s' "
                   "already has a property named `%s'",
                   x_type_name (type),
                   x_quark_str (pspec->name));
        return;
    }
    pspec->param_id = property_id;
    param_pool_insert (pspec, type);
}
/* 通知队列 */
typedef struct {
    xSList  *pspecs;
    xsize   n_pspecs;
    xint    freeze_count;
} NotifyQueue;
X_LOCK_DEFINE_STATIC    (notify_queue);
static xQuark           _quark_notify_queue;
static void
notify_queue_free       (xptr           data)
{
    NotifyQueue *nqueue = data;

    x_slist_free (nqueue->pspecs);
    x_slice_free (NotifyQueue, nqueue);
}

static NotifyQueue*
notify_queue_freeze     (xObject        *object,
                         xbool          conditional)
{
    NotifyQueue     *nqueue;

    X_LOCK(notify_queue);
    nqueue = x_data_list_id_get_data (&object->qdata, _quark_notify_queue);
    if (!nqueue) {
        if (conditional) {
            X_UNLOCK(notify_queue);
            return NULL;
        }

        nqueue = x_slice_new0 (NotifyQueue);
        x_data_list_id_set_data_full (&object->qdata, _quark_notify_queue,
                                      nqueue, notify_queue_free);
    }

    if (nqueue->freeze_count >= 0xFFFF)
        x_critical("Free queue for %s (%p) is larger than 65535,"
                   " called g_object_freeze_notify() too often."
                   " Forgot to call g_object_thaw_notify() or infinite loop",
                   X_OBJECT_TYPE_NAME (object), object);
    else
        nqueue->freeze_count++;
    X_UNLOCK(notify_queue);
    return nqueue;
}
static void
notify_queue_thaw       (xObject        *object,
                         NotifyQueue    *nqueue)
{
    xParam  *pspecs_mem[16], **pspecs, **free_me = NULL;
    xSList  *slist;
    xsize   n_pspecs = 0;

    x_return_if_fail (nqueue->freeze_count > 0);
    x_return_if_fail (x_atomic_int_get(&object->ref_count) > 0);

    X_LOCK(notify_queue);

    /* Just make sure we never get into some nasty race condition */
    if (X_UNLIKELY(nqueue->freeze_count == 0)) {
        X_UNLOCK(notify_queue);
        x_warning ("%s: property-changed notification for %s(%p) is not frozen",
                   X_STRFUNC, X_OBJECT_TYPE_NAME (object), object);
        return;
    }

    nqueue->freeze_count--;
    if (nqueue->freeze_count) {
        X_UNLOCK(notify_queue);
        return;
    }

    pspecs = nqueue->n_pspecs > 16 ?
        free_me = x_new (xParam*, nqueue->n_pspecs) : pspecs_mem;

    for (slist = nqueue->pspecs; slist; slist = slist->next) {
        pspecs[n_pspecs++] = slist->data;
    }
    x_data_list_id_remove_data (&object->qdata, _quark_notify_queue);

    X_UNLOCK(notify_queue);

    if (n_pspecs)
        X_OBJECT_GET_CLASS (object)->dispatch_properties_changed (object,
                                                                  n_pspecs,
                                                                  pspecs);
    x_free (free_me);
}
static void
notify_queue_add        (xObject        *object,
                         NotifyQueue    *nqueue,
                         xParam         *pspec)
{
    x_return_if_fail (nqueue->n_pspecs < 0xFFFF);

    X_LOCK(notify_queue);

    if (x_slist_find (nqueue->pspecs, pspec) == NULL) {
        nqueue->pspecs = x_slist_prepend (nqueue->pspecs, pspec);
        nqueue->n_pspecs++;
    }

    X_UNLOCK(notify_queue);
}
static inline xParam*
get_notify_pspec        (xParam         *pspec)
{
    xParam  *redirected;

    /* we don't notify on non-READABLE parameters */
    if (~pspec->flags & X_PARAM_READABLE)
        return NULL;

    /* if the paramspec is redirected, notify on the target */
    redirected = x_param_get_redirect(pspec);
    if (redirected != NULL)
        return redirected;

    /* else, notify normally */
    return pspec;
}

static inline void
object_set_param_value  (xObject        *object,
                         xParam         *pspec,
                         const xValue   *value,
                         NotifyQueue    *nqueue)
{
    static xcstr    enable_diagnostic = NULL;
    xObjectClass    *klass = x_class_peek (pspec->owner_type);
    xuint   param_id  = pspec->param_id;
    xValue  tmp_value = X_VALUE_INIT;
    xParam *redirect;

    if (klass == NULL) {
        x_warning ("'%s::%s' is not a valid property name; "
                   "'%s' is not a xObject subtype",
                   x_type_name (pspec->owner_type),
                   x_quark_str (pspec->name),
                   x_type_name (pspec->owner_type));
        return;
    }

    redirect = x_param_get_redirect (pspec);
    if (redirect)
        pspec = redirect;    

    if (X_UNLIKELY (!enable_diagnostic)) {
        enable_diagnostic = x_getenv ("X_ENABLE_DIAGNOSTIC");
        if (!enable_diagnostic)
            enable_diagnostic = "0";
    }
    if (enable_diagnostic[0] == '1')
    {
        if (pspec->flags & X_PARAM_DEPRECATED)
            x_warning ("The property %s:%s is deprecated and shouldn't be used "
                       "anymore. It will be removed in a future version.",
                       X_OBJECT_TYPE_NAME (object), x_quark_str (pspec->name));
    }
    /* provide a copy to work from, convert (if necessary) and validate */
    x_value_init (&tmp_value, pspec->value_type);
    if (!x_value_transform (value, &tmp_value)) {
        x_warning ("unable to set property `%s' of "
                   "type `%s' from value of type `%s'",
                   x_quark_str (pspec->name),
                   x_type_name (pspec->value_type),
                   X_VALUE_TYPE_NAME (value));
    }
    else if (x_param_value_validate (pspec, &tmp_value) &&
             !(pspec->flags & X_PARAM_LAX_VALIDATION)) {
        xchar *contents = x_strdup_value_contents (value);

        x_warning ("value \"%s\" of type `%s' is invalid or out of range for property `%s' of type `%s'",
                   contents,
                   X_VALUE_TYPE_NAME (value),
                   x_quark_str (pspec->name),
                   x_type_name (pspec->value_type));
        x_free (contents);
    }
    else {
        xParam      *notify_pspec;
        klass->set_property (object, param_id, &tmp_value, pspec);
        notify_pspec = get_notify_pspec (pspec);

        if (notify_pspec != NULL)
            notify_queue_add (object, nqueue, notify_pspec);
    }
    x_value_clear (&tmp_value);
}
static inline void
object_get_param_value  (xObject        *object,
                         xParam         *pspec,
                         xValue         *value)
{
    xObjectClass *klass = x_class_peek (pspec->owner_type);
    xuint param_id = pspec->param_id;
    xParam  *redirect;

    if (klass == NULL) {
        x_warning ("'%s::%s' is not a valid property name; "
                   "'%s' is not a xObject subtype",
                   x_type_name (pspec->owner_type),
                   x_quark_str (pspec->name),
                   x_type_name (pspec->owner_type));
        return;
    }

    redirect = x_param_get_redirect (pspec);
    if (redirect)
        pspec = redirect; 

    klass->get_property (object, param_id, value, pspec);
}
static inline xParam*
get_notify_param        (xParam         *pspec)
{
    xParam  *redirected;

    /* we don't notify on non-READABLE parameters */
    if (~pspec->flags & X_PARAM_READABLE)
        return NULL;

    /* if the paramspec is redirected, notify on the target */
    redirected = x_param_get_redirect (pspec);
    if (redirected != NULL)
        return redirected;

    /* else, notify normally */
    return pspec;
}

static inline void
object_notify_internal  (xObject        *object,
                         xParam         *pspec)
{
    xParam      *notify_pspec;

    notify_pspec = get_notify_pspec (pspec);

    if (notify_pspec != NULL) {
        NotifyQueue *nqueue;
        nqueue = notify_queue_freeze (object, TRUE);

        if (nqueue != NULL) {
            /* we're frozen, so add to the queue and release our freeze */
            notify_queue_add (object, nqueue, notify_pspec);
            notify_queue_thaw (object, nqueue);
        }
        else {
            /* not frozen, so just dispatch the notification directly */
            X_OBJECT_GET_CLASS (object)
                ->dispatch_properties_changed (object, 1, &notify_pspec);
        }
    }
}

static xptr
object_constructor      (xType          type,
                         xsize          n_properties,
                         xConstructParam *params)
{
    xObject *object;

    object = (xObject*) x_instance_new (type);

    if (n_properties) {
        NotifyQueue *nqueue = notify_queue_freeze (object, FALSE);

        while (n_properties--) {
            xValue  *value = params->value;
            xParam  *pspec = params->pspec;

            params++;
            object_set_param_value (object, pspec, value, nqueue);
        }
        notify_queue_thaw (object, nqueue);
        /* the notification queue is still frozen from object_init(), so
         * we don't need to handle it here, x_object_newv() takes
         * care of that
         */
    }

    return object;

}
static void
object_constructed      (xObject        *object)
{
    /* empty default impl to allow unconditional upchaining */
}

static xbool
slist_maybe_remove      (xSList         **slist,
                         xcptr          data)
{
    xSList *last = NULL, *node = *slist;
    while (node) {
        if (node->data == data)
        {
            if (last)
                last->next = node->next;
            else
                *slist = node->next;
            x_slist_free1 (node);
            return TRUE;
        }
        last = node;
        node = last->next;
    }
    return FALSE;
}

static void
object_base_init        (xObjectClass   *klass)
{
    xObjectClass *pclass = x_class_peek_parent (klass);

    /* Don't inherit HAS_DERIVED_CLASS flag from parent class */
    klass->flags &= ~CLASS_HAS_DERIVED_CLASS_FLAG;

    if (pclass)
        pclass->flags |= CLASS_HAS_DERIVED_CLASS_FLAG;

    /* reset instance specific fields and methods that don't get inherited */
    klass->construct_properties =
        pclass ? x_slist_copy (pclass->construct_properties) : NULL;
    klass->get_property = NULL;
    klass->set_property = NULL;
}
static void
object_base_finalize    (xObjectClass    *klass)
{
    _x_signals_destroy (X_CLASS_TYPE (klass));

    x_slist_free (klass->construct_properties);
    klass->construct_properties = NULL;

    param_pool_destroy (X_CLASS_TYPE (klass));
}
static void
object_set_property     (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{

}
static void
object_get_property     (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{

}
static void
object_dispatch_properties_changed  (xObject            *object,
                                     xsize              n_pspecs,
                                     xParam             **pspecs)
{
    xsize i;

    for (i = 0; i < n_pspecs; i++) {
        x_signal_emit (object, _object_signals[NOTIFY],
                       pspecs[i]->name, pspecs[i]);
    }
}
static void
object_dispose          (xObject        *object)
{
}
static void
object_finalize         (xObject        *object)
{
    x_data_list_clear (&object->qdata);
}
static void
object_class_init       (xObjectClass   *klass,
                         xptr           klass_data)
{
    _param_ht = x_hash_table_new (param_hash, param_cmp);
    _quark_notify_queue = x_quark_from_static("xObject-notify-queue");
    _quark_weak_refs = x_quark_from_static ("xObject-weak-references");
    klass->constructor  = object_constructor;
    klass->constructed  = object_constructed;
    klass->set_property = object_set_property;
    klass->get_property = object_get_property;
    klass->dispose      = object_dispose;
    klass->finalize     = object_finalize;
    klass->dispatch_properties_changed = object_dispatch_properties_changed;
    klass->notify       = NULL;
    _object_signals[NOTIFY] =
        x_signal_new (x_intern_static_str ("notify"),
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_FIRST | X_SIGNAL_NO_RECURSE |
                      X_SIGNAL_DETAILED | X_SIGNAL_NO_HOOKS | X_SIGNAL_ACTION,
                      X_STRUCT_OFFSET (xObjectClass, notify),
                      NULL, NULL,
                      X_TYPE_VOID,
                      1, X_TYPE_PARAM);

}
static void 
object_init             (xObject        *object,
                         xObjectClass   *klass)
{
    object->ref_count = 1;
    x_data_list_init (&object->qdata);
    if (CLASS_HAS_PROPS (klass)) {
        /* freeze object's notification queue,
         * x_object_newv() preserves pairedness
         */
        notify_queue_freeze (object, FALSE);
    }

    if (CLASS_HAS_CUSTOM_CONSTRUCTOR (klass)) {
        /* enter construction list for notify_queue_thaw() and to allow construct-only properties */
        X_LOCK (construction_objects);
        construction_objects = x_slist_prepend (construction_objects, object);
        X_UNLOCK (construction_objects);
    }
}
static void
object_value_init       (xValue         *value)
{
    value->data[0].v_pointer = NULL;
}

static void
object_value_free       (xValue         *value)
{
    if (value->data[0].v_pointer)
        x_object_unref (value->data[0].v_pointer);
    value->data[0].v_pointer = NULL;
}

static void
object_value_copy       (const xValue   *src,
                         xValue	        *dest)
{
    if (src->data[0].v_pointer)
        dest->data[0].v_pointer = x_object_ref (src->data[0].v_pointer);
    else
        dest->data[0].v_pointer = NULL;
}
static xptr
object_value_peek       (const xValue   *value)
{
    return value->data[0].v_pointer;
}
static xstr
object_value_collect    (xValue         *value,
                         xsize          n_collect,
                         xCValue        *cvalues,
                         xuint          flags)
{
    if (cvalues[0].v_pointer) {
        xObject *object = cvalues[0].v_pointer;

        if (X_OBJECT_GET_CLASS(object) == NULL)
            return x_strdup_printf ("invalid unclassed object pointer "
                                    "for value type `%s'",
                                    X_VALUE_TYPE_NAME (value));
        else if (!x_value_type_compatible (X_INSTANCE_TYPE (object), X_VALUE_TYPE (value)))
            return x_strdup_printf ("invalid object type `%s'"
                                    "' for value type `%s'",
                                    X_INSTANCE_TYPE_NAME (object),
                                    X_VALUE_TYPE_NAME (value));
        /* never honour X_VALUE_NOCOPY_CONTENTS for ref-counted types */
        value->data[0].v_pointer = x_object_ref (object);
    }
    else
        value->data[0].v_pointer = NULL;

    return NULL;
}
static xstr
object_value_lcopy      (const xValue   *value,
                         xsize          n_collect,
                         xCValue        *cvalues,
                         xuint          flags)
{
    xObject **object_p = cvalues[0].v_pointer;

    if (!object_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (value));

    if (!value->data[0].v_pointer)
        *object_p = NULL;
    else if (flags & X_VALUE_NOCOPY_CONTENTS)
        *object_p = value->data[0].v_pointer;
    else
        *object_p = x_object_ref (value->data[0].v_pointer);

    return NULL;
}
static void
object_value_transform  (const xValue   *src,
                         xValue         *dest)
{
    if (x_instance_is (src->data[0].v_pointer, X_VALUE_TYPE (dest)))
        dest->data[0].v_pointer = x_object_ref (src->data[0].v_pointer);
    else
        dest->data[0].v_pointer = NULL;
}

void
_x_object_type_init     (void)
{
    const xFundamentalInfo finfo = {
        X_TYPE_CLASSED | X_TYPE_INSTANTIATABLE
            | X_TYPE_DERIVABLE | X_TYPE_DEEP_DERIVABLE,
    };
    const xValueTable value_table = {
        object_value_init,
        object_value_free,
        object_value_copy,
        object_value_peek,
        X_VALUE_COLLECT_PTR,
        object_value_collect,
        X_VALUE_COLLECT_PTR,
        object_value_lcopy,
    };
    const xTypeInfo info = {
        sizeof (xObjectClass),
        (xBaseInitFunc)object_base_init,
        (xBaseFinalizeFunc)object_base_finalize,
        (xClassInitFunc)object_class_init,
        NULL,
        NULL,
        sizeof (xObject),
        (xInstanceInitFunc)object_init,
        &value_table,
    };
    x_type_register_fundamental (X_TYPE_OBJECT,
                                 "xObject",
                                 &info,
                                 &finfo,
                                 0);
    x_value_register_transform (X_TYPE_OBJECT,
                                X_TYPE_OBJECT,
                                (xValueTransformFunc)object_value_transform);
}
/* API */
xptr
x_object_new_json       (xType          type,
                         xcstr          json);
xptr
x_object_new            (xType          type,
                         xcstr          first_property,
                         ...)
{
    va_list     argv;
    xObject     *object;

    x_return_val_if_fail (X_TYPE_IS_OBJECT (type), NULL);
    if (!first_property)
        return x_object_newv (type, 0, NULL);

    va_start(argv, first_property);
    object = x_object_new_valist (type, first_property, argv);
    va_end(argv);

    return object;
}
xptr
x_object_new_valist1    (xType          type,
                         xcstr          first_property,
                         va_list        argv1,
                         xcstr          other_property,
                         ...)
{
    va_list     argv2;
    xObject     *object;

    x_return_val_if_fail (X_TYPE_IS_OBJECT (type), NULL);
    if (!other_property)
        return x_object_new_valist (type, first_property, argv1);

    va_start(argv2, other_property);
    object = x_object_new_valist2 (type,
                                   first_property, argv1,
                                   other_property, argv2);
    va_end(argv2);

    return object;
}
static xParamValue*
alloc_paramv            (xParamValue    *params,
                         xType          type,
                         xcstr          first_property,
                         va_list        var_args,
                         xuint          *n_params,
                         xuint          *n_alloced_params)
{
    xcstr name = first_property;
    while (name) {
        xstr error = NULL;
        xParam *pspec;
        pspec = param_pool_lookup (name, type, TRUE);
        if (!pspec) {
            x_warning ("%s: object class `%s' has no property named `%s'",
                       X_STRFUNC,
                       x_type_name (type),
                       name);
            break;
        }
        if ((*n_params) >= (*n_alloced_params)) {
            (*n_alloced_params) += 16;
            params = x_renew (xParamValue, params, (*n_alloced_params));
            memset (params + (*n_params), 0, 16 * (sizeof *params));
        }
        params[(*n_params)].name = name;
        X_VALUE_COLLECT_INIT (&params[(*n_params)].value,
                              pspec->value_type, var_args, 0, error);
        if (error) {
            x_warning ("%s: %s", X_STRFUNC, error);
            x_free (error);
            x_value_clear (&params[(*n_params)].value);
            break;
        }
        (*n_params)++;
        name = va_arg (var_args, xcstr);
    }
    return params;
}
xptr
x_object_new_valist2    (xType          type,
                         xcstr          first_property,
                         va_list        argv1,
                         xcstr          other_property,
                         va_list        argv2)
{
    xObjectClass        *klass;
    xParamValue         *params;
    xObject             *object;
    xuint n_params = 0, n_alloced_params = 16;

    x_return_val_if_fail (X_TYPE_IS_OBJECT (type), NULL);

    if (!first_property && !other_property)
        return x_object_newv (type, 0, NULL);
    else if (!other_property)
        return x_object_new_valist (type, first_property, argv1);

    klass = x_class_ref (type);

    params = x_new0 (xParamValue, n_alloced_params);
    params = alloc_paramv (params, type,
                           first_property, argv1,
                           &n_params, &n_alloced_params);
    params = alloc_paramv (params, type,
                           other_property, argv2,
                           &n_params, &n_alloced_params);

    object = x_object_newv (type, n_params, params);

    while (n_params--)
        x_value_clear (&params[n_params].value);
    x_free (params);

    x_class_unref (klass);

    return object;
}
xptr
x_object_new_valist     (xType          type,
                         xcstr          first_property,
                         va_list        var_args)
{
    xObjectClass        *klass;
    xParamValue         *params;
    xObject             *object;
    xuint n_params = 0, n_alloced_params = 16;

    x_return_val_if_fail (X_TYPE_IS_OBJECT (type), NULL);

    if (!first_property)
        return x_object_newv (type, 0, NULL);

    klass = x_class_ref (type);

    params = x_new0 (xParamValue, n_alloced_params);
    params = alloc_paramv (params, type,
                           first_property, var_args,
                           &n_params, &n_alloced_params);

    object = x_object_newv (type, n_params, params);

    while (n_params--)
        x_value_clear (&params[n_params].value);
    x_free (params);

    x_class_unref (klass);

    return object;
}
xptr
x_object_newv           (xType          type,
                         xsize          n_parameters,
                         xParamValue    *parameters)
{
    xConstructParam     *cparams,*oparams;
    NotifyQueue        *nqueue;
    xObject             *object;
    xObjectClass        *klass, *unref_class;
    xSList              *slist;
    xuint               n_total_cparams, n_cparams, n_oparams, n_cvalues;
    xValue              *cvalues;
    xList               *clist = NULL;
    xbool               newly_constructed;
    xuint               i;

    x_return_val_if_fail (X_TYPE_IS_OBJECT (type), NULL);

    klass = x_class_peek (type);
    if (!klass)
        klass = unref_class = x_class_ref (type);
    else
        unref_class = NULL;

    n_cparams = 0;
    n_oparams = 0;
    /* n_total_cparams 是需要的构造参数个数 */
    n_total_cparams = 0;
    for (slist = klass->construct_properties; slist; slist = slist->next)
    {
        clist = x_list_prepend (clist, slist->data);
        n_total_cparams += 1;
    }

    if (n_parameters == 0 && n_total_cparams == 0) {
        oparams = NULL;
        object = klass->constructor (type, 0, NULL);
        goto did_construction;
    }

    /* cparams是构造参数集,oparams是普通参数集 */
    cparams = x_new (xConstructParam, n_total_cparams);
    oparams = x_new (xConstructParam, n_parameters);
    /* 收集参数并排序 */
    for (i = 0; i < n_parameters; i++) {
        xValue  *value = &parameters[i].value;
        xParam  *pspec = param_pool_lookup (parameters[i].name,type, TRUE);
        if (!pspec) {
            x_warning ("%s: object class `%s' has no property named `%s'",
                       X_STRFUNC,
                       x_type_name (type),
                       parameters[i].name);
            continue;
        }
        if (!(pspec->flags & X_PARAM_WRITABLE)) {
            x_warning ("%s: property `%s' of object class `%s' is not writable",
                       X_STRFUNC,
                       x_quark_str (pspec->name),
                       x_type_name (type));
            continue;
        }
        if (pspec->flags & (X_PARAM_CONSTRUCT|X_PARAM_CONSTRUCT_ONLY)) {
            xList *list = x_list_find (clist, pspec);

            if (!list) {
                x_warning ("%s: construct property \"%s\" "
                           "for object `%s' can't be set twice",
                           X_STRFUNC,
                           x_quark_str (pspec->name),
                           x_type_name (type));
                continue;
            }
            cparams[n_cparams].pspec = pspec;
            cparams[n_cparams].value = value;
            n_cparams++;
            if (!list->prev)
                clist = list->next;
            else
                list->prev->next = list->next;
            if (list->next)
                list->next->prev = list->prev;
            x_list_free1 (list);
        }
        else {
            oparams[n_oparams].pspec = pspec;
            oparams[n_oparams].value = value;
            n_oparams++;
        }
    }

    /* 未提供的使用默认构造参数 */
    n_cvalues = n_total_cparams - n_cparams;
    cvalues = x_new (xValue, n_cvalues);
    while (clist) {
        xList   *tmp = clist->next;
        xParam  *pspec = clist->data;
        xValue *value = cvalues + n_total_cparams - n_cparams - 1;

        value->type = 0;
        x_value_init (value, pspec->value_type);
        x_param_value_init (pspec, value);

        cparams[n_cparams].pspec = pspec;
        cparams[n_cparams].value = value;
        n_cparams++;

        x_list_free1 (clist);
        clist = tmp;
    }

    object = klass->constructor (type, n_total_cparams, cparams);
    /* 释放资源 */
    x_free (cparams);
    while (n_cvalues--)
        x_value_clear (cvalues + n_cvalues);
    x_free (cvalues);

did_construction:
    if (CLASS_HAS_CUSTOM_CONSTRUCTOR (klass)) {
        /* adjust freeze_count according to object_init() and remaining properties */
        X_LOCK (construction_objects);
        newly_constructed = slist_maybe_remove (&construction_objects, object);
        X_UNLOCK (construction_objects);
    }
    else
        newly_constructed = TRUE;

    if (CLASS_HAS_PROPS (klass)) {
        if (newly_constructed || n_oparams)
            nqueue = notify_queue_freeze (object, FALSE);
        if (newly_constructed)
            notify_queue_thaw (object, nqueue);
    }

    /* run 'constructed' handler if there is a custom one */
    if (newly_constructed && CLASS_HAS_CUSTOM_CONSTRUCTED (klass))
        klass->constructed (object);

    /* set remaining properties */
    for (i = 0; i < n_oparams; i++)
        object_set_param_value (object, oparams[i].pspec, oparams[i].value, nqueue);
    x_free (oparams);

    if (CLASS_HAS_PROPS (klass)) {
        /* release our own freeze count and handle notifications */
        if (newly_constructed || n_oparams)
            notify_queue_thaw (object, nqueue);
    }

    if (unref_class)
        x_class_unref (unref_class);

    return object;
}

xbool
x_object_floating       (xptr           instance)
{
    xObject *object = instance;

    x_return_val_if_fail (X_IS_OBJECT (instance), FALSE);

    return object_floating (object, 0);
}

xptr
x_object_sink           (xptr           instance)
{
    xObject *object = instance;
    xbool was_floating;

    x_return_val_if_fail (X_IS_OBJECT (instance), NULL);
    x_return_val_if_fail (x_atomic_int_get (&object->ref_count) > 0, NULL);

    x_object_ref (object);

    was_floating = object_floating (object, -1);
    if (was_floating)
        x_object_unref (object);
    return object;
}

void
x_object_unsink         (xptr           instance)
{
    xObject *object = instance;

    if(!instance) return;
    x_return_if_fail (X_IS_OBJECT (instance));
    x_return_if_fail (x_atomic_int_get (&object->ref_count) > 0);

    object_floating (object, +1);
}

xptr
x_object_ref            (xptr           instance)
{
    xObject *object = (xObject*) instance;

    if(!instance) return NULL;
    x_return_val_if_fail (X_IS_OBJECT (instance), NULL);
    x_return_val_if_fail (x_atomic_int_get (&object->ref_count) > 0, NULL);

#ifdef  X_ENABLE_DEBUG
    if (x_trap_object_ref == object)
        X_BREAKPOINT ();
#endif  /* X_ENABLE_DEBUG */
    x_atomic_int_inc (&object->ref_count);
    return object;
}

xptr
x_object_weak_ref       (xptr           instance,
                         xCallback      notify,
                         xptr           user)
{
    WeakRefStack *wstack;
    xuint i;
    xObject *object = (xObject*) instance;

    if(!instance) return NULL;
    x_return_val_if_fail (X_IS_OBJECT (object), NULL);
    x_return_val_if_fail (notify != NULL, NULL);
    x_return_val_if_fail (object->ref_count >= 1, NULL);

    X_LOCK (weak_refs_mutex);
    wstack = x_data_list_id_remove_no_notify (&object->qdata, _quark_weak_refs);
    if (wstack)
    {
        i = wstack->n_weak_refs++;
        wstack = x_realloc (wstack, sizeof (*wstack) + sizeof (wstack->weak_refs[0]) * i);
    }
    else
    {
        wstack = x_new (WeakRefStack, 1);
        wstack->object = object;
        wstack->n_weak_refs = 1;
        i = 0;
    }
    wstack->weak_refs[i].notify = notify;
    wstack->weak_refs[i].data   = user;
    x_data_list_id_set_data_full (&object->qdata, _quark_weak_refs, wstack, weak_refs_notify);
    X_UNLOCK (weak_refs_mutex);

    return object;
}

void
x_object_weak_unref     (xptr           instance,
                         xCallback      notify,
                         xptr           user)
{
    WeakRefStack *wstack;
    xbool found_one = FALSE;
    xObject *object = (xObject*) instance;

    if(!instance) return;
    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (notify != NULL);

    X_LOCK (weak_refs_mutex);
    wstack = x_data_list_id_get_data (&object->qdata, _quark_weak_refs);
    if (wstack)
    {
        xuint i;

        for (i = 0; i < wstack->n_weak_refs; i++)
            if (wstack->weak_refs[i].notify == notify &&
                wstack->weak_refs[i].data == user)
            {
                found_one = TRUE;
                wstack->n_weak_refs -= 1;
                if (i != wstack->n_weak_refs)
                    wstack->weak_refs[i] = wstack->weak_refs[wstack->n_weak_refs];

                break;
            }
    }
    X_UNLOCK (weak_refs_mutex);
    if (!found_one)
        x_warning ("%s: couldn't find weak ref %p(%p)", X_STRFUNC, notify, user);
}
static void
nullify_pointer         (xptr     *weakptr_location)
{
    *weakptr_location = NULL;
}
void
x_object_add_weakptr    (xptr           instance, 
                         xptr           *weakptr_location)
{
    xObject *object = (xObject*) instance;
    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (weakptr_location != NULL);

    x_object_weak_ref (object, 
                       X_CALLBACK(nullify_pointer), 
                       weakptr_location);
}

void
x_object_remove_weakptr (xptr           instance, 
                         xptr           *weakptr_location)
{
    xObject *object = (xObject*) instance;
    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (weakptr_location != NULL);

    x_object_weak_unref (object, 
                         X_CALLBACK(nullify_pointer),
                         weakptr_location);
}

xint
x_object_unref          (xptr           instance)
{
    xObject *object = (xObject*) instance;
    xint    ref_count;
    xObjectClass    *klass;

    if(!instance) return -1;
    x_return_val_if_fail (X_IS_OBJECT (instance), -1);
    x_return_val_if_fail (x_atomic_int_get (&object->ref_count) > 0, -1);


    ref_count = x_atomic_int_dec (&object->ref_count);
    if X_LIKELY (ref_count !=0)
        return ref_count;

    klass = X_OBJECT_GET_CLASS (object);
    klass->dispose (object);
    klass->finalize(object);
    x_signal_clean (object);
    x_instance_delete (object);

    return ref_count;
}
void
x_oclass_install_params (xptr           klass,
                         xsize          n_pspecs,
                         xParam         **pspecs)
{
    xType oclass_type, parent_type;
    xsize i;
    xObjectClass    *oclass;

    x_return_if_fail (X_IS_OBJECT_CLASS (klass));
    x_return_if_fail (n_pspecs > 1);
    x_return_if_fail (pspecs[0] == NULL);

    oclass = (xObjectClass*)klass;
    if (CLASS_HAS_DERIVED_CLASS (oclass))
        x_error ("Attempt to add properties to %s after it was derived",
                 X_OBJECT_CLASS_NAME (oclass));

    oclass_type = X_OBJECT_CLASS_TYPE (oclass);
    parent_type = x_type_parent (oclass_type);

    /* we skip the first element of the array as it would have a 0 prop_id */
    for (i = 1; i < n_pspecs; i++) {
        xParam *pspec = pspecs[i];

        x_return_if_fail (pspec != NULL);

        if (pspec->flags & X_PARAM_WRITABLE)
            x_return_if_fail (oclass->set_property != NULL);

        if (pspec->flags & X_PARAM_READABLE)
            x_return_if_fail (oclass->get_property != NULL);

        x_return_if_fail (pspec->param_id == 0);	/* paranoid */
        if (pspec->flags & X_PARAM_CONSTRUCT)
            x_return_if_fail ((pspec->flags & X_PARAM_CONSTRUCT_ONLY) == 0);
        if (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))
            x_return_if_fail (pspec->flags & X_PARAM_WRITABLE);

        oclass->flags |= CLASS_HAS_PROPS_FLAG;
        install_property (oclass_type, i, pspec);

        if (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY)) {
            oclass->construct_properties = 
                x_slist_prepend (oclass->construct_properties, pspec);
        }

        /* for property overrides of construct properties, we have to get rid
         * of the overidden inherited construct property
         */
        pspec = param_pool_lookup_id (pspec->name, parent_type, TRUE);
        if (pspec && 
            (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))) {
            oclass->construct_properties = 
                x_slist_remove (oclass->construct_properties, pspec);
        }
    }
}
void
x_oclass_install_param  (xptr           klass,
                         xuint          param_id,
                         xParam         *pspec)
{
    xObjectClass    *oclass;
    x_return_if_fail (X_IS_OBJECT_CLASS (klass));
    x_return_if_fail (X_IS_PARAM (pspec));

    oclass = (xObjectClass*)klass;
    if (CLASS_HAS_DERIVED_CLASS (oclass))
        x_error ("Attempt to add property %s::%s to class after it was derived",
                 X_OBJECT_CLASS_NAME (oclass), x_quark_str (pspec->name));

    oclass->flags |= CLASS_HAS_PROPS_FLAG;

    x_return_if_fail (pspec->flags & X_PARAM_READWRITE);
    if (pspec->flags & X_PARAM_WRITABLE)
        x_return_if_fail (oclass->set_property != NULL);
    if (pspec->flags & X_PARAM_READABLE)
        x_return_if_fail (oclass->get_property != NULL);
    x_return_if_fail (param_id > 0);
    x_return_if_fail (pspec->param_id == 0);	/* paranoid */
    if (pspec->flags & X_PARAM_CONSTRUCT)
        x_return_if_fail ((pspec->flags & X_PARAM_CONSTRUCT_ONLY) == 0);
    if (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))
        x_return_if_fail (pspec->flags & X_PARAM_WRITABLE);

    install_property (X_OBJECT_CLASS_TYPE (oclass), param_id, pspec);

    if (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))
        oclass->construct_properties = 
            x_slist_prepend (oclass->construct_properties, pspec);

    /* for property overrides of construct properties, we have to get rid
     * of the overidden inherited construct property
     */
    pspec = param_pool_lookup_id (pspec->name,
                                  x_type_parent (X_OBJECT_CLASS_TYPE (oclass)),
                                  TRUE);
    if (pspec && pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))
        oclass->construct_properties = 
            x_slist_remove (oclass->construct_properties, pspec);
}
void
x_iface_install_param   (xptr           iface,
                         xParam         *pspec)
{
    xIFace *iface_class = iface;

    x_return_if_fail (X_TYPE_IS_IFACE (iface_class->type));
    x_return_if_fail (X_IS_PARAM (pspec));
    x_return_if_fail (!X_IS_PARAM_OVERRIDE (pspec)); /* paranoid */
    x_return_if_fail (pspec->param_id == 0);	/* paranoid */

    x_return_if_fail (pspec->flags & (X_PARAM_READABLE | X_PARAM_WRITABLE));
    if (pspec->flags & X_PARAM_CONSTRUCT)
        x_return_if_fail ((pspec->flags & X_PARAM_CONSTRUCT_ONLY) == 0);
    if (pspec->flags & (X_PARAM_CONSTRUCT | X_PARAM_CONSTRUCT_ONLY))
        x_return_if_fail (pspec->flags & X_PARAM_WRITABLE);

    install_property (iface_class->type, 0, pspec);
}

void
x_oclass_override_param (xptr           oclass,
                         xuint          param_id,
                         xcstr          name)
{
    xParam *overridden = NULL;
    xParam *param;
    xType parent_type;

    x_return_if_fail (X_IS_OBJECT_CLASS (oclass));
    x_return_if_fail (param_id > 0);
    x_return_if_fail (name != NULL);

    /* Find the overridden property; first check parent types
    */
    parent_type = x_type_parent (X_OBJECT_CLASS_TYPE (oclass));
    if (parent_type != X_TYPE_INVALID)
        overridden = param_pool_lookup (name, parent_type, TRUE);
    if (!overridden) {
        xType *ifaces;
        xsize n_ifaces;

        /* Now check interfaces */
        ifaces = x_type_ifaces (X_OBJECT_CLASS_TYPE (oclass), &n_ifaces);
        while (n_ifaces-- && !overridden) {
            overridden = param_pool_lookup (name, ifaces[n_ifaces], FALSE);
        }

        x_free (ifaces);
    }

    if (!overridden) {
        x_warning ("%s: Can't find property to override for '%s::%s'",
                   X_STRFUNC, X_OBJECT_CLASS_NAME (oclass), name);
        return;
    }

    param = x_param_override_new (name, overridden);
    x_oclass_install_param (oclass, param_id, param);
}

void
x_object_get            (xptr           object,
                         xcstr          first_property,
                         ...)
{
    va_list args;

    va_start (args, first_property);
    x_object_get_valist (object, first_property, args);
    va_end (args);
}

void
x_object_set            (xptr           object,
                         xcstr          first_property,
                         ...)
{
    va_list args;

    va_start (args, first_property);
    x_object_set_valist (object, first_property, args);
    va_end (args);
}

void
x_object_get_valist     (xptr           instance,
                         xcstr          first_property,
                         va_list        args)
{
    xcstr   name;
    xObject *object;

    x_return_if_fail (X_IS_OBJECT (instance));
    object = (xObject*) instance;
    x_object_ref (object);

    name = first_property;

    while (name) {
        xValue value = X_VALUE_INIT;
        xParam  *pspec;
        xstr    error;

        pspec = param_pool_lookup (name, X_INSTANCE_TYPE (object), TRUE);
        if (!pspec) {
            x_warning ("%s: object class `%s' has no property named `%s'",
                       X_STRFUNC,
                       X_OBJECT_TYPE_NAME (object),
                       name);
            break;
        }
        if (!(pspec->flags & X_PARAM_READABLE)) {
            x_warning ("%s: property `%s' of object class `%s' is not readable",
                       X_STRFUNC,
                       x_quark_str (pspec->name),
                       X_OBJECT_TYPE_NAME (object));
            break;
        }

        x_value_init (&value, pspec->value_type);

        object_get_param_value (object, pspec, &value);

        X_VALUE_LCOPY (&value, args, 0, error);
        if (error) {
            x_warning ("%s: %s", X_STRFUNC, error);
            x_free (error);
            x_value_clear (&value);
            break;
        }

        x_value_clear (&value);

        name = va_arg (args, xcstr);
    }

    x_object_unref (object);
}

void
x_object_set_valist     (xptr           instance,
                         xcstr          first_property,
                         va_list        args)
{
    NotifyQueue *nqueue;
    xcstr       name;
    xObject     *object;

    x_return_if_fail (X_IS_OBJECT (instance));
    object = (xObject*) instance;
    x_object_ref (object);

    nqueue = notify_queue_freeze (object, FALSE);
    name = first_property;
    while (name) {
        xValue value = X_VALUE_INIT;
        xParam  *pspec;
        xstr    error = NULL;

        pspec = param_pool_lookup (name, X_INSTANCE_TYPE (object), TRUE);
        if (!pspec) {
            x_warning ("%s: object class `%s' has no property named `%s'",
                       X_STRFUNC,
                       X_OBJECT_TYPE_NAME (object),
                       name);
            break;
        }
        if (!(pspec->flags & X_PARAM_WRITABLE)) {
            x_warning ("%s: property `%s' of object class `%s' is not writable",
                       X_STRFUNC,
                       x_quark_str (pspec->name),
                       X_OBJECT_TYPE_NAME (object));
            break;
        }
        if ((pspec->flags & X_PARAM_CONSTRUCT_ONLY)
            && !object_in_construction (object)) {
            x_warning ("%s: construct property \"%s\" "
                       "for object `%s' can't be set after construction",
                       X_STRFUNC,
                       x_quark_str (pspec->name),
                       X_OBJECT_TYPE_NAME (object));
            break;
        }

        X_VALUE_COLLECT_INIT (&value, pspec->value_type, args, 0, error);
        if (error) {
            x_warning ("%s: %s", X_STRFUNC, error);
            x_free (error);
            x_value_clear (&value);
            break;
        }

        object_set_param_value (object, pspec, &value, nqueue);
        x_value_clear (&value);

        name = va_arg (args, xcstr);
    }

    notify_queue_thaw (object, nqueue);
    x_object_unref (object);
}

void
x_object_get_value      (xptr           object,
                         xcstr          property_name,
                         xValue         *value)
{
    xParam *pspec;

    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (property_name != NULL);
    x_return_if_fail (X_IS_VALUE (value));

    x_object_ref (object);

    pspec = param_pool_lookup (property_name,
                               X_OBJECT_TYPE (object),
                               TRUE);
    if (!pspec)
        x_warning ("%s: object class `%s' has no property named `%s'",
                   X_STRFUNC,
                   X_OBJECT_TYPE_NAME (object),
                   property_name);
    else if (!(pspec->flags & X_PARAM_READABLE))
        x_warning ("%s: property `%s' of object class `%s' is not readable",
                   X_STRFUNC,
                   x_quark_str (pspec->name),
                   X_OBJECT_TYPE_NAME (object));
    else {
        xValue *prop_value, tmp_value = X_VALUE_INIT;

        /* auto-conversion of the callers value type
        */
        if (X_VALUE_TYPE (value) == pspec->value_type) {
            x_value_reset (value);
            prop_value = value;
        }
        else if (!x_value_type_transformable (pspec->value_type, X_VALUE_TYPE (value))) {
            x_warning ("%s: can't retrieve property `%s' of type `%s' as value of type `%s'",
                       X_STRFUNC,
                       x_quark_str (pspec->name),
                       x_type_name (pspec->value_type),
                       X_VALUE_TYPE_NAME (value));
            x_object_unref (object);
            return;
        }
        else {
            x_value_init (&tmp_value, pspec->value_type);
            prop_value = &tmp_value;
        }
        object_get_param_value (object, pspec, prop_value);
        if (prop_value != value) {
            x_value_transform (prop_value, value);
            x_value_clear (&tmp_value);
        }
    }

    x_object_unref (object);

}

void
x_object_set_value      (xptr           object,
                         xcstr          property_name,
                         const xValue   *value)
{
    NotifyQueue *nqueue;
    xParam *pspec;

    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (property_name != NULL);
    x_return_if_fail (X_IS_VALUE (value));

    x_object_ref (object);
    nqueue = notify_queue_freeze (object, FALSE);

    pspec = param_pool_lookup (property_name,
                               X_OBJECT_TYPE (object),
                               TRUE);
    if (!pspec)
        x_warning ("%s: object class `%s' has no property named `%s'",
                   X_STRFUNC,
                   X_OBJECT_TYPE_NAME (object),
                   property_name);
    else if (!(pspec->flags & X_PARAM_WRITABLE))
        x_warning ("%s: property `%s' of object class `%s' is not writable",
                   X_STRFUNC,
                   x_quark_str (pspec->name),
                   X_OBJECT_TYPE_NAME (object));
    else if ((pspec->flags & X_PARAM_CONSTRUCT_ONLY)
             && !object_in_construction (object))
        x_warning ("%s: construct property \"%s\" for object "
                   "`%s' can't be set after construction",
                   X_STRFUNC,
                   x_quark_str (pspec->name),
                   X_OBJECT_TYPE_NAME (object));
    else
        object_set_param_value (object, pspec, value, nqueue);

    notify_queue_thaw (object, nqueue);
    x_object_unref (object);
}
void
x_object_set_str        (xptr           object,
                         xcstr          property_name,
                         xcstr          value)
{
    xValue      xval = X_VALUE_INIT;

    x_value_init (&xval, X_TYPE_STR);
    x_value_set_static_str (&xval, value);
    x_object_set_value (object, property_name, &xval);
}

void
x_object_get_str        (xptr           object,
                         xcstr          property_name,
                         xstr           *value)
{
    xValue      xval;

    x_object_get_value (object, property_name, &xval);
    *value = x_value_dup_str (&xval);
    x_value_clear (&xval);
}
xptr
x_object_get_qdata      (xptr           object,
                         xQuark         key)
{
    x_return_val_if_fail (X_IS_OBJECT (object), NULL);

    return key ? 
        x_data_list_id_get_data (&((xObject*)object)->qdata, key) : NULL;
}

void
x_object_set_qdata_full (xptr           object,
                         xQuark         key,
                         xptr           val,
                         xFreeFunc      free_func)
{
    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (key > 0);

    x_data_list_id_set_data_full (&((xObject*)object)->qdata, key, val,
                                  val ? free_func : NULL);
}
void
x_object_notify         (xptr           object,
                         xcstr          property_name)
{
    xParam  *pspec;

    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (property_name != NULL);

    x_object_ref (object);
    pspec = param_pool_lookup (property_name, X_INSTANCE_TYPE (object), TRUE);

    if (!pspec)
        x_warning ("%s: object class `%s' has no property named `%s'",
                   X_STRFUNC,
                   X_OBJECT_TYPE_NAME (object),
                   property_name);
    else
        object_notify_internal (object, pspec);

    x_object_unref (object);
}
void
x_object_notify_param   (xptr           object,
                         xParam         *pspec)
{
    x_return_if_fail (X_IS_OBJECT (object));
    x_return_if_fail (X_IS_PARAM (pspec));

    x_object_ref (object);
    object_notify_internal (object, pspec);
    x_object_unref (object);
}
