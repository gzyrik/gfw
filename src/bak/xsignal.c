#include "config.h"
#include "xobject.h"
#include <string.h>
X_LOCK_DEFINE_STATIC    (signal);
static xuint            n_signal_nodes = 0;
static SignalNode       signal_nodes = NULL;
static inline SignalNode*
lookup_signal_node_L    (xuint          signal_id)
{
    if (signal_id < n_signal_nodes)
        return signal_nodes[signal_id];
    else
        return NULL;
}

xuint
x_signal_new            (xcstr          signal_name,
                         xType          itype,
                         xSignalFlags   signal_flags,
                         xuint          class_offset,
                         xSignalAccumulator	 accumulator,
                         xptr		    accu_data,
                         xSignalCMarshaller  c_marshaller,
                         xType          return_type,
                         xuint          n_params,
                         ...)
{
    va_list     args;
    xuint       signal_id;

    x_return_val_if_fail (signal_name != NULL, 0);

    va_start (args, n_params);

    signal_id = x_signal_new_valist (signal_name, itype, signal_flags,
                                     !class_offset ?  NULL:
                                     x_signal_type_cclosure_new (itype,
                                                                 class_offset),
                                     accumulator, accu_data, c_marshaller,
                                     return_type, n_params, args);

    va_end (args);

    return signal_id;
}
xuint
x_signal_new_valist     (xcstr          signal_name,
                         xType          itype,
                         xSignalFlags   signal_flags,
                         xClosure       *class_closure,
                         xSignalAccumulator accumulator,
                         xptr		    accu_data,
                         xSignalCMarshaller c_marshaller,
                         xType          return_type,
                         xuint          n_params,
                         va_list        args);
{
    xType   *param_types;
    xuint   i;
    xuint   signal_id;

    if (n_params > 0) {
        param_types = x_new (xType, n_params);

        for (i = 0; i < n_params; i++)
            param_types[i] = va_arg (args, xType);
    }
    else
        param_types = NULL;

    signal_id = x_signal_newv (signal_name, itype, signal_flags,
                               class_closure, accumulator, accu_data,
                               c_marshaller,
                               return_type, n_params, param_types);
    x_free (param_types);

    return signal_id;
}

xuint
x_signal_newv           (xcstr          signal_name,
                         xType          itype,
                         xSignalFlags   signal_flags,
                         xClosure       *class_closure,
                         xSignalAccumulator accumulator,
                         xptr		    accu_data,
                         xSignalCMarshaller c_marshaller,
                         xType		    return_type,
                         xuint          n_params,
                         xType		    *param_types)
{
}

#define CLOSURE_MAGIC 0x12345678
/* name -> xSignal */
X_LOCK_DEFINE_STATIC (signal_ht);
static xHashTbl     *signal_ht;
static xMarshal     default_marshal (xcstr format);

static xuint
signal_hash             (xcptr          key)
{
    const xSignal *signal = key;
    return signal->owner_type + X_PTR_TO_UINT(signal->name);
}

static xint
signal_cmp              (xcptr          key1,
                         xcptr          key2)
{
    const xSignal *signal1 = key1;
    const xSignal *signal2 = key2;
    if(signal1->owner_type != signal2->owner_type)
        return signal1->owner_type - signal2->owner_type;
    return signal1->name - signal2->name;
}

void
x_signal_insert         (xType          owner_type,
                         xSignal        *signals,
                         xuint          n_signals)
{
    xuint i;

    for (i = 0; i < n_signals && signals[i].name; ++i) {
        signals[i].owner_type = owner_type;
        signals[i].name       = x_istr_from_sstr (signals[i].name);
        if (!signals[i].marshal)
            signals[i].marshal = default_marshal (signals[i].format);
    }
    n_signals = i;

    X_LOCK (signal_ht);
    if (!signal_ht)
        signal_ht = x_hashtbl_new (signal_hash, signal_cmp);

    for (i = 0; i < n_signals; ++i)
        x_hashtbl_add (signal_ht,  signals+i);

    X_UNLOCK (signal_ht); 
}

