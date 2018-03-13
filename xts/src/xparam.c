#include "config.h"
#include "xclass.h"
#include "xvalue.h"
#include "xparam.h"
#include "xclass-prv.h"
#include "xvalist.h"
#include "xobject.h"
#include "xboxed.h"
#include <string.h>
#define PARAM_FLOATING_FLAG                     0x2
#define HOLDS_VALUE_TYPE(pspec, value)	                                \
    (x_value_holds ((value), X_PARAM_VALUE_TYPE (pspec)))

static xType   *_param_types;
const xType*
x_param_types   (void)
{
    return _param_types;
}
xParam*
x_param_sink            (xParam         *pspec)
{
    xsize oldvalue;
    x_return_val_if_fail (X_IS_PARAM (pspec), NULL);

    oldvalue = x_atomic_ptr_and (&pspec->qdata, ~(xsize)PARAM_FLOATING_FLAG);
    if (!(oldvalue & PARAM_FLOATING_FLAG))
        x_param_ref (pspec);

    return pspec;
}

xParam*
x_param_ref             (xParam         *pspec)
{
    x_return_val_if_fail (X_IS_PARAM (pspec), NULL);
    x_return_val_if_fail (x_atomic_int_get (&pspec->ref_count) > 0, NULL);

    x_atomic_int_inc (&pspec->ref_count);

    return pspec;
}
xint
x_param_unref           (xParam         *pspec)
{
    xint ref_count;

    x_return_val_if_fail (X_IS_PARAM (pspec), -1);
    x_return_val_if_fail (x_atomic_int_get (&pspec->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&pspec->ref_count);

    if X_LIKELY (ref_count != 0)
        return ref_count;

    X_PARAM_GET_CLASS (pspec)->finalize (pspec);
    return ref_count;
}  
void
x_param_value_init      (const xParam   *pspec,
                         xValue         *value)
{
    x_return_if_fail (X_IS_PARAM (pspec));
    x_return_if_fail (X_IS_VALUE (value));
    x_return_if_fail (HOLDS_VALUE_TYPE (pspec, value));

    x_value_reset (value);
    X_PARAM_GET_CLASS (pspec)->value_init (pspec, value);
}  
xbool
x_param_value_default   (const xParam   *pspec,
                         const xValue   *value)
{
    xint    ret;
    xValue  dflt_value = X_VALUE_INIT;

    x_return_val_if_fail (X_IS_PARAM (pspec), FALSE);
    x_return_val_if_fail (X_IS_VALUE (value), FALSE);
    x_return_val_if_fail (HOLDS_VALUE_TYPE (pspec, value),FALSE);

    x_value_init (&dflt_value, X_PARAM_VALUE_TYPE (pspec));
    X_PARAM_GET_CLASS (pspec)->value_init (pspec, &dflt_value);
    ret = X_PARAM_GET_CLASS (pspec)->value_cmp (pspec, value, &dflt_value);
    x_value_clear (&dflt_value);

    return ret == 0;

}
xbool
x_param_value_validate  (const xParam   *pspec,
                         xValue         *value)
{
    x_return_val_if_fail (X_IS_PARAM (pspec), FALSE);
    x_return_val_if_fail (X_IS_VALUE (value), FALSE);
    x_return_val_if_fail (HOLDS_VALUE_TYPE (pspec, value), FALSE);

    if (X_PARAM_GET_CLASS (pspec)->value_validate) {
        xValue oval = *value;

        if (X_PARAM_GET_CLASS (pspec)->value_validate (pspec, value) ||
            memcmp (&oval.data, &value->data, sizeof (oval.data)))
            return TRUE;
    }

    return FALSE;

}
xbool
x_param_value_convert   (const xParam   *pspec,
                         const xValue   *src_value,
                         xValue         *dest_value,
                         xbool          strict_validation)
{
    xValue tmp_value = X_VALUE_INIT;

    x_return_val_if_fail (X_IS_PARAM (pspec), FALSE);
    x_return_val_if_fail (X_IS_VALUE (src_value), FALSE);
    x_return_val_if_fail (X_IS_VALUE (dest_value), FALSE);
    x_return_val_if_fail (HOLDS_VALUE_TYPE (pspec, dest_value), FALSE);

    /* better leave dest_value untouched when returning FALSE */

    x_value_init (&tmp_value, X_VALUE_TYPE (dest_value));
    if (x_value_transform (src_value, &tmp_value) &&
        (!x_param_value_validate (pspec, &tmp_value) || !strict_validation)) {

        x_value_clear (dest_value);
        memcpy (dest_value, &tmp_value, sizeof (tmp_value));

        return TRUE;
    }
    else {
        x_value_clear (&tmp_value);

        return FALSE;
    }
}

xint
x_param_value_cmp       (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    xint cmp;

    x_return_val_if_fail (X_IS_PARAM (pspec), 0);
    x_return_val_if_fail (X_IS_VALUE (value1), 0);
    x_return_val_if_fail (X_IS_VALUE (value2), 0);
    x_return_val_if_fail (HOLDS_VALUE_TYPE (pspec, value1), 0);
    x_return_val_if_fail (HOLDS_VALUE_TYPE (pspec, value2), 0);

    cmp = X_PARAM_GET_CLASS (pspec)->value_cmp (pspec, value1, value2);

    return CLAMP (cmp, -1, 1);
}
typedef struct
{
    xType           value_type;
    void (*finalize)        (xParam         *pspec);
    void (*value_init)      (const xParam   *pspec,
                             xValue         *value);
    xbool(*value_validate)  (const xParam   *pspec,
                             xValue         *value);
    xint (*value_cmp)       (const xParam   *pspec,
                             const xValue   *value1,
                             const xValue   *value2);
} ParamClassInfo;

static void
generic_class_init      (xParamClass    *klass,
                         ParamClassInfo *info)
{
    klass->value_type = info->value_type;
    if (info->finalize)
        klass->finalize = info->finalize;			/* optional */
    klass->value_init = info->value_init;
    if (info->value_validate)
        klass->value_validate = info->value_validate;	/* optional */
    klass->value_cmp = info->value_cmp;
    x_free (info);
}

static void
dflt_value_init         (const xParam   *pspec,
                         xValue         *value)
{
    /* value is already zero initialized */
}

static xint
dflt_value_cmp          (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    return memcmp (&value1->data, &value2->data, sizeof (value1->data));
}

xType
x_param_register_static (xcstr          name,
                         const xParamInfo   *pspec_info)
{
    xTypeInfo info  = {0,};
    ParamClassInfo  *cinfo;

    x_return_val_if_fail (name != NULL, 0);
    x_return_val_if_fail (pspec_info != NULL, 0);
    x_return_val_if_fail (x_type_from_name (name) == 0, 0);
    x_return_val_if_fail (pspec_info->instance_size >= sizeof (xParam), 0);
    x_return_val_if_fail (x_type_name (pspec_info->value_type) != NULL, 0);

    cinfo = x_new (ParamClassInfo, 1);
    cinfo->value_type   = pspec_info->value_type;
    cinfo->finalize     = pspec_info->finalize;
    cinfo->value_init   = pspec_info->value_init ? pspec_info->value_init : dflt_value_init;
    cinfo->value_validate = pspec_info->value_validate;
    cinfo->value_cmp    = pspec_info->value_cmp ? pspec_info->value_cmp : dflt_value_cmp;

    info.class_size     = sizeof(xParamClass);
    info.class_init     = (xClassInitFunc)generic_class_init;
    info.class_data     = cinfo;
    info.instance_size  = pspec_info->instance_size;
    info.instance_init  = (xInstanceInitFunc)pspec_info->instance_init;

    return x_type_register_static (name, X_TYPE_PARAM, &info, 0);
}


static void
param_value_init        (xValue         *value)
{
    value->data[0].v_pointer = NULL;
}

static void
param_value_free        (xValue         *value)
{
    if (value->data[0].v_pointer)
        x_param_unref (value->data[0].v_pointer);
}

static void
param_value_copy        (const xValue   *src,
                         xValue         *dest)
{
    if (src->data[0].v_pointer)
        dest->data[0].v_pointer = x_param_ref (src->data[0].v_pointer);
    else
        dest->data[0].v_pointer = NULL;
}

static void
param_value_transform   (const xValue   *src,
                         xValue         *dest)
{
    if (src->data[0].v_pointer &&
        x_type_is (X_PARAM_TYPE (dest->data[0].v_pointer), X_VALUE_TYPE (dest)))
        dest->data[0].v_pointer = x_param_ref (src->data[0].v_pointer);
    else
        dest->data[0].v_pointer = NULL;
}

static xptr
param_value_peek        (const xValue   *value)
{
    return value->data[0].v_pointer;
}

static xstr
param_value_collect     (xValue         *value,
                         xsize          n_collect,
                         xCValue        *collect,
                         xuint          flags)
{
    if (collect[0].v_pointer) {
        xParam *param = collect[0].v_pointer;

        if (X_INSTANCE_CLASS(param) == NULL)
            return x_strcat ("invalid unclassed param spec pointer for value type `",
                             X_VALUE_TYPE_NAME (value),
                             "'",
                             NULL);
        else if (!x_value_type_compatible (X_PARAM_TYPE (param), X_VALUE_TYPE (value)))
            return x_strcat ("invalid param spec type `",
                             X_PARAM_TYPE_NAME (param),
                             "' for value type `",
                             X_VALUE_TYPE_NAME (value),
                             "'",
                             NULL);
        value->data[0].v_pointer = x_param_ref (param);
    }
    else
        value->data[0].v_pointer = NULL;

    return NULL;
}

static xstr
param_value_lcopy       (const xValue   *value,
                         xsize          n_collect,
                         xCValue        *collect,
                         xuint          flags)
{
    xParam **param_p = collect[0].v_pointer;

    if (!param_p)
        return x_strdup_printf ("value location for `%s' passed as NULL", X_VALUE_TYPE_NAME (value));

    if (!value->data[0].v_pointer)
        *param_p = NULL;
    else if (flags & X_VALUE_NOCOPY_CONTENTS)
        *param_p = value->data[0].v_pointer;
    else
        *param_p = x_param_ref (value->data[0].v_pointer);

    return NULL;
}
static void
param_base_init         (xParamClass    *klass)
{
}

static void
param_base_finalize     (xParamClass    *klass)
{
}
static void
param_finalize          (xParam         *pspec)
{
    x_data_list_clear (&pspec->qdata);

    if (!(pspec->flags & X_PARAM_STATIC_NICK))
        x_free (pspec->nick);

    if (!(pspec->flags & X_PARAM_STATIC_BLURB))
        x_free (pspec->blurb);

    x_instance_delete (pspec);
}
static void
param_class_init        (xParamClass    *klass,
                         xptr           class_data)
{
    klass->value_type   = X_TYPE_VOID;
    klass->finalize     = param_finalize;
    klass->value_init   = NULL;
    klass->value_validate = NULL;
    klass->value_cmp    = NULL;
}

static void
param_init              (xParam         *pspec,
                         xParamClass    *klass)
{
    pspec->name         = 0;
    pspec->flags        = 0;
    pspec->value_type   = klass->value_type;
    pspec->owner_type   = 0;
    pspec->qdata        = NULL;
    pspec->ref_count    = 1;
    pspec->param_id     = 0;
    x_data_list_set_flags (&pspec->qdata, PARAM_FLOATING_FLAG);
}

xParam*
x_param_get_redirect    (xParam *pspec)
{
    x_return_val_if_fail (X_IS_PARAM (pspec), NULL);

    if (X_IS_PARAM_OVERRIDE (pspec))
    {
        xParamOverride *ospec = X_PARAM_OVERRIDE (pspec);

        return ospec->overridden;
    }
    else
        return NULL;
}
static inline xbool
is_canonical            (const xchar    *key)
{
    const xchar *p;

    for (p = key; *p != 0; p++)
    {
        xchar c = *p;

        if (c != '-' &&
            (c < '0' || c > '9') &&
            (c < 'A' || c > 'Z') &&
            (c < 'a' || c > 'z'))
            return FALSE;
    }

    return TRUE;
}
xptr
x_param_new             (xType          param_type,
                         xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xuint          flags)
{
    xParam  *pspec;

    x_return_val_if_fail (X_TYPE_IS_PARAM (param_type) && param_type != X_TYPE_PARAM, NULL);
    x_return_val_if_fail (name != NULL, NULL);
    x_return_val_if_fail ((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= 'a' && name[0] <= 'z'), NULL);
    x_return_val_if_fail (!(flags & X_PARAM_STATIC_NAME) || is_canonical (name), NULL);

    pspec = x_instance_new (param_type);

    if (flags & X_PARAM_STATIC_NAME) {
        /* pspec->name is not freed if (flags & G_PARAM_STATIC_NAME) */

        if (!is_canonical (name))
            x_warning ("X_PARAM_STATIC_NAME used with non-canonical pspec name: %s",
                       x_quark_str(pspec->name));

        pspec->name = x_quark_from_static(name);
    }
    else
    {
        if (is_canonical (name))
            pspec->name = x_quark_from(name);
        else
        {
            xchar *tmp = x_strdup (name);
            //canonicalize_key (tmp);
            pspec->name = x_quark_from(tmp);
            x_free (tmp);
        }
    }

    if (flags & X_PARAM_STATIC_NICK)
        pspec->nick = (xstr) nick;
    else
        pspec->nick = x_strdup (nick);

    if (flags & X_PARAM_STATIC_BLURB)
        pspec->blurb = (xstr) blurb;
    else
        pspec->blurb = x_strdup (blurb);

    pspec->flags = (flags & X_PARAM_USER_MASK) | (flags & X_PARAM_MASK);

    return pspec;
}
xParam*
x_param_char_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xchar          minimum,
                         xchar          maximum,
                         xchar          default_value,
                         xuint          flags)
{
    xParamChar   *ispec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    ispec = x_param_new	(X_TYPE_PARAM_UINT,
                         name, nick, blurb, flags);

    ispec->minimum = minimum;
    ispec->maximum = maximum;
    ispec->default_value = default_value;

    return X_PARAM (ispec);
}
xParam*
x_param_uchar_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xuchar         minimum,
                         xuchar         maximum,
                         xuchar         default_value,
                         xuint          flags)
{
    xParamUChar  *ispec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    ispec = x_param_new	(X_TYPE_PARAM_UINT,
                         name, nick, blurb, flags);

    ispec->minimum = minimum;
    ispec->maximum = maximum;
    ispec->default_value = default_value;

    return X_PARAM (ispec);
}

