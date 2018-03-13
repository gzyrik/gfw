#include "config.h"
#include "xclosure-prv.h"
#include "xclass.h"
#include "xvalue.h"
#include "xparam.h"
#include "xsignal.h"
#include "xboxed.h"
#include "xobject.h"
#include <string.h>
enum {
    FNOTIFY,
    INOTIFY,
    PRE_NOTIFY,
    POST_NOTIFY
};
#define CLOSURE_MAX_REF_COUNT       ((1 << 15) - 1)
#define CLOSURE_MAX_N_GUARDS        ((1 << 1) - 1)
#define CLOSURE_MAX_N_FNOTIFIERS    ((1 << 2) - 1)
#define CLOSURE_MAX_N_INOTIFIERS    ((1 << 8) - 1)
#define CLOSURE_N_MFUNCS(cl)        (((cl)->n_guards << 1L))
/* same as G_CLOSURE_N_NOTIFIERS() (keep in sync) */
#define CLOSURE_N_NOTIFIERS(cl)     (CLOSURE_N_MFUNCS (cl) + \
                                     (cl)->n_fnotifiers + \
                                     (cl)->n_inotifiers)

typedef union {
    xClosure closure;
    volatile xint vint;
} ClosureInt;


#define CHANGE_FIELD(_closure, _field, _OP, _value,                     \
                     _must_set, _SET_OLD, _SET_NEW) X_STMT_START {      \
    ClosureInt *cunion = (ClosureInt*) _closure;                        \
    xint new_int, old_int, success;                                     \
    do {                                                                \
        ClosureInt tmp;                                                 \
        tmp.vint = old_int = cunion->vint;                              \
        _SET_OLD tmp.closure._field;                                    \
        tmp.closure._field _OP _value;                                  \
        _SET_NEW tmp.closure._field;                                    \
        new_int = tmp.vint;                                             \
        success = x_atomic_int_cas (&cunion->vint, old_int, new_int);   \
    }                                                                   \
    while (!success && _must_set);                                      \
} X_STMT_END

#define SWAP(_closure, _field, _value, _oldv)                           \
    CHANGE_FIELD (_closure, _field, =, _value, TRUE, *(_oldv) =, (void) )

#define SET(_closure, _field, _value)                                   \
    CHANGE_FIELD (_closure, _field, =, _value, TRUE,  (void),    (void) )

#define INC(_closure, _field)                                           \
    CHANGE_FIELD (_closure, _field, +=,     1, TRUE,  (void),    (void) )

#define INC_ASSIGN(_closure, _field, _newv)                             \
    CHANGE_FIELD (_closure, _field, +=,     1, TRUE,  (void),*(_newv) = )

#define DEC(_closure, _field)                                           \
    CHANGE_FIELD (_closure, _field, -=,     1, TRUE,  (void),    (void) )

#define DEC_ASSIGN(_closure, _field, _newv)                             \
    CHANGE_FIELD (_closure, _field, -=,     1, TRUE,  (void),*(_newv) = )

static void
class_meta_marshal      (xClosure       *closure,
                         xValue /*out*/ *return_value,
                         xsize          n_param_values,
                         const xValue   *param_values,
                         xptr           invocation_hint,
                         xptr           marshal_data)
{
    xClass      *klass;
    xptr        callback;
    xuint       offset;
    xType       type;

    type = (xType) closure->data;
    offset = X_PTR_TO_SIZE (marshal_data);
    klass = X_INSTANCE_GET_CLASS (x_value_peek_ptr (param_values + 0),
                                  type, xClass);
    callback = X_STRUCT_MEMBER (xptr, klass, offset);
    if (callback)
        closure->marshal (closure,
                          return_value,
                          n_param_values, param_values,
                          invocation_hint,
                          callback);
}
static void
iface_meta_marshal      (xClosure       *closure,
                         xValue /*out*/ *return_value,
                         xsize          n_param_values,
                         const xValue   *param_values,
                         xptr           invocation_hint,
                         xptr           marshal_data)
{
    xIFace      *iface;
    xptr        callback;
    xType       type;
    xuint       offset;

    type = (xType) closure->data;
    offset = X_PTR_TO_SIZE (marshal_data);
    iface = X_INSTANCE_GET_IFACE (x_value_peek_ptr (param_values + 0),
                                  type, xIFace);
    callback = X_STRUCT_MEMBER (xptr, iface, offset);
    if (callback)
        closure->marshal (closure,
                          return_value,
                          n_param_values, param_values,
                          invocation_hint,
                          callback);
}
static void
class_meta_marshalv     (xClosure       *closure,
                         xValue         *return_value,
                         xptr           instance,
                         va_list        args,
                         xptr           marshal_data,
                         xsize          n_params,
                         xType          *param_types)
{
    xRealClosure    *real_closure;
    xClass          *klass;
    xptr            callback;
    xuint           offset;
    xType           type;

    real_closure = X_REAL_CLOSURE (closure);
    type = (xType) closure->data;
    offset = X_PTR_TO_SIZE (marshal_data);
    klass = X_INSTANCE_GET_CLASS (instance, type, xClass);
    callback = X_STRUCT_MEMBER (xptr, klass, offset);
    if (callback)
        real_closure->va_marshal (closure,
                                  return_value,
                                  instance, args,
                                  callback,
                                  n_params,
                                  param_types);
}