void
x_signal_remove         (xSignal        *signals,
                         xuint          n_signals)
{
    xuint i;

    x_return_if_fail (signal_ht);

    X_LOCK (signal_ht);

    for (i = 0; i < n_signals && signals[i].name; ++i)
        x_hashtbl_remove (signal_ht,  signals+i);

    X_UNLOCK (signal_ht);
}

xSignal*
x_signal_lookup         (xcstr          detail_signal,
                         xType          owner_type,
                         xcstr          *detail,
                         xbool          walk_ancestors)
{
    xSignal signal, *ret;
    xcstr colon;

    x_return_val_if_fail (signal_ht, NULL);

    signal.owner_type = owner_type;
    colon = strchr (detail_signal, ':');
    if (!colon) {
        signal.name = x_istr_try_str (detail_signal);
    }
    else if (colon[1] == ':') {
        xstr name = x_strdup (detail_signal, colon - detail_signal);
        signal.name = x_istr_try_str (name);
        x_free (name);
    }

    x_return_val_if_fail (signal.name, NULL);

    X_LOCK (signal_ht);

    if (walk_ancestors) {
        do {
            ret = x_hashtbl_lookup (signal_ht,  &signal);
            signal.owner_type = x_type_parent (signal.owner_type);
        } while (!ret && signal.owner_type);
    }
    else {
        ret = x_hashtbl_lookup (signal_ht,  &signal);
    }

    X_UNLOCK (signal_ht); 

    if (ret && colon && colon[2] && detail)
        *detail = &colon[2];

    return ret;
}

typedef struct _xClosure   xClosure;
typedef struct _xClosureLst xClosureLst;
struct _xClosure
{
    xulong              magic;
    xCallback           callback;
    xptr                data;
    xFreeFunc           destroy_data;
    xQuark              detail;
    xint                swapped:1;
    xint                blocked:1;
    xint                deleted:1;
    xint                after:1;
    xClosure*           prev;
    xClosure*           next;
};

struct _xClosureLst
{
    xInstance   *instance;
    xSignal     *signal;
    xClosure    *closures;
    xClosure    *tail_before;
    xClosure    *tail_after;
    xMutex      mutex;
    xint        ref;
};

static xuint
closure_list_hash       (xcptr          key)
{
    const xClosureLst *lst = key;
    return X_PTR_TO_UINT(lst->instance) + X_PTR_TO_UINT(lst->signal);
}

static xint
closure_list_cmp        (xcptr          key1,
                         xcptr          key2)
{
    const xClosureLst *lst1 = key1;
    const xClosureLst *lst2 = key2;
    if(lst1->instance != lst2->instance)
        return lst1->instance - lst2->instance;
    return lst1->signal - lst2->signal;
}

static void
closure_list_delete     (xptr           key)
{
    xClosureLst *lst = key;
    x_slice_free_chain (xClosure, lst->closures, next);
    x_slice_free (xClosureLst, lst);
}

/* xInstance,xSignal -> xClosureLst */
X_LOCK_DEFINE_STATIC (closure_ht);
static xHashTbl      *closure_ht;

static  xClosureLst*
closure_list_ref        (xInstance      *instance,
                         xSignal        *signal,
                         xbool          ensure)
{
    xClosureLst lst, *ret=NULL;

    X_LOCK (closure_ht);
    if (!closure_ht) {
        if (ensure)
            closure_ht = x_hashtbl_new_full (closure_list_hash,
                                             closure_list_cmp,
                                             closure_list_delete,
                                             NULL);
        else 
            goto clean;
    }

    lst.instance = instance;
    lst.signal   = signal;
    ret = x_hashtbl_lookup (closure_ht, &lst);
    if (ret || !ensure) goto clean;

    ret = x_slice_new0 (xClosureLst);
    ret->instance = instance;
    ret->signal   = signal;
    ret->ref      = 1;
    x_hashtbl_add (closure_ht, ret);

clean:
    if (ret) x_atomic_int_inc (&ret->ref);
    X_UNLOCK (closure_ht);
    return ret;
}