xParam*
x_param_bool_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xbool          default_value,
                         xuint          flags)
{
    xParamBool   *ispec;

    x_return_val_if_fail (default_value == TRUE
                          || default_value == FALSE, NULL);

    ispec = x_param_new	(X_TYPE_PARAM_BOOL,
                         name, nick, blurb, flags);

    ispec->default_value = default_value;

    return X_PARAM (ispec);
}
xParam*
x_param_int_new		    (xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xint	        minimum,
                         xint	        maximum,
                         xint	        default_value,
                         xuint          flags)
{
    xParamInt    *ispec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    ispec = x_param_new	(X_TYPE_PARAM_INT,
                         name, nick, blurb, flags);

    ispec->minimum = minimum;
    ispec->maximum = maximum;
    ispec->default_value = default_value;

    return X_PARAM (ispec);
}
xParam*
x_param_uint_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xuint	        minimum,
                         xuint	        maximum,
                         xuint	        default_value,
                         xuint          flags)
{
    xParamUInt *ispec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    ispec = x_param_new	(X_TYPE_PARAM_UINT,
                         name, nick, blurb, flags);

    ispec->minimum = minimum;
    ispec->maximum = maximum;
    ispec->default_value = default_value;

    return X_PARAM (ispec);
}
xParam*
x_param_long_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xlong          minimum,
                         xlong          maximum,
                         xlong          default_value,
                         xuint          flags)
{
    xParamLong *lspec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    lspec = x_param_new	(X_TYPE_PARAM_LONG,
                         name, nick, blurb, flags);

    lspec->minimum = minimum;
    lspec->maximum = maximum;
    lspec->default_value = default_value;

    return X_PARAM (lspec);
}