static void
iface_meta_marshalv     (xClosure       *closure,
                         xValue         *return_value,
                         xptr           instance,
                         va_list        args,
                         xptr           marshal_data,
                         xsize          n_params,
                         xType          *param_types)
{
    xRealClosure    *real_closure;
    xIFace          *iface;
    xptr            callback;
    xuint           offset;
    xType           type;

    real_closure = X_REAL_CLOSURE (closure);
    type = (xType) closure->data;
    offset = X_PTR_TO_SIZE (marshal_data);
    iface = X_INSTANCE_GET_IFACE (instance, type, xIFace);
    callback = X_STRUCT_MEMBER (xptr, iface, offset);
    if (callback)
        real_closure->va_marshal (closure,
                                  return_value,
                                  instance, args,
                                  callback,
                                  n_params,
                                  param_types);
}
static void
closure_set_meta_va_marshal (xClosure       *closure,
                             xVaClosureMarshal va_meta_marshal)
{
    xRealClosure *real_closure;

    x_return_if_fail (closure != NULL);
    x_return_if_fail (va_meta_marshal != NULL);
    x_return_if_fail (closure->is_invalid == FALSE);
    x_return_if_fail (closure->in_marshal == FALSE);

    real_closure = X_REAL_CLOSURE (closure);

    x_return_if_fail (real_closure->meta_marshal != NULL);

    real_closure->va_meta_marshal = va_meta_marshal;
}
static inline xbool
closure_try_remove_inotify  (xClosure       *closure,
                             xptr       notify_data,
                             xNotify notify_func)
{
    xClosureNotifyData *ndata, *nlast;

    nlast = closure->notifiers + CLOSURE_N_NOTIFIERS (closure) - 1;
    for (ndata = nlast + 1 - closure->n_inotifiers; ndata <= nlast; ndata++)
        if (ndata->notify == notify_func && ndata->data == notify_data) {
            DEC (closure, n_inotifiers);
            if (ndata < nlast)
                *ndata = *nlast;

            return TRUE;
        }
    return FALSE;
}

static inline xbool
closure_try_remove_fnotify  (xClosure       *closure,
                             xptr       notify_data,
                             xNotify notify_func)
{
    xClosureNotifyData *ndata, *nlast;

    nlast = closure->notifiers + CLOSURE_N_NOTIFIERS (closure)
        - closure->n_inotifiers - 1;
    for (ndata = nlast + 1 - closure->n_fnotifiers; ndata <= nlast; ndata++)
        if (ndata->notify == notify_func && ndata->data == notify_data) {
            DEC (closure, n_fnotifiers);
            if (ndata < nlast)
                *ndata = *nlast;
            if (closure->n_inotifiers)
                closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
                                    closure->n_fnotifiers)] = 
                    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
                                        closure->n_fnotifiers +
                                        closure->n_inotifiers)];
            return TRUE;
        }
    return FALSE;
}
static inline void
closure_invoke_notifiers(xClosure       *closure,
                         xuint          notify_type)
{
    /* notifier layout:
     *     n_guards    n_guards     n_fnotif.  n_inotifiers
     * ->[[pre_guards][post_guards][fnotifiers][inotifiers]]
     *
     * CLOSURE_N_MFUNCS(cl)    = n_guards + n_guards;
     * CLOSURE_N_NOTIFIERS(cl) = CLOSURE_N_MFUNCS(cl) + n_fnotifiers + n_inotifiers
     *
     * constrains/catches:
     * - closure->notifiers may be reloacted during callback
     * - closure->n_fnotifiers and closure->n_inotifiers may change during callback
     * - i.e. callbacks can be removed/added during invocation
     * - must prepare for callback removal during FNOTIFY and INOTIFY (done via ->marshal= & ->data=)
     * - must distinguish (->marshal= & ->data=) for INOTIFY vs. FNOTIFY (via ->in_inotify)
     * + closure->n_guards is const during PRE_NOTIFY & POST_NOTIFY
     * + none of the callbacks can cause recursion
     * + closure->n_inotifiers is const 0 during FNOTIFY
     */
    xClosureNotifyData *ndata;
    xuint i, n, offs;
    switch (notify_type) {
    case FNOTIFY:
        while (closure->n_fnotifiers) {
            DEC_ASSIGN (closure, n_fnotifiers, &n);

            ndata = closure->notifiers + CLOSURE_N_MFUNCS (closure) + n;
            closure->marshal = (xClosureMarshal) ndata->notify;
            closure->data = ndata->data;
            ndata->notify (ndata->data, closure);
        }
        closure->marshal = NULL;
        closure->data = NULL;
        break;
    case INOTIFY:
        SET (closure, in_inotify, TRUE);
        while (closure->n_inotifiers) {
            DEC_ASSIGN (closure, n_inotifiers, &n);

            ndata = closure->notifiers + CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers + n;
            closure->marshal = (xClosureMarshal) ndata->notify;
            closure->data = ndata->data;
            ndata->notify (ndata->data, closure);
        }
        closure->marshal = NULL;
        closure->data = NULL;
        SET (closure, in_inotify, FALSE);
        break;
    case PRE_NOTIFY:
        i = closure->n_guards;
        offs = 0;
        while (i--) {
            ndata = closure->notifiers + offs + i;
            ndata->notify (ndata->data, closure);
        }
        break;
    case POST_NOTIFY:
        i = closure->n_guards;
        offs = i;
        while (i--) {
            ndata = closure->notifiers + offs + i;
            ndata->notify (ndata->data, closure);
        }
        break;
    }
}
xClosure*
x_cclosure_new          (xCallback      callback_func,
                         xptr           user_data,
                         xNotify        destroy_data)
{
    xClosure *closure;

    x_return_val_if_fail (callback_func != NULL, NULL);

    closure = x_closure_new_simple (sizeof (xCClosure), user_data);
    if (destroy_data)
        x_closure_on_finalize(closure, user_data, destroy_data);
    ((xCClosure*) closure)->callback = (xptr) callback_func;

    return closure;
}