static xint 
closure_list_unref      (xClosureLst    *lst)
{
    xint ref = x_atomic_int_dec (&lst->ref);
    if (!ref){
        X_LOCK (closure_ht);
        x_hashtbl_remove (closure_ht, lst);
        X_UNLOCK (closure_ht);
    }
    return ref;
}

/* tail_after 总是移到链末,因此after的总是添加到尾部。
 * tail_before总是移到当前,因此普通的总是往后添加
 * [closures, tail_before],为普通函数
 * (tail_before, tail_after],为after的函数
 */
static xClosure*
closure_insert          (xInstance      *instance,
                         xSignal        *signal,
                         xClosure       *closure)
{
    xClosureLst* clist;

    clist = closure_list_ref (instance, signal, TRUE);

    x_mutex_lock (&clist->mutex);
    if (!clist->closures) {
        clist->closures = closure;
        if (!closure->after)
            clist->tail_before = closure;
    }
    else if (closure->after) {
        /* 插到tail_after之后 */
        closure->prev = clist->tail_after;
        clist->tail_after->next = closure;
    }
    else {
        if (clist->tail_before) {
            /* 插到tail_before之后 */
            closure->next = clist->tail_before->next;
            if (closure->next)
                closure->next->prev = closure;
            closure->prev = clist->tail_before;
            clist->tail_before->next = closure;
        }
        else {
            /* 插到closures之前 */
            closure->next = clist->closures;
            if (closure->next)
                closure->next->prev = closure;
            clist->closures = closure;
        }
        clist->tail_before = closure;
    }
    if (!closure->next)
        clist->tail_after = closure;
    x_mutex_unlock (&clist->mutex);

    if(!closure_list_unref (clist))
        return NULL;
    return closure;
}

xClosure*
x_signal_connect_full       (xInstance      *instance,
                             xSignal        *signal,
                             xQuark         detail,
                             xCallback      callback,
                             xptr           data,
                             xFreeFunc      destroy_data,
                             xuint          signal_flags)
{
    xClosure *closure;
    x_return_val_if_fail (instance, NULL);
    x_return_val_if_fail (signal, NULL);
    x_return_val_if_fail (callback, NULL);

    closure               = x_slice_new0 (xClosure);
    closure->magic        = CLOSURE_MAGIC;
    closure->swapped      = (signal_flags&X_CON_SWAPPED);
    closure->callback     = callback;
    closure->data         = data;
    closure->detail       = detail;
    closure->destroy_data = destroy_data;

    return closure_insert (instance, signal, closure);
}

xClosure*
x_signal_connect_data   (xInstance      *instance,
                         xcstr          detail_signal,
                         xCallback      callback,
                         xptr           data,
                         xFreeFunc      destroy_data,
                         xuint          signal_flags)
{
    xSignal *signal;
    xcstr detail = NULL;

    x_return_val_if_fail (instance, NULL);
    x_return_val_if_fail (detail_signal, NULL);

    signal = x_signal_lookup (detail_signal,
                              X_INSTANCE_TYPE(instance),
                              &detail,
                              TRUE);
    x_return_val_if_fail (signal, NULL);

    return x_signal_connect_full (instance, signal,
                                  x_quark_from_str (detail),
                                  callback, data,
                                  destroy_data,signal_flags);
}

xint
x_signal_emit_full          (xInstance      *instance,
                             xSignal        *signal,
                             xQuark         detail,
                             ...)
{
    va_list argv;
    va_start(argv, detail);
    return x_signal_emit_valist(instance, signal, detail, argv);
}