xParam*
x_param_ulong_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xulong         minimum,
                         xulong         maximum,
                         xulong         default_value,
                         xuint          flags)
{
    xParamULong *uspec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    uspec = x_param_new	(X_TYPE_PARAM_ULONG,
                         name, nick, blurb, flags);

    uspec->minimum = minimum;
    uspec->maximum = maximum;
    uspec->default_value = default_value;

    return X_PARAM (uspec);
}

xParam*
x_param_int64_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xint64         minimum,
                         xint64         maximum,
                         xint64         default_value,
                         xuint          flags)
{
    xParamInt64 *ispec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    ispec = x_param_new	(X_TYPE_PARAM_INT64,
                         name, nick, blurb, flags);

    ispec->minimum = minimum;
    ispec->maximum = maximum;
    ispec->default_value = default_value;

    return X_PARAM (ispec);
}

xParam*
x_param_uint64_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xuint64        minimum,
                         xuint64        maximum,
                         xuint64        default_value,
                         xuint          flags)
{
    xParamUInt64 *uspec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    uspec = x_param_new	(X_TYPE_PARAM_INT64,
                         name, nick, blurb, flags);

    uspec->minimum = minimum;
    uspec->maximum = maximum;
    uspec->default_value = default_value;

    return X_PARAM (uspec);
}

xParam*
x_param_wchar_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xwchar         default_value,
                         xuint          flags)
{
    xParamWChar *uspec;

    uspec = x_param_new	(X_TYPE_PARAM_WCHAR,
                         name, nick, blurb, flags);

    uspec->default_value = default_value;

    return X_PARAM (uspec);
}

xParam*
x_param_enum_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xType          enum_type,
                         xint           default_value,
                         xuint          flags)
{
    xParamEnum *espec;
    xEnumClass *enum_class;

    x_return_val_if_fail (X_TYPE_IS_ENUM (enum_type), NULL);

    enum_class = x_class_ref (enum_type);

    x_return_val_if_fail (x_enum_get_value (enum_class, default_value) != NULL, NULL);

    espec = x_param_new	(X_TYPE_PARAM_ENUM,
                         name, nick, blurb, flags);

    espec->enum_class = enum_class;
    espec->default_value = default_value;

    X_PARAM (espec)->value_type = enum_type;
    x_class_unref (enum_class);

    return X_PARAM (espec);
}

xParam*
x_param_flags_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xType          flags_type,
                         xuint          default_value,
                         xuint          flags)
{
    xParamFlags *fspec;
    xFlagsClass *flags_class;

    x_return_val_if_fail (X_TYPE_IS_FLAGS (flags_type), NULL);

    flags_class = x_class_ref (flags_type);

    x_return_val_if_fail ((default_value & flags_class->mask) == default_value, NULL);

    fspec = x_param_new	(X_TYPE_PARAM_FLAGS,
                         name, nick, blurb, flags);

    fspec->flags_class = flags_class;
    fspec->default_value = default_value;
    X_PARAM (fspec)->value_type = flags_type;
    x_class_unref (flags_class);

    return X_PARAM (fspec);
}

xParam*
x_param_float_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xfloat         minimum,
                         xfloat         maximum,
                         xfloat         default_value,
                         xuint          flags)
{
    xParamFloat *fspec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    fspec = x_param_new	(X_TYPE_PARAM_FLOAT,
                         name, nick, blurb, flags);

    fspec->minimum = minimum;
    fspec->maximum = maximum;
    fspec->default_value = default_value;

    return X_PARAM (fspec);
}