xClosure*
x_cclosure_new_swap     (xCallback      callback_func,
                         xptr           user_data,
                         xNotify        destroy_data)
{
    xClosure *closure;
    x_return_val_if_fail (callback_func != NULL, NULL);

    closure = x_cclosure_new (callback_func, user_data, destroy_data);
    SET (closure, derivative_flag, TRUE);

    return closure;
}

xClosure*
x_type_cclosure_new     (xType          type,
                         xlong          struct_offset)
{
    xClosure *closure;

    x_return_val_if_fail (X_TYPE_IS_CLASSED (type) ||
                          X_TYPE_IS_IFACE (type), NULL);
    x_return_val_if_fail (struct_offset >= sizeof (xClass), NULL);

    closure = x_closure_new_simple (sizeof (xClosure), (xptr) type);
    if (X_TYPE_IS_IFACE (type)) {
        x_closure_set_meta_marshal (closure,
                                    X_SIZE_TO_PTR (struct_offset),
                                    iface_meta_marshal);
        closure_set_meta_va_marshal (closure,
                                     iface_meta_marshalv);
    }
    else {
        x_closure_set_meta_marshal (closure,
                                    X_SIZE_TO_PTR (struct_offset),
                                    class_meta_marshal);
        closure_set_meta_va_marshal (closure,
                                     class_meta_marshalv);
    }
    return closure;
}

xClosure*
x_closure_new_simple    (xsize          sizeof_closure,
                         xptr           data)
{
    xRealClosure    *real_closure;
    xClosure        *closure;

    x_return_val_if_fail (sizeof_closure >= sizeof (xClosure), NULL);
    sizeof_closure = sizeof_closure + sizeof (xRealClosure) - sizeof (xClosure);

    real_closure = x_malloc0 (sizeof_closure);
    closure = &real_closure->closure;

    SET (closure, ref_count, 1);
    SET (closure, floating, TRUE);
    closure->data = data;

    return closure;
}
void
x_closure_sink          (xClosure       *closure)
{
    x_return_if_fail (closure != NULL);
    x_return_if_fail (closure->ref_count > 0);

    if (closure->floating) {
        xbool   was_floating;
        SWAP (closure, floating, FALSE, &was_floating);
        /* unref floating flag only once */
        if (was_floating)
            x_closure_unref (closure);
    }
}
xClosure*
x_closure_ref           (xClosure       *closure)
{
    xint new_ref_count;
    x_return_val_if_fail (closure != NULL, NULL);
    x_return_val_if_fail (closure->ref_count > 0, NULL);
    x_return_val_if_fail (closure->ref_count < CLOSURE_MAX_REF_COUNT, NULL);

    INC_ASSIGN (closure, ref_count, &new_ref_count);
    x_return_val_if_fail (new_ref_count > 1, NULL);

    return closure;
}