xint
x_signal_emit_valist    (xInstance      *instance,
                         xSignal        *signal,
                         xQuark         detail,
                         va_list        argv)
{
    xClosure *closure, *next;
    xClosureLst *lst;
    xCallback klass_cb;
    xint ret = 0;

    x_return_val_if_fail (instance, -1);
    x_return_val_if_fail (signal, -1);

    lst = closure_list_ref (instance, signal, FALSE);
    if (!lst) return 0;

    klass_cb= X_STRUCT_MEMBER(xCallback,
                              X_INSTANCE_CLASS (instance), signal->offset);
    if (signal->flags & X_SIG_FIRST) {
        signal->marshal (instance, klass_cb, &ret, argv, NULL, FALSE);
        klass_cb = NULL;
    }

    x_mutex_lock (&lst->mutex);
    closure = lst->closures;
    while (closure && ret >= 0) {
        if (closure->deleted) {
            if (closure->prev)
                closure->prev->next = closure->next;
            else
                lst->closures = closure->next;
            if (closure->next) 
                closure->next->prev = closure->prev;
            if (closure == lst->tail_before)
                lst->tail_before = closure->prev;
            next = closure->next;
            x_slice_free (xClosure, closure);
            closure = next;
            continue;
        }
        else {
            if (klass_cb && closure->after && (signal->flags & X_SIG_LAST)) {
                signal->marshal (instance, klass_cb, &ret, argv, NULL, FALSE);
                klass_cb = NULL;
            }
            if (!closure->blocked
                && (!closure->detail || detail == closure->detail))
                signal->marshal(instance, closure->callback, &ret,
                                argv, closure->data, closure->swapped);
            closure = closure->next;
        }
    }
    if (klass_cb)
        signal->marshal (instance, klass_cb, &ret, argv, NULL, FALSE);
    x_mutex_unlock (&lst->mutex);
    closure_list_unref (lst);
    return 0;
}

xint
x_signal_emit           (xInstance      *instance,
                         xcstr          detail_signal,
                         ...)
{
    va_list argv;
    xSignal *signal;
    xcstr detail = NULL;

    x_return_val_if_fail (instance, -1);
    x_return_val_if_fail (detail_signal, -1);

    signal = x_signal_lookup (detail_signal,
                              X_INSTANCE_TYPE(instance),
                              &detail,
                              TRUE);
    x_return_val_if_fail (signal, -1);

    va_start(argv, detail_signal);
    return x_signal_emit_valist(instance, signal,
                                x_quark_from_str (detail), argv);
}

static xbool
instance_clean          (xptr           key,
                         xptr           value,
                         xptr           instance)
{
    xClosureLst* lst = key;
    return lst->instance == instance;
}

void
x_signal_clean          (xInstance      *instance)
{
    xClosureLst lst;


    x_return_if_fail (instance);
    x_return_if_fail (closure_ht);

    lst.instance = instance;

    X_LOCK (closure_ht);
    x_hashtbl_foreach_remove (closure_ht,
                              instance_clean, instance);
    X_UNLOCK (closure_ht);
}

xint
x_closure_block         (xClosure       *closure)
{
    x_return_val_if_fail (closure->magic == CLOSURE_MAGIC, -1);
    closure->blocked = 1;
    return 0;
}

xint
x_closure_unblock       (xClosure       *closure)
{
    x_return_val_if_fail (closure->magic == CLOSURE_MAGIC, -1);
    closure->blocked = 0;
    return 0;
}

xint
x_closure_delete        (xClosure       *closure)
{
    x_return_val_if_fail (closure->magic == CLOSURE_MAGIC, -1);
    closure->deleted = 1;
    return 0;
}

#pragma warning(disable:4087)
static void
x_marshal__VOID (xptr inst, xCallback fun, xint* ret,
                 va_list argv, xptr data, xbool swap)
{
    if (swap)
        fun(data, inst, ret);
    else
        fun(inst, data, ret);
}

static void
x_marshal__INT (xptr inst, xCallback fun, xint* ret,
                va_list argv, xptr data, xbool swap)
{
    xint arg1 = va_arg(argv, xint);
    if (swap)
        fun(data, arg1, inst, ret);
    else
        fun(inst, arg1, data, ret);
}

static void
x_marshal__PTR (xptr inst, xCallback fun, xint* ret,
                va_list argv, xptr data, xbool swap)
{
    xptr arg1 = va_arg(argv, xptr);
    if (swap)
        fun(data, arg1, inst, ret);
    else
        fun(inst, arg1, data, ret);
}

static xMarshal
default_marshal         (xcstr          format)
{
    if ( !format || format[0] == '\0')
        return x_marshal__VOID;
    else if (!strcmp(format, "i"))
        return x_marshal__INT;
    else if (!strcmp(format, "p"))
        return x_marshal__PTR;
    return NULL;
}
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