xParam*
x_param_double_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xdouble        minimum,
                         xdouble        maximum,
                         xdouble        default_value,
                         xuint          flags)
{
    xParamDouble *dspec;

    x_return_val_if_fail (default_value >= minimum
                          && default_value <= maximum, NULL);

    dspec = x_param_new	(X_TYPE_PARAM_DOUBLE,
                         name, nick, blurb, flags);

    dspec->minimum = minimum;
    dspec->maximum = maximum;
    dspec->default_value = default_value;

    return X_PARAM (dspec);
}

xParam*
x_param_str_new		(xcstr          name,
                     xcstr          nick,
                     xcstr          blurb,
                     xcstr          default_value,
                     xuint          flags)
{
    xParamStr *sspec;

    sspec = x_param_new (X_TYPE_PARAM_STR,
                         name, nick, blurb, flags);

    x_free (sspec->default_value);
    sspec->default_value = x_strdup (default_value);

    return X_PARAM (sspec);
}

xParam*
x_param_strv_new		(xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xuint          flags)
{
    xParamStrV *vspec;

    vspec = x_param_new (X_TYPE_PARAM_STRV,
                         name, nick, blurb, flags);

    return X_PARAM (vspec);
}
xParam*
x_param_boxed_new       (xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xType          boxed_type,
                         xuint	        flags)
{
    xParamBoxed *bspec;

    x_return_val_if_fail (X_TYPE_IS_BOXED (boxed_type), NULL);
    x_return_val_if_fail (X_TYPE_IS_VALUE (boxed_type), NULL);

    bspec = x_param_new (X_TYPE_PARAM_BOXED,
                         name, nick, blurb, flags);
    X_PARAM (bspec)->value_type = boxed_type;

    return X_PARAM (bspec);
}
xParam*
x_param_ptr_new         (xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xuint	        flags)
{
    xParamPtr *pspec;

    pspec = x_param_new (X_TYPE_PARAM_PTR,
                         name, nick, blurb, flags);

    return X_PARAM (pspec);
}