xint
x_closure_unref         (xClosure       *closure)
{
    xint new_ref_count;

    x_return_val_if_fail (closure != NULL, -1);
    x_return_val_if_fail (closure->ref_count > 0, -1);

    if (closure->ref_count == 1)
        x_closure_invalidate (closure);

    DEC_ASSIGN (closure, ref_count, &new_ref_count);

    if (new_ref_count == 0) {
        closure_invoke_notifiers (closure, FNOTIFY);
        x_free (closure->notifiers);
        x_free (X_REAL_CLOSURE (closure));
    }
    return new_ref_count;
}
void
x_closure_invalidate    (xClosure       *closure)
{
    x_return_if_fail (closure != NULL);

    if (!closure->is_invalid) {
        xbool   was_invalid;
        x_closure_ref (closure);
        SWAP (closure, is_invalid, TRUE, &was_invalid);
        if (!was_invalid)
            closure_invoke_notifiers (closure, INOTIFY);
        x_closure_unref (closure);
    }
}
void
x_closure_set_marshal   (xClosure	    *closure,
                         xClosureMarshal marshal)
{
    x_return_if_fail (closure != NULL);
    x_return_if_fail (marshal != NULL);

    if (closure->marshal && closure->marshal != marshal)
        x_warning ("attempt to override closure->marshal (%p) "
                   "with new marshal (%p)",
                   closure->marshal, marshal);
    else
        closure->marshal = marshal;
}
void
x_closure_set_meta_marshal (xClosure        *closure,
                            xptr            marshal_data,
                            xClosureMarshal meta_marshal)
{
    xRealClosure *real_closure;

    x_return_if_fail (closure != NULL);
    x_return_if_fail (meta_marshal != NULL);
    x_return_if_fail (closure->is_invalid == FALSE);
    x_return_if_fail (closure->in_marshal == FALSE);

    real_closure = X_REAL_CLOSURE (closure);
    x_return_if_fail (real_closure->meta_marshal == NULL);

    real_closure->meta_marshal = meta_marshal;
    real_closure->meta_marshal_data = marshal_data;
}
void
x_closure_on_finalize   (xClosure       *closure,
                         xptr           user_data,
                         xNotify        destroy_data)
{
    xuint i;

    x_return_if_fail (closure != NULL);
    x_return_if_fail (destroy_data != NULL);
    x_return_if_fail (closure->n_fnotifiers < CLOSURE_MAX_N_FNOTIFIERS);

    closure->notifiers = x_renew (xClosureNotifyData,
                                  closure->notifiers,
                                  CLOSURE_N_NOTIFIERS (closure) + 1);
    if (closure->n_inotifiers)
        closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
                            closure->n_fnotifiers +
                            closure->n_inotifiers)] =
            closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
                                closure->n_fnotifiers + 0)];
    i = CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers;
    closure->notifiers[i].data = user_data;
    closure->notifiers[i].notify = destroy_data;
    INC (closure, n_fnotifiers);
}
void
x_closure_off_finalize  (xClosure       *closure,
                         xptr           user_data,
                         xNotify        destroy_data)
{
    x_return_if_fail (closure != NULL);
    x_return_if_fail (destroy_data != NULL);

    if (closure->is_invalid && !closure->in_inotify &&
        ((xptr) closure->marshal) == ((xptr) destroy_data) &&
        closure->data == user_data)
        closure->marshal = NULL;
    else if (!closure_try_remove_fnotify (closure, user_data, destroy_data))
        x_warning ("%s: unable to remove uninstalled "
                   "finalization notifier: %p (%p)",
                   X_STRLOC, 
                   destroy_data, user_data);
}
void
x_closure_on_invalidate (xClosure       *closure,
                         xptr           user_data,
                         xNotify        destroy_data)
{
    xuint i;

    x_return_if_fail (closure != NULL);
    x_return_if_fail (destroy_data != NULL);
    x_return_if_fail (closure->is_invalid == FALSE);
    x_return_if_fail (closure->n_inotifiers < CLOSURE_MAX_N_INOTIFIERS);

    closure->notifiers = x_renew (xClosureNotifyData,
                                  closure->notifiers,
                                  CLOSURE_N_NOTIFIERS (closure) + 1);
    i = CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers
        + closure->n_inotifiers;
    closure->notifiers[i].data = user_data;
    closure->notifiers[i].notify = destroy_data;
    INC (closure, n_inotifiers);
}

void
x_closure_off_invalidate(xClosure       *closure,
                         xptr           user_data,
                         xNotify        destroy_data)
{
    x_return_if_fail (closure != NULL);
    x_return_if_fail (destroy_data != NULL);

    if (closure->is_invalid && closure->in_inotify &&
        ((xptr) closure->marshal) == ((xptr) destroy_data) &&
        closure->data == user_data)
        closure->marshal = NULL;
    else if (!closure_try_remove_inotify (closure, user_data, destroy_data))
        x_warning ("%s: unable to remove uninstalled "
                   "invalidation notifier: %p (%p)",
                   X_STRLOC,
                   destroy_data, user_data);
}
void
x_closure_add_guards    (xClosure       *closure,
                         xptr           pre_guard_data,
                         xNotify        pre_guard,
                         xptr           post_guard_data,
                         xNotify        post_guard)
{
}

void
x_closure_invoke        (xClosure       *closure,
                         xValue         *return_value,
                         xsize          n_param_values,
                         const xValue   *param_values,
                         xptr           invocation_hint)
{
    xRealClosure *real_closure;

    x_return_if_fail (closure != NULL);

    real_closure = X_REAL_CLOSURE (closure);

    x_closure_ref (closure);      /* preserve floating flag */
    if (!closure->is_invalid) {
        xClosureMarshal marshal;
        xptr    marshal_data;
        xbool   in_marshal = closure->in_marshal;

        x_return_if_fail (closure->marshal || real_closure->meta_marshal);

        SET (closure, in_marshal, TRUE);
        if (real_closure->meta_marshal) {
            marshal_data = real_closure->meta_marshal_data;
            marshal = real_closure->meta_marshal;
        }
        else {
            marshal_data = NULL;
            marshal = closure->marshal;
        }
        if (!in_marshal)
            closure_invoke_notifiers (closure, PRE_NOTIFY);
        marshal (closure,
                 return_value,
                 n_param_values, param_values,
                 invocation_hint,
                 marshal_data);
        if (!in_marshal)
            closure_invoke_notifiers (closure, POST_NOTIFY);
        SET (closure, in_marshal, in_marshal);
    }
    x_closure_unref (closure);
}
typedef union {
    xptr _xptr;
    xfloat _float;
    xdouble _double;
    xint _gint;
    xuint _guint;
    xlong _glong;
    xulong _gulong;
    xint64 _gint64;
    xuint64 _guint64;
} va_arg_storage;
#include "libffi/ffi.h"