xParam*
x_param_object_new      (xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         xType          object_type,
                         xuint          flags)
{
    xParamObject *ospec;

    x_return_val_if_fail (X_TYPE_IS_OBJECT (object_type), NULL);

    ospec = x_param_new (X_TYPE_PARAM_OBJECT,
                         name, nick, blurb, flags);
    X_PARAM (ospec)->value_type = object_type;

    return X_PARAM (ospec);
}
xParam*
x_param_override_new    (xcstr          name,
                         xParam*        overridden)
{
    xParam *pspec, *indirect;

    x_return_val_if_fail (name != NULL, NULL);
    x_return_val_if_fail (X_IS_PARAM (overridden), NULL);

    do {
        indirect = overridden;
        overridden = x_param_get_redirect (indirect);
    } while (overridden != NULL);

    pspec = x_param_new (X_TYPE_PARAM_OVERRIDE,
                         name, indirect->nick, indirect->blurb,
                         indirect->flags | X_PARAM_STATIC_STR);
                         
    pspec->flags &= ~(X_PARAM_CONSTRUCT|X_PARAM_CONSTRUCT_ONLY);
    pspec->value_type = X_PARAM_VALUE_TYPE (indirect);
    X_PARAM_OVERRIDE (pspec)->overridden = x_param_ref (indirect);

    return X_PARAM (pspec);
}
/* X_TYPE_PARAM_CHAR */
static void
param_char_init         (xParam         *pspec)
{
    xParamChar *cspec = X_PARAM_CHAR (pspec);

    cspec->minimum = 0x7f;
    cspec->maximum = 0x80;
    cspec->default_value = 0;
}
#define param_char_final    NULL
static void
param_char_default      (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_int = X_PARAM_CHAR (pspec)->default_value;
}
static xbool
param_char_validate     (const xParam   *pspec,
                         xValue         *value)
{
    xParamChar *cspec = X_PARAM_CHAR (pspec);
    xint oval = value->data[0].v_int;

    value->data[0].v_int = CLAMP (value->data[0].v_int,
                                  cspec->minimum, cspec->maximum);

    return value->data[0].v_int != oval;
}
#define param_char_cmp  param_int_cmp
/* X_TYPE_PARAM_UCHAR */
static void
param_uchar_init        (xParam         *pspec)
{
    xParamUChar *uspec = X_PARAM_UCHAR (pspec);

    uspec->minimum = 0;
    uspec->maximum = 0xff;
    uspec->default_value = 0;
}
#define param_uchar_final   NULL
static void
param_uchar_default     (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_uint = X_PARAM_UCHAR (pspec)->default_value;
}
static xbool
param_uchar_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xParamUChar *uspec = X_PARAM_UCHAR (pspec);
    xuint oval = value->data[0].v_uint;

    value->data[0].v_uint = CLAMP (value->data[0].v_uint,
                                   uspec->minimum, uspec->maximum);

    return value->data[0].v_uint != oval;
}
#define param_uchar_cmp param_int_cmp
/* X_TYPE_PARAM_BOOL */
#define param_bool_init     NULL
#define param_bool_final    NULL
static void
param_bool_default      (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_uint = X_PARAM_BOOL (pspec)->default_value;
}
static xbool
param_bool_validate     (const xParam   *pspec,
                         xValue         *value)
{
    xint oval = value->data[0].v_int;

    value->data[0].v_int = value->data[0].v_int != FALSE;

    return value->data[0].v_int != oval;
}
#define param_bool_cmp  param_int_cmp
/* X_TYPE_PARAM_INT */
static void
param_int_init          (xParam         *pspec)
{
    xParamInt *ispec = X_PARAM_INT (pspec);

    ispec->minimum = 0x7fffffff;
    ispec->maximum = 0x80000000;
    ispec->default_value = 0;
}
#define param_int_final    NULL
static void
param_int_default       (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_int = X_PARAM_INT (pspec)->default_value;
}
static xbool
param_int_validate      (const xParam   *pspec,
                         xValue         *value)
{
    xParamInt *ispec = X_PARAM_INT (pspec);
    xint oval = value->data[0].v_int;

    value->data[0].v_int = CLAMP (value->data[0].v_int,
                                  ispec->minimum, ispec->maximum);

    return value->data[0].v_int != oval;
}
static xint
param_int_cmp           (const xParam      *pspec,
                         const xValue      *value1,
                         const xValue      *value2)
{
    if (value1->data[0].v_int < value2->data[0].v_int)
        return -1;
    else
        return value1->data[0].v_int > value2->data[0].v_int;
}
/* X_TYPE_PARAM_UINT */
static void
param_uint_init         (xParam         *pspec)
{
    xParamUInt *uspec = X_PARAM_UINT (pspec);

    uspec->minimum = 0;
    uspec->maximum = 0xffffffff;
    uspec->default_value = 0;
}
#define param_uint_final    NULL
static void
param_uint_default      (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_uint = X_PARAM_UINT (pspec)->default_value;
}
static xbool
param_uint_validate     (const xParam   *pspec,
                         xValue         *value)
{
    xParamUInt *uspec = X_PARAM_UINT (pspec);
    xuint oval = value->data[0].v_uint;

    value->data[0].v_uint = CLAMP (value->data[0].v_uint,
                                   uspec->minimum, uspec->maximum);

    return value->data[0].v_uint != oval;
}
static xint
param_uint_cmp          (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_uint < value2->data[0].v_uint)
        return -1;
    else
        return value1->data[0].v_uint > value2->data[0].v_uint;
}
/* X_TYPE_PARAM_LONG */
static void
param_long_init         (xParam         *pspec)
{
    xParamLong *lspec = X_PARAM_LONG (pspec);

#if X_SIZEOF_LONG == 4
    lspec->minimum = 0x7fffffff;
    lspec->maximum = 0x80000000;
#else /* SIZEOF_LONG != 4 (8) */
    lspec->minimum = 0x7fffffffffffffff;
    lspec->maximum = 0x8000000000000000;
#endif
    lspec->default_value = 0;
}
#define param_long_final    NULL
static void
param_long_default      (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_long = X_PARAM_LONG (pspec)->default_value;
}
static xbool
param_long_validate     (const xParam   *pspec,
                         xValue         *value)
{
    xParamLong *lspec = X_PARAM_LONG (pspec);
    xlong oval = value->data[0].v_long;

    value->data[0].v_long = CLAMP (value->data[0].v_long,
                                   lspec->minimum, lspec->maximum);

    return value->data[0].v_long != oval;
}
static xint
param_long_cmp          (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_long < value2->data[0].v_long)
        return -1;
    else
        return value1->data[0].v_long > value2->data[0].v_long;
}
/* X_TYPE_PARAM_ULONG */
static void
param_ulong_init        (xParam         *pspec)
{
    xParamULong *uspec = X_PARAM_ULONG (pspec);

    uspec->minimum = 0;
#if X_SIZEOF_LONG == 4
    uspec->maximum = 0xffffffff;
#else /* SIZEOF_LONG != 4 (8) */
    uspec->maximum = 0xffffffffffffffff;
#endif
    uspec->default_value = 0;
}
#define param_ulong_final   NULL
static void
param_ulong_default     (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_ulong = X_PARAM_ULONG (pspec)->default_value;
}
static xbool
param_ulong_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xParamULong *uspec = X_PARAM_ULONG (pspec);
    xulong oval = value->data[0].v_ulong;

    value->data[0].v_ulong = CLAMP (value->data[0].v_ulong,
                                    uspec->minimum, uspec->maximum);

    return value->data[0].v_ulong != oval;
}
static xint
param_ulong_cmp         (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_ulong < value2->data[0].v_ulong)
        return -1;
    else
        return value1->data[0].v_ulong > value2->data[0].v_ulong;
}
/* X_TYPE_PARAM_INT64 */
static void
param_int64_init        (xParam         *pspec)
{
    xParamInt64 *lspec = X_PARAM_INT64(pspec);

    lspec->minimum = X_MININT64;
    lspec->maximum = X_MAXINT64;
    lspec->default_value = 0;
}
#define param_int64_final   NULL
static void
param_int64_default     (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_int64 = X_PARAM_INT64 (pspec)->default_value;
}
static xbool
param_int64_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xParamInt64 *lspec = X_PARAM_INT64 (pspec);
    xint64 oval = value->data[0].v_int64;

    value->data[0].v_int64 = CLAMP (value->data[0].v_int64,
                                    lspec->minimum, lspec->maximum);

    return value->data[0].v_int64 != oval;
}
static xint
param_int64_cmp         (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_int64 < value2->data[0].v_int64)
        return -1;
    else
        return value1->data[0].v_int64 > value2->data[0].v_int64;
}
/* X_TYPE_PARAM_UINT64 */
static void
param_uint64_init       (xParam         *pspec)
{
    xParamUInt64 *uspec = X_PARAM_UINT64(pspec);

    uspec->minimum = 0;
    uspec->maximum = X_MAXUINT64;
    uspec->default_value = 0;
}
#define param_uint64_final   NULL
static void
param_uint64_default    (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_uint64 = X_PARAM_UINT64 (pspec)->default_value;
}
static xbool
param_uint64_validate   (const xParam   *pspec,
                         xValue         *value)
{
    xParamUInt64 *uspec = X_PARAM_UINT64 (pspec);
    xuint64 oval = value->data[0].v_uint64;

    value->data[0].v_uint64 = CLAMP (value->data[0].v_uint64,
                                     uspec->minimum, uspec->maximum);

    return value->data[0].v_uint64 != oval;
}
static xint
param_uint64_cmp        (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_uint64 < value2->data[0].v_uint64)
        return -1;
    else
        return value1->data[0].v_uint64 > value2->data[0].v_uint64;
}
/* X_TYPE_PARAM_WCHAR */
static void
param_wchar_init        (xParam         *pspec)
{
    xParamWChar *uspec = X_PARAM_WCHAR (pspec);

    uspec->default_value = 0;
}
#define param_wchar_final   NULL
static void
param_wchar_default     (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_uint = X_PARAM_WCHAR (pspec)->default_value;
}
static xbool
param_wchar_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xwchar oval = value->data[0].v_uint;
    xbool  changed = FALSE;

    if (!x_wchar_validate (oval)) {
        value->data[0].v_uint = 0;
        changed = TRUE;
    }
    return changed;
}
static xint
param_wchar_cmp         (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_uint < value2->data[0].v_uint)
        return -1;
    else
        return value1->data[0].v_uint > value2->data[0].v_uint;
}
/* X_TYPE_PARAM_ENUM */
static void
param_enum_init         (xParam         *pspec)
{
    xParamEnum *espec = X_PARAM_ENUM (pspec);

    espec->enum_class = NULL;
    espec->default_value = 0;
}
static void
param_enum_final        (xParam         *pspec)
{
    xParamEnum *espec = X_PARAM_ENUM (pspec);
    xParamClass *parent_class = x_class_peek (x_type_parent (X_TYPE_PARAM_ENUM));

    if (espec->enum_class) {
        x_class_unref (espec->enum_class);
        espec->enum_class = NULL;
    }

    parent_class->finalize (pspec);
}
static void
param_enum_default    (const xParam   *pspec,
                       xValue	        *value)
{
    value->data[0].v_long = X_PARAM_ENUM (pspec)->default_value;
}
static xbool
param_enum_validate   (const xParam   *pspec,
                       xValue         *value)
{
    xParamEnum *espec = X_PARAM_ENUM (pspec);
    xlong oval = value->data[0].v_long;
    //
    //   if (!espec->enum_class ||
    //      !g_enum_get_value (espec->enum_class, value->data[0].v_long))
    //       value->data[0].v_long = espec->default_value;

    return value->data[0].v_long != oval;
}
#define param_enum_cmp  param_long_cmp
/* X_TYPE_PARAM_FLAGS */
static void
param_flags_init        (xParam         *pspec)
{
    xParamFlags *fspec = X_PARAM_FLAGS (pspec);

    fspec->flags_class = NULL;
    fspec->default_value = 0;
}
static void
param_flags_final       (xParam         *pspec)
{
    xParamFlags *fspec = X_PARAM_FLAGS (pspec);
    xParamClass *parent_class = x_class_peek (x_type_parent (X_TYPE_PARAM_FLAGS));

    if (fspec->flags_class) {
        x_class_unref (fspec->flags_class);
        fspec->flags_class = NULL;
    }
    parent_class->finalize (pspec);
}
static void
param_flags_default     (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_ulong = X_PARAM_FLAGS (pspec)->default_value;
}
static xbool
param_flags_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xParamFlags *fspec = X_PARAM_FLAGS (pspec);
    xulong oval = value->data[0].v_ulong;

    if (fspec->flags_class)
        value->data[0].v_ulong &= fspec->flags_class->mask;
    else
        value->data[0].v_ulong = fspec->default_value;

    return value->data[0].v_ulong != oval;
}
#define param_flags_cmp param_ulong_cmp
/* X_TYPE_PARAM_FLOAT */
static void
param_float_init        (xParam         *pspec)
{
    xParamFloat *fspec = X_PARAM_FLOAT (pspec);

    fspec->minimum = -X_MAXFLOAT;
    fspec->maximum = X_MAXFLOAT;
    fspec->default_value = 0;
    fspec->epsilon = X_FLOAT_EPSILON;
}
#define param_float_final       NULL
static void
param_float_default     (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_float = X_PARAM_FLOAT (pspec)->default_value;
}
static xbool
param_float_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xParamFloat *fspec = X_PARAM_FLOAT (pspec);
    xfloat oval = value->data[0].v_float;

    value->data[0].v_float = CLAMP (value->data[0].v_float,
                                    fspec->minimum, fspec->maximum);

    return value->data[0].v_float != oval;
}
static xint
param_float_cmp         (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    xfloat epsilon = X_PARAM_FLOAT (pspec)->epsilon;

    if (value1->data[0].v_float < value2->data[0].v_float)
        return - (value2->data[0].v_float - value1->data[0].v_float > epsilon);
    else
        return value1->data[0].v_float - value2->data[0].v_float > epsilon;
}
/* X_TYPE_PARAM_DOUBLE */
static void
param_double_init       (xParam         *pspec)
{
    xParamDouble *dspec = X_PARAM_DOUBLE (pspec);

    dspec->minimum = -X_MAXDOUBLE;
    dspec->maximum = X_MAXDOUBLE;
    dspec->default_value = 0;
    dspec->epsilon = X_DOUBLE_EPSILON;
}
#define param_double_final   NULL
static void
param_double_default    (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_double = X_PARAM_DOUBLE (pspec)->default_value;
}
static xbool
param_double_validate   (const xParam   *pspec,
                         xValue         *value)
{
    xParamDouble *dspec = X_PARAM_DOUBLE (pspec);
    xdouble oval = value->data[0].v_double;

    value->data[0].v_double = CLAMP (value->data[0].v_double,
                                     dspec->minimum, dspec->maximum);

    return value->data[0].v_double != oval;
}
static xint
param_double_cmp        (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    xdouble epsilon = X_PARAM_DOUBLE (pspec)->epsilon;

    if (value1->data[0].v_double < value2->data[0].v_double)
        return - (value2->data[0].v_double - value1->data[0].v_double > epsilon);
    else
        return value1->data[0].v_double - value2->data[0].v_double > epsilon;
}
/* X_TYPE_PARAM_STR */
static void
param_str_init          (xParam         *pspec)
{
    xParamStr *sspec = X_PARAM_STR (pspec);

    sspec->default_value = NULL;
    sspec->cset_first = NULL;
    sspec->cset_nth = NULL;
    sspec->substitutor = '_';
    sspec->null_fold_if_empty = FALSE;
    sspec->ensure_non_null = FALSE;
}
static void
param_str_final         (xParam         *pspec)
{
    xParamStr *sspec = X_PARAM_STR (pspec);
    xParamClass *parent_class = 
        x_class_peek (x_type_parent (X_TYPE_PARAM_STR));

    x_free (sspec->default_value);
    x_free (sspec->cset_first);
    x_free (sspec->cset_nth);
    sspec->default_value = NULL;
    sspec->cset_first = NULL;
    sspec->cset_nth = NULL;

    parent_class->finalize (pspec);
}
static void
param_str_default       (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_pointer = 
        x_strdup (X_PARAM_STR (pspec)->default_value);
}
static xbool
param_str_validate      (const xParam   *pspec,
                         xValue         *value)
{
    xParamStr *sspec = X_PARAM_STR (pspec);
    xchar *string = value->data[0].v_pointer;
    xbool changed = FALSE;

    if (string && string[0]) {
        xchar *s;

        if (sspec->cset_first && !strchr (sspec->cset_first, string[0])) {
            if (value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS) {
                value->data[0].v_pointer = x_strdup (string);
                string = value->data[0].v_pointer;
                value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
            }
            string[0] = sspec->substitutor;
            changed = TRUE;
        }
        if (sspec->cset_nth)
            for (s = string + 1; *s; s++)
                if (!strchr (sspec->cset_nth, *s)) {
                    if (value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)
                    {
                        value->data[0].v_pointer = x_strdup (string);
                        s = (xchar*) value->data[0].v_pointer + (s - string);
                        string = value->data[0].v_pointer;
                        value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
                    }
                    *s = sspec->substitutor;
                    changed = TRUE;
                }
    }
    if (sspec->null_fold_if_empty && string && string[0] == 0) {
        if (!(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS))
            x_free (value->data[0].v_pointer);
        else
            value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
        value->data[0].v_pointer = NULL;
        changed = TRUE;
        string = value->data[0].v_pointer;
    }
    if (sspec->ensure_non_null && !string) {
        value->data[1].v_uint &= ~X_VALUE_NOCOPY_CONTENTS;
        value->data[0].v_pointer = x_strdup ("");
        changed = TRUE;
        string = value->data[0].v_pointer;
    }
    return changed;
}
static xint
param_str_cmp           (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (!value1->data[0].v_pointer)
        return value2->data[0].v_pointer != NULL ? -1 : 0;
    else if (!value2->data[0].v_pointer)
        return value1->data[0].v_pointer != NULL;
    else
        return strcmp (value1->data[0].v_pointer, value2->data[0].v_pointer);
}
/* X_TYPE_PARAM_PARAM */
#define param_param_init    param_ptr_init
#define param_param_final   NULL
#define param_param_default param_ptr_default
static xbool
param_param_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xParam *param = value->data[0].v_pointer;
    xbool changed = FALSE;

    if (param && !x_value_type_compatible (X_PARAM_TYPE (param),
                                           X_PARAM_VALUE_TYPE (pspec)))
    {
        x_param_unref (param);
        value->data[0].v_pointer = NULL;
        changed = TRUE;
    }
    return changed;
}
#define param_param_cmp param_ptr_cmp
/* X_TYPE_PARAM_BOXED */
#define param_boxed_init    param_ptr_init
#define param_boxed_final   NULL
#define param_boxed_default param_ptr_default
#define param_boxed_validate param_ptr_validate
#define param_boxed_cmp     param_ptr_cmp
/* X_TYPE_PARAM_PTR */
static void
param_ptr_init          (xParam         *pspec)
{
}
#define param_ptr_final   NULL
static void
param_ptr_default       (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_pointer = NULL;
}
static xbool
param_ptr_validate      (const xParam   *pspec,
                         xValue         *value)
{
    return FALSE;
}
static xint
param_ptr_cmp           (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_pointer < value2->data[0].v_pointer)
        return -1;
    else
        return value1->data[0].v_pointer > value2->data[0].v_pointer;
}
/* X_TYPE_PARAM_OBJECT */
#define param_object_init    param_ptr_init
#define param_object_final   NULL
#define param_object_default param_ptr_default
#define param_boxed_validate param_ptr_validate
static xbool
param_object_validate   (const xParam   *pspec,
                         xValue         *value)
{
    xParamObject *ospec = X_PARAM_OBJECT (pspec);
    xObject *object = value->data[0].v_pointer;
    xbool changed = FALSE;

    if (object && !x_value_type_compatible (X_OBJECT_TYPE (object),
                                            X_PARAM_VALUE_TYPE (ospec))) {
        x_object_unref (object);
        value->data[0].v_pointer = NULL;
        changed = TRUE;
    }

    return changed;
}
#define param_object_cmp     param_ptr_cmp
/* X_TYPE_PARAM_OVERRIDE */
static void
param_override_init     (xParam         *pspec)
{
}
static void
param_override_final    (xParam         *pspec)
{
    xParamOverride  *ospec = X_PARAM_OVERRIDE (pspec);
    xParamClass *parent_class =
        x_class_peek (x_type_parent (X_TYPE_PARAM_OVERRIDE));

    if (ospec->overridden) {
        x_param_unref (ospec->overridden);
        ospec->overridden = NULL;
    }

    parent_class->finalize (pspec);
}
static void
param_override_default  (const xParam   *pspec,
                         xValue	        *value)
{
    xParamOverride  *ospec = X_PARAM_OVERRIDE (pspec);
    x_param_value_default (ospec->overridden, value);
}
static xbool
param_override_validate (const xParam   *pspec,
                         xValue         *value)
{
    xParamOverride  *ospec = X_PARAM_OVERRIDE (pspec);
    return x_param_value_validate (ospec->overridden, value);
}
static xint
param_override_cmp      (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    xParamOverride  *ospec = X_PARAM_OVERRIDE (pspec);
    return x_param_value_cmp (ospec->overridden, value1, value2);
}
/* X_TYPE_PARAM_XTYPE */
static void
param_xtype_init        (xParam         *pspec)
{
}
#define param_xtype_final   NULL
static void
param_xtype_default     (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_uint64 = X_PARAM_UINT64 (pspec)->default_value;
}
static xbool
param_xtype_validate    (const xParam   *pspec,
                         xValue         *value)
{
    xParamXType *tspec = X_PARAM_XTYPE (pspec);
    xType xtype = value->data[0].v_long;
    xbool changed = FALSE;

    if (tspec->xtype != X_TYPE_VOID && !x_type_is (xtype, tspec->xtype))
    {
        value->data[0].v_long = tspec->xtype;
        changed = TRUE;
    }

    return changed;
}
static xint
param_xtype_cmp         (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    if (value1->data[0].v_long < value2->data[0].v_long)
        return -1;
    else
        return value1->data[0].v_long > value2->data[0].v_long;
}
/* X_TYPE_PARAM_STRV */
static void
param_strv_init         (xParam         *pspec)
{
}
#define param_strv_final   NULL
static void
param_strv_default      (const xParam   *pspec,
                         xValue	        *value)
{
    value->data[0].v_pointer = NULL;
}
static xbool
param_strv_validate     (const xParam   *pspec,
                         xValue         *value)
{
    return FALSE;
}
static xint
param_strv_cmp          (const xParam   *pspec,
                         const xValue   *value1,
                         const xValue   *value2)
{
    return x_strv_cmp (value1->data[0].v_pointer,
                       value2->data[0].v_pointer);
}
void
_x_param_types_init     (void)
{
    xType       type;
    xType       *param_types;
    const xuint n_types = 23;
    {
        const xFundamentalInfo finfo = {
            (X_TYPE_CLASSED |
             X_TYPE_INSTANTIATABLE |
             X_TYPE_DERIVABLE |
             X_TYPE_DEEP_DERIVABLE),
        };
        const xValueTable value_table = {
            param_value_init,
            param_value_free,
            param_value_copy,
            param_value_peek,
            X_VALUE_COLLECT_PTR,
            param_value_collect,
            X_VALUE_COLLECT_PTR,
            param_value_lcopy,
        };
        const xTypeInfo info = {
            sizeof (xParamClass),
            (xBaseInitFunc)param_base_init,
            (xBaseFinalizeFunc)param_base_finalize,
            (xClassInitFunc)param_class_init,
            NULL,
            NULL,
            sizeof (xParam),
            (xInstanceInitFunc)param_init,
            &value_table
        };

        x_type_register_fundamental (X_TYPE_PARAM,
                                     "xParam",
                                     &info,
                                     &finfo,
                                     X_TYPE_ABSTRACT);
        x_value_register_transform (X_TYPE_PARAM,
                                    X_TYPE_PARAM,
                                    param_value_transform);
    }
    _param_types =  param_types = x_new0 (xType, n_types);
    /* X_TYPE_PARAM_CHAR*/
    {
        const xParamInfo pspec_info = {
            sizeof (xParamChar),
            param_char_init,
            X_TYPE_CHAR,
            param_char_final,
            param_char_default,
            param_char_validate,
            param_char_cmp,
        };
        type = x_param_register_static (x_intern_static_str("xParamChar"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_CHAR);
    }
    /* X_TYPE_PARAM_UCHAR*/
    {
        const xParamInfo pspec_info = {
            sizeof (xParamUChar),
            param_uchar_init,
            X_TYPE_UCHAR,
            param_uchar_final,
            param_uchar_default,
            param_uchar_validate,
            param_uchar_cmp,
        };
        type = x_param_register_static (x_intern_static_str("xParamUChar"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_UCHAR);
    }
    /* X_TYPE_PARAM_BOOL */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamChar),
            param_bool_init,
            X_TYPE_BOOL,
            param_bool_final,
            param_bool_default,
            param_bool_validate,
            param_bool_cmp,
        };
        type = x_param_register_static (x_intern_static_str("xParamBool"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_BOOL);
    }
    /* X_TYPE_PARAM_INT*/
    {
        const xParamInfo pspec_info = {
            sizeof (xParamInt),
            param_int_init,
            X_TYPE_INT,
            param_int_final,
            param_int_default,
            param_int_validate,
            param_int_cmp,
        };
        type = x_param_register_static (x_intern_static_str("xParamInt"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_INT);
    }
    /* X_TYPE_PARAM_UINT*/
    {
        const xParamInfo pspec_info = {
            sizeof (xParamUInt),
            param_uint_init,
            X_TYPE_UINT,
            param_uint_final,
            param_uint_default,
            param_uint_validate,
            param_uint_cmp,
        };
        type = x_param_register_static (x_intern_static_str("xParamUInt"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_UINT);
    }
    /* X_TYPE_PARAM_LONG */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamLong),
            param_long_init,
            X_TYPE_LONG,
            param_long_final,
            param_long_default,
            param_long_validate,
            param_long_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamLong"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_LONG);
    }
    /* X_TYPE_PARAM_ULONG */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamULong),
            param_ulong_init,
            X_TYPE_ULONG,
            param_ulong_final,
            param_ulong_default,
            param_ulong_validate,
            param_ulong_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamULong"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_ULONG);
    }
    /* X_TYPE_PARAM_INT64 */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamInt64),
            param_int64_init,
            X_TYPE_INT64,
            param_int64_final,
            param_int64_default,
            param_int64_validate,
            param_int64_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamInt64"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_INT64);
    }
    /* X_TYPE_PARAM_UINT64 */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamUInt64),
            param_uint64_init,
            X_TYPE_UINT64,
            param_uint64_final,
            param_uint64_default,
            param_uint64_validate,
            param_uint64_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamUInt64"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_UINT64);
    }
    /* X_TYPE_PARAM_WCHAR */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamWChar),
            param_wchar_init,
            X_TYPE_UINT,
            param_wchar_final,
            param_wchar_default,
            param_wchar_validate,
            param_wchar_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamWChar"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_WCHAR);
    }
    /* X_TYPE_PARAM_ENUM */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamEnum),
            param_enum_init,
            X_TYPE_ENUM,
            param_enum_final,
            param_enum_default,
            param_enum_validate,
            param_long_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamEnum"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_ENUM);
    }
    /* X_TYPE_PARAM_FLAGS */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamFlags),
            param_flags_init,
            X_TYPE_FLAGS,
            param_flags_final,
            param_flags_default,
            param_flags_validate,
            param_ulong_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamFlags"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_FLAGS);
    }
    /* X_TYPE_PARAM_FLOAT */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamFloat),
            param_float_init,
            X_TYPE_FLOAT,
            param_float_final,
            param_float_default,
            param_float_validate,
            param_float_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamFloat"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_FLOAT);
    }
    /* X_TYPE_PARAM_DOUBLE */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamDouble),
            param_double_init,
            X_TYPE_DOUBLE,
            param_double_final,
            param_double_default,
            param_double_validate,
            param_double_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamDouble"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_DOUBLE);
    }
    /* X_TYPE_PARAM_STR */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamStr),
            param_str_init,
            X_TYPE_STR,
            param_str_final,
            param_str_default,
            param_str_validate,
            param_str_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamStr"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_STR);
    }
    /* X_TYPE_PARAM_PARAM */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamParam),
            param_param_init,
            X_TYPE_PARAM,
            param_param_init,
            param_param_default,
            param_param_validate,
            param_param_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamParam"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_PARAM);
    }
    /* X_TYPE_PARAM_BOXED */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamBoxed),
            param_boxed_init,
            X_TYPE_BOXED,
            param_boxed_final,
            param_boxed_default,
            param_boxed_validate,
            param_boxed_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamBoxed"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_BOXED);
    }
    /* X_TYPE_PARAM_PTR */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamPtr),
            param_ptr_init,
            X_TYPE_PTR,
            param_ptr_final,
            param_ptr_default,
            param_ptr_validate,
            param_ptr_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamPtr"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_PTR);
    }
    /* X_TYPE_PARAM_OBJECT */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamObject),
            param_object_init,
            X_TYPE_OBJECT,
            param_object_final,
            param_object_default,
            param_object_validate,
            param_object_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamObject"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_OBJECT);
    }
    /* X_TYPE_PARAM_OVERRIDE */
    {
        const xParamInfo pspec_info = {
            sizeof (xParamOverride),
            param_override_init,
            X_TYPE_VOID,
            param_override_final,
            param_override_default,
            param_override_validate,
            param_override_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamOverride"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_OVERRIDE);
    }
    /* X_TYPE_PARAM_XTYPE */
    {
        xParamInfo pspec_info = {
            sizeof (xParamXType),
            param_xtype_init,
            0xdeadbeef,		/* value_type, assigned further down */
            param_xtype_final,
            param_xtype_default,
            param_xtype_validate,
            param_xtype_cmp,
        };
        pspec_info.value_type = X_TYPE_XTYPE;
        type = x_param_register_static (x_intern_static_str ("xParamXType"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_XTYPE);
    }
    /* X_TYPE_PARAM_STRV */
    {
        xParamInfo pspec_info = {
            sizeof (xParamStrV),
            param_strv_init,
            X_TYPE_STRV,
            param_strv_final,
            param_strv_default,
            param_strv_validate,
            param_strv_cmp,
        };
        type = x_param_register_static (x_intern_static_str ("xParamStrV"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == X_TYPE_PARAM_STRV);
    }
}