static ffi_type *
va_to_ffi_type          (xType          gtype,
                         va_list        *va,
                         va_arg_storage *storage)
{
    ffi_type *rettype = NULL;
    xType type = x_type_fundamental (gtype);
    x_assert (type != X_TYPE_INVALID);

    switch (type) {
    case X_TYPE_BOOL:
    case X_TYPE_CHAR:
    case X_TYPE_INT:
    case X_TYPE_ENUM:
        rettype = &ffi_type_sint;
        storage->_gint = va_arg (*va, xint);
        break;
    case X_TYPE_UCHAR:
    case X_TYPE_UINT:
    case X_TYPE_FLAGS:
        rettype = &ffi_type_uint;
        storage->_guint = va_arg (*va, xuint);
        break;
    case X_TYPE_STR:
    case X_TYPE_OBJECT:
    case X_TYPE_BOXED:
    case X_TYPE_PARAM:
    case X_TYPE_PTR:
    case X_TYPE_IFACE:
        //    case X_TYPE_VARIANT:
        rettype = &ffi_type_pointer;
        storage->_xptr = va_arg (*va, xptr);
        break;
    case X_TYPE_FLOAT:
        /* Float args are passed as doubles in varargs */
        rettype = &ffi_type_float;
        storage->_float = (float)va_arg (*va, double);
        break;
    case X_TYPE_DOUBLE:
        rettype = &ffi_type_double;
        storage->_double = va_arg (*va, double);
        break;
    case X_TYPE_LONG:
        rettype = &ffi_type_slong;
        storage->_glong = va_arg (*va, xlong);
        break;
    case X_TYPE_ULONG:
        rettype = &ffi_type_ulong;
        storage->_gulong = va_arg (*va, xulong);
        break;
    case X_TYPE_INT64:
        rettype = &ffi_type_sint64;
        storage->_gint64 = va_arg (*va, xint64);
        break;
    case X_TYPE_UINT64:
        rettype = &ffi_type_uint64;
        storage->_guint64 = va_arg (*va, xuint64);
        break;
    default:
        rettype = &ffi_type_pointer;
        storage->_guint64  = 0;
        x_warning ("va_to_ffi_type: Unsupported fundamental type: %s",
                   x_type_name (type));
        break;
    }
    return rettype;
}
static ffi_type *
value_to_ffi_type       (const xValue   *xvalue,
                         xptr           *value,
                         xint           *enum_tmpval,
                         xbool          *tmpval_used)
{
    ffi_type *rettype = NULL;
    xType type = x_type_fundamental (X_VALUE_TYPE (xvalue));
    x_assert (type != X_TYPE_INVALID);

    if (enum_tmpval) {
        x_assert (tmpval_used != NULL);
        *tmpval_used = FALSE;
    }

    switch (type) {
    case X_TYPE_BOOL:
    case X_TYPE_CHAR:
    case X_TYPE_INT:
        rettype = &ffi_type_sint;
        *value = (xptr)&(xvalue->data[0].v_int);
        break;
    case X_TYPE_ENUM:
        /* enums are stored in v_long even though they are integers, which makes
         * marshalling through libffi somewhat complicated.  They need to be
         * marshalled as signed ints, but we need to use a temporary int sized
         * value to pass to libffi otherwise it'll pull the wrong value on
         * BE machines with 32-bit integers when treating v_long as 32-bit int.
         */
        x_assert (enum_tmpval != NULL);
        rettype = &ffi_type_sint;
        *enum_tmpval = x_value_get_enum (xvalue);
        *value = enum_tmpval;
        *tmpval_used = TRUE;
        break;
    case X_TYPE_UCHAR:
    case X_TYPE_UINT:
    case X_TYPE_FLAGS:
        rettype = &ffi_type_uint;
        *value = (xptr)&(xvalue->data[0].v_uint);
        break;
    case X_TYPE_STR:
    case X_TYPE_OBJECT:
    case X_TYPE_BOXED:
    case X_TYPE_PARAM:
    case X_TYPE_PTR:
    case X_TYPE_IFACE:
        //case X_TYPE_VARIANT:
        rettype = &ffi_type_pointer;
        *value = (xptr)&(xvalue->data[0].v_pointer);
        break;
    case X_TYPE_FLOAT:
        rettype = &ffi_type_float;
        *value = (xptr)&(xvalue->data[0].v_float);
        break;
    case X_TYPE_DOUBLE:
        rettype = &ffi_type_double;
        *value = (xptr)&(xvalue->data[0].v_double);
        break;
    case X_TYPE_LONG:
        rettype = &ffi_type_slong;
        *value = (xptr)&(xvalue->data[0].v_long);
        break;
    case X_TYPE_ULONG:
        rettype = &ffi_type_ulong;
        *value = (xptr)&(xvalue->data[0].v_ulong);
        break;
    case X_TYPE_INT64:
        rettype = &ffi_type_sint64;
        *value = (xptr)&(xvalue->data[0].v_int64);
        break;
    case X_TYPE_UINT64:
        rettype = &ffi_type_uint64;
        *value = (xptr)&(xvalue->data[0].v_uint64);
        break;
    default:
        rettype = &ffi_type_pointer;
        *value = NULL;
        x_warning ("value_to_ffi_type: Unsupported fundamental type: %s",
                   x_type_name (type));
        break;
    }
    return rettype;
}
static void
value_from_ffi_type     (xValue         *xvalue,
                         xptr           *value)
{
    ffi_arg *int_val = (ffi_arg*) value;

    switch (x_type_fundamental (X_VALUE_TYPE (xvalue))) {
    case X_TYPE_INT:
        x_value_set_int (xvalue, (xint) *int_val);
        break;
    case X_TYPE_FLOAT:
        x_value_set_float (xvalue, *(xfloat*)value);
        break;
    case X_TYPE_DOUBLE:
        x_value_set_double (xvalue, *(xdouble*)value);
        break;
    case X_TYPE_BOOL:
        x_value_set_bool (xvalue, (xbool) *int_val);
        break;
    case X_TYPE_STR:
        x_value_take_str (xvalue, *(xstr*)value);
        break;
    case X_TYPE_CHAR:
        x_value_set_char (xvalue, (xchar) *int_val);
        break;
    case X_TYPE_UCHAR:
        x_value_set_uchar (xvalue, (xuchar) *int_val);
        break;
    case X_TYPE_UINT:
        x_value_set_uint (xvalue, (xuint) *int_val);
        break;
    case X_TYPE_PTR:
        x_value_set_ptr (xvalue, *(xptr*)value);
        break;
    case X_TYPE_LONG:
        x_value_set_long (xvalue, (xlong) *int_val);
        break;
    case X_TYPE_ULONG:
        x_value_set_ulong (xvalue, (xulong) *int_val);
        break;
    case X_TYPE_INT64:
        x_value_set_int64 (xvalue, (xint64) *int_val);
        break;
    case X_TYPE_UINT64:
        x_value_set_uint64 (xvalue, (xuint64) *int_val);
        break;
    case X_TYPE_BOXED:
        x_value_take_boxed (xvalue, *(xptr*)value);
        break;
    case X_TYPE_ENUM:
        x_value_set_enum (xvalue, (xint) *int_val);
        break;
    case X_TYPE_FLAGS:
        x_value_set_flags (xvalue, (xuint) *int_val);
        break;
    case X_TYPE_PARAM:
        x_value_take_param (xvalue, *(xptr*)value);
        break;
    case X_TYPE_OBJECT:
        x_value_take_object (xvalue, *(xptr*)value);
        break;
        //case X_TYPE_VARIANT:
        //    x_value_take_variant (xvalue, *(xptr*)value);
        //    break;
    default:
        x_warning ("value_from_ffi_type: Unsupported fundamental type: %s",
                   x_type_name (x_type_fundamental (X_VALUE_TYPE (xvalue))));
    }
}
void
_x_closure_set_va_marshal (xClosure       *closure,
                           xVaClosureMarshal marshal)
{
    xRealClosure *real_closure;

    x_return_if_fail (closure != NULL);
    x_return_if_fail (marshal != NULL);

    real_closure = X_REAL_CLOSURE (closure);

    if (real_closure->va_marshal && real_closure->va_marshal != marshal)
        x_warning ("attempt to override closure->va_marshal (%p) "
                   "with new marshal (%p)",
                   real_closure->va_marshal, marshal);
    else
        real_closure->va_marshal = marshal;
}
xbool
_x_closure_supports_invoke_va (xClosure       *closure)
{
    xRealClosure *real_closure;

    x_return_val_if_fail (closure != NULL, FALSE);

    real_closure = X_REAL_CLOSURE (closure);

    return real_closure->va_marshal != NULL &&
        (real_closure->meta_marshal == NULL ||
         real_closure->va_meta_marshal != NULL);
}
xbool
_x_closure_is_void      (xClosure       *closure,
                         xptr           instance)
{
    xRealClosure *real_closure;
    xptr callback;
    xType itype;
    xuint offset;

    if (closure->is_invalid)
        return TRUE;

    real_closure = X_REAL_CLOSURE (closure);

    if (real_closure->meta_marshal == iface_meta_marshal)
    {
        xIFace *iface;
        itype = (xType) closure->data;
        offset = X_PTR_TO_SIZE (real_closure->meta_marshal_data);

        iface = X_INSTANCE_GET_IFACE (instance, itype, xIFace);
        callback = X_STRUCT_MEMBER (xptr, iface, offset);
        return callback == NULL;
    }
    else if (real_closure->meta_marshal == class_meta_marshal)
    {
        xClass *klass;
        itype = (xType) closure->data;
        offset = X_PTR_TO_SIZE (real_closure->meta_marshal_data);

        klass = X_INSTANCE_GET_CLASS (instance, itype, xClass);
        callback = X_STRUCT_MEMBER (xptr, klass, offset);
        return callback == NULL;
    }

    return FALSE;
}

void
_x_cclosure_marshal     (xClosure       *closure,
                         xValue         *return_xvalue,
                         xsize          n_param_values,
                         const xValue   *param_values,
                         xptr           invocation_hint,
                         xptr           marshal_data)
{
    ffi_type *rtype;
    void *rvalue;
    int n_args;
    ffi_type **atypes;
    void **args;
    int i;
    ffi_cif cif;
    xCClosure *cc = (xCClosure*) closure;
    xint *enum_tmpval;
    xbool tmpval_used = FALSE;

    enum_tmpval = x_alloca (sizeof (xint));
    if (return_xvalue && X_VALUE_TYPE (return_xvalue)) {
        rtype = value_to_ffi_type (return_xvalue, &rvalue, 
                                   enum_tmpval, &tmpval_used);
    }
    else {
        rtype = &ffi_type_void;
    }

    rvalue = x_alloca (MAX (rtype->size, sizeof (ffi_arg)));

    n_args = n_param_values + 1;
    atypes = x_alloca (sizeof (ffi_type *) * n_args);
    args =  x_alloca (sizeof (xptr) * n_args);

    if (tmpval_used)
        enum_tmpval = x_alloca (sizeof (xint));

    if (X_CLOSURE_SWAP_DATA (closure))
    {
        atypes[n_args-1] = value_to_ffi_type (param_values + 0,
                                              &args[n_args-1],
                                              enum_tmpval,
                                              &tmpval_used);
        atypes[0] = &ffi_type_pointer;
        args[0] = &closure->data;
    }
    else
    {
        atypes[0] = value_to_ffi_type (param_values + 0,
                                       &args[0],
                                       enum_tmpval,
                                       &tmpval_used);
        atypes[n_args-1] = &ffi_type_pointer;
        args[n_args-1] = &closure->data;
    }

    for (i = 1; i < n_args - 1; i++)
    {
        if (tmpval_used)
            enum_tmpval = x_alloca (sizeof (xint));

        atypes[i] = value_to_ffi_type (param_values + i,
                                       &args[i],
                                       enum_tmpval,
                                       &tmpval_used);
    }

    if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, n_args, rtype, atypes) != FFI_OK)
        return;

    ffi_call (&cif, marshal_data ? marshal_data : (xptr)cc->callback, rvalue, args);

    if (return_xvalue && X_VALUE_TYPE (return_xvalue))
        value_from_ffi_type (return_xvalue, rvalue);
}

void
_x_closure_va_marshal   (xClosure       *closure,
                         xValue         *return_value,
                         xptr           instance,
                         va_list        args_list,
                         xptr           marshal_data,
                         xsize          n_params,
                         xType          *param_types)
{
    ffi_type *rtype;
    void *rvalue;
    int n_args;
    ffi_type **atypes;
    void **args;
    va_arg_storage *storage;
    xsize i;
    ffi_cif cif;
    xCClosure *cc = (xCClosure*) closure;
    xint *enum_tmpval;
    xbool tmpval_used = FALSE;
    va_list args_copy;

    enum_tmpval = x_alloca (sizeof (xint));
    if (return_value && X_VALUE_TYPE (return_value)) {
        rtype = value_to_ffi_type (return_value, &rvalue,
                                   enum_tmpval, &tmpval_used);
    }
    else {
        rtype = &ffi_type_void;
    }

    rvalue = x_alloca (MAX (rtype->size, sizeof (ffi_arg)));

    n_args = n_params + 2;
    atypes = x_alloca (sizeof (ffi_type *) * n_args);
    args =  x_alloca (sizeof (xptr) * n_args);
    storage = x_alloca (sizeof (va_arg_storage) * n_params);

    if (tmpval_used)
        enum_tmpval = x_alloca (sizeof (xint));

    if (X_CLOSURE_SWAP_DATA (closure)) {
        atypes[n_args-1] = &ffi_type_pointer;
        args[n_args-1] = &instance;
        atypes[0] = &ffi_type_pointer;
        args[0] = &closure->data;
    }
    else {
        atypes[0] = &ffi_type_pointer;
        args[0] = &instance;
        atypes[n_args-1] = &ffi_type_pointer;
        args[n_args-1] = &closure->data;
    }
    va_copy (args_copy, args_list);

    /* Box non-primitive arguments */
    for (i = 0; i < n_params; i++) {
        xType type = param_types[i]  & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        xType fundamental = X_TYPE_FUNDAMENTAL (type);

        atypes[i+1] = va_to_ffi_type (type,
                                      &args_copy,
                                      &storage[i]);
        args[i+1] = &storage[i];

        if ((param_types[i]  & X_SIGNAL_TYPE_STATIC_SCOPE) == 0) {
            if (fundamental == X_TYPE_STR && storage[i]._xptr != NULL)
                storage[i]._xptr = x_strdup (storage[i]._xptr);
            else if (fundamental == X_TYPE_PARAM && storage[i]._xptr != NULL)
                storage[i]._xptr = x_param_ref (storage[i]._xptr);
            else if (fundamental == X_TYPE_BOXED && storage[i]._xptr != NULL)
                storage[i]._xptr = x_boxed_copy (type, storage[i]._xptr);
            //else if (fundamental == X_TYPE_VARIANT && storage[i]._xptr != NULL)
            //    storage[i]._xptr = x_variant_ref_sink (storage[i]._xptr);
        }
        if (fundamental == X_TYPE_OBJECT && storage[i]._xptr != NULL)
            storage[i]._xptr = x_object_ref (storage[i]._xptr);
    }

    va_end (args_copy);

    if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, n_args, rtype, atypes) != FFI_OK)
        return;

    ffi_call (&cif, marshal_data ? marshal_data : (xptr)cc->callback, rvalue, args);

    /* Unbox non-primitive arguments */
    for (i = 0; i < n_params; i++) {
        xType type = param_types[i]  & ~X_SIGNAL_TYPE_STATIC_SCOPE;
        xType fundamental = X_TYPE_FUNDAMENTAL (type);

        if ((param_types[i]  & X_SIGNAL_TYPE_STATIC_SCOPE) == 0) {
            if (fundamental == X_TYPE_STR && storage[i]._xptr != NULL)
                x_free (storage[i]._xptr);
            else if (fundamental == X_TYPE_PARAM && storage[i]._xptr != NULL)
                x_param_unref (storage[i]._xptr);
            else if (fundamental == X_TYPE_BOXED && storage[i]._xptr != NULL)
                x_boxed_free (type, storage[i]._xptr);
            //else if (fundamental == X_TYPE_VARIANT && storage[i]._xptr != NULL)
            //    x_variant_unref (storage[i]._xptr);
        }
        if (fundamental == X_TYPE_OBJECT && storage[i]._xptr != NULL)
            x_object_unref (storage[i]._xptr);
    }

    if (return_value && X_VALUE_TYPE (return_value))
        value_from_ffi_type (return_value, rvalue);
}
void
_x_closure_invoke_va (xClosure       *closure,
                      xValue /*out*/ *return_value,
                      xptr           instance,
                      va_list        args,
                      xsize          n_params,
                      xType          *param_types)
{
    xRealClosure *real_closure;

    x_return_if_fail (closure != NULL);

    real_closure = X_REAL_CLOSURE (closure);

    x_closure_ref (closure);      /* preserve floating flag */
    if (!closure->is_invalid) {
        xVaClosureMarshal marshal;
        xptr marshal_data;
        xbool in_marshal = closure->in_marshal;

        x_return_if_fail (closure->marshal || real_closure->meta_marshal);

        SET (closure, in_marshal, TRUE);
        if (real_closure->va_meta_marshal) {
            marshal_data = real_closure->meta_marshal_data;
            marshal = real_closure->va_meta_marshal;
        }
        else {
            marshal_data = NULL;
            marshal = real_closure->va_marshal;
        }
        if (!in_marshal)
            closure_invoke_notifiers (closure, PRE_NOTIFY);
        marshal (closure,
                 return_value,
                 instance, args,
                 marshal_data,
                 n_params, param_types);
        if (!in_marshal)
            closure_invoke_notifiers (closure, POST_NOTIFY);
        SET (closure, in_marshal, in_marshal);
    }
    x_closure_unref (closure);
}
