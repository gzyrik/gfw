#include "config.h"
#include "xvalue.h"
#include "xobject.h"
#include "xparam.h"
#include "xboxed.h"
#include "xvalist.h"
#include <string.h>
typedef struct {
    xType         src_type;
    xType         dst_type;
    xValueTransformFunc func;
} TransformEntry;

static xHashTable       *_transform_ht;
static xuint
transform_entry_hash    (xcptr          key)
{
    TransformEntry  *entry = (TransformEntry*)key;
    return entry->src_type + entry->dst_type;
}
static xint
transform_entry_cmp     (xcptr          key1,
                         xcptr          key2,
                         ...)
{
    TransformEntry  *entry1 = (TransformEntry*)key1;
    TransformEntry  *entry2 = (TransformEntry*)key2;
    if (entry1->src_type < entry2->src_type)
        return  -1;
    return  entry1->dst_type > entry2->dst_type;
}
static void
transform_entry_free    (xptr           self)
{
    x_slice_free (TransformEntry, self);
}
static xValueTransformFunc
lookup_transform_func   (xType          src_type,
                         xType          dest_type)
{
    TransformEntry  *result;
    TransformEntry  entry;
    entry.src_type = src_type;
    do {
        entry.dst_type = dest_type;
        do {
            result = x_hash_table_lookup (_transform_ht, &entry);
            if (result) {
                if (x_value_table_peek (entry.dst_type)
                    == x_value_table_peek (dest_type) &&
                    x_value_table_peek (entry.src_type) == x_value_table_peek (src_type))
                    return result->func;
            }
            entry.dst_type = x_type_parent (entry.dst_type);
        } while (entry.dst_type);

        entry.src_type = x_type_parent (entry.src_type);
    } while (entry.src_type);
    return NULL;
}

static inline void
value_meminit           (xValue         *val,
                         xType          type)
{
    val->type = type;
    memset (val->data, 0, sizeof (val->data));
}
xValue*
x_value_init            (xValue         *val,
                         xType          type)
{
    x_return_val_if_fail (val != NULL, NULL);
    if (X_VALUE_TYPE (val)) {
        x_warning ("%s: cannot initialize xValue with type `%s', "
                   "the value has already been initialized as `%s'",
                   X_STRLOC,
                   x_type_name (type),
                   x_type_name (X_VALUE_TYPE (val)));
    }
    else if (!X_TYPE_IS_VALUE (type)) {
        x_warning ("%s: cannot initialize xValue with type `%s', %s",
                   X_STRLOC,
                   x_type_name (type),
                   x_value_table_peek (type) ?
                   "this type is abstract with regards to xValue use, "
                   "use a more specific (derived) type" :
                   "this type has no xValueTable implementation");
    }
    else {/* (X_TYPE_IS_VALUE (type) && X_VALUE_TYPE (val) == 0) */
        xValueTable *value_table = x_value_table_peek (type);
        value_meminit (val, type);
        value_table->value_init (val);
    }
    return val;
}

xValue*
x_value_reset           (xValue         *val)
{
    xValueTable *value_table;
    xType type;

    x_return_val_if_fail (X_IS_VALUE (val), NULL);

    type = X_VALUE_TYPE (val);
    value_table = x_value_table_peek (type);

    if (value_table->value_clear)
        value_table->value_clear (val);

    /* setup and init */
    value_meminit (val, type);
    value_table->value_init (val);

    return val;
}
void
x_value_clear           (xValue         *val)
{
    xValueTable *value_table;

    x_return_if_fail (X_IS_VALUE (val));

    value_table = x_value_table_peek (X_VALUE_TYPE (val));

    if (value_table->value_clear)
        value_table->value_clear (val);
    memset (val, 0, sizeof (*val));
}
void
x_value_copy            (const xValue   *src,
                         xValue         *dest)
{
    x_return_if_fail (X_IS_VALUE (src));
    x_return_if_fail (X_IS_VALUE (dest));
    x_return_if_fail (x_value_type_compatible (X_VALUE_TYPE (src),
                                               X_VALUE_TYPE (dest)));

    if (src != dest) {
        xType dest_type = X_VALUE_TYPE (dest);
        xValueTable *value_table = x_value_table_peek (dest_type);

        if (value_table->value_clear)
            value_table->value_clear (dest);

        value_meminit (dest, dest_type);
        value_table->value_copy (src, dest);
    }
}
xbool
x_value_transform       (const xValue   *src,
                         xValue         *dst)
{
    xType dest_type;

    x_return_val_if_fail (X_IS_VALUE (src), FALSE);
    x_return_val_if_fail (X_IS_VALUE (dst), FALSE);

    dest_type = X_VALUE_TYPE (dst);
    if (x_value_type_compatible (X_VALUE_TYPE (src), dest_type)) {
        x_value_copy (src, dst);

        return TRUE;
    }
    else {
        xValueTransformFunc transform = lookup_transform_func (X_VALUE_TYPE (src), dest_type);

        if (transform) {
            x_value_clear (dst);
            value_meminit (dst, dest_type);
            transform (src, dst);
            return TRUE;
        }
    }
    return FALSE;

}
xbool
x_value_type_compatible (xType          src,
                         xType          dst)
{
    x_return_val_if_fail (X_TYPE_IS_VALUE (src), FALSE);
    x_return_val_if_fail (X_TYPE_IS_VALUE (dst), FALSE);

    return (x_type_is (src, dst) &&
            x_value_table_peek (dst) == x_value_table_peek (src));

}
xbool
x_value_type_transformable  (xType          src,
                             xType          dst)
{
    x_return_val_if_fail (X_TYPE_IS_VALUE (src), FALSE);
    x_return_val_if_fail (X_TYPE_IS_VALUE (dst), FALSE);

    return (x_value_type_compatible (src, dst) ||
            lookup_transform_func(src, dst) != NULL);
}
void
x_value_register_transform  (xType          src,
                             xType          dst,
                             xValueTransformFunc transform)
{
    TransformEntry  *entry;
    x_return_if_fail (transform != NULL);
    entry = x_slice_new(TransformEntry);
    entry->src_type = src;
    entry->dst_type = dst;
    entry->func = transform;
    x_hash_table_add (_transform_ht,entry);
}
xstr
x_strdup_value_contents (const xValue   *val)
{
    return NULL;
}

xbool
x_value_get_bool        (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_BOOL (val), 0);

    return val->data[0].v_int;
}    
void
x_value_set_bool        (xValue         *val,
                         xbool          b)
{
    x_return_if_fail (X_VALUE_HOLDS_BOOL (val));

    val->data[0].v_int = (b != FALSE);
}  
xchar
x_value_get_char        (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_CHAR (val), 0);

    return val->data[0].v_int;
}
void
x_value_set_char        (xValue         *val,
                         xchar          c)
{
    x_return_if_fail (X_VALUE_HOLDS_CHAR (val));

    val->data[0].v_int = c;
}
xuchar
x_value_get_uchar       (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_UCHAR (val), 0);

    return val->data[0].v_int;
}
void
x_value_set_uchar       (xValue         *val,
                         xuchar         c)
{
    x_return_if_fail (X_VALUE_HOLDS_UCHAR (val));

    val->data[0].v_int = c;
}
xint
x_value_get_int         (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_INT(val), 0);

    return val->data[0].v_int;
}
void
x_value_set_int         (xValue         *val,
                         xint           i)
{
    x_return_if_fail (X_VALUE_HOLDS_INT (val));

    val->data[0].v_int = i;
}
xuint
x_value_get_uint        (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_UINT(val), 0);

    return val->data[0].v_uint;
}
void
x_value_set_uint        (xValue         *val,
                         xuint          i)
{
    x_return_if_fail (X_VALUE_HOLDS_UINT (val));

    val->data[0].v_uint = i;
}
xlong
x_value_get_long        (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_LONG(val), 0);

    return val->data[0].v_long;
}
void
x_value_set_long        (xValue         *val,
                         xlong          l)
{
    x_return_if_fail (X_VALUE_HOLDS_LONG(val));

    val->data[0].v_long = l;
}
xulong
x_value_get_ulong       (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_ULONG(val), 0);

    return val->data[0].v_ulong;
}
void
x_value_set_ulong       (xValue         *val,
                         xulong         l)
{
    x_return_if_fail (X_VALUE_HOLDS_ULONG(val));

    val->data[0].v_ulong = l;
}
xfloat
x_value_get_float       (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_FLOAT(val), 0);

    return val->data[0].v_float;
}
void
x_value_set_float       (xValue         *val,
                         xfloat         f)
{
    x_return_if_fail (X_VALUE_HOLDS_FLOAT(val));

    val->data[0].v_float = f;
}
xdouble
x_value_get_double      (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_DOUBLE(val), 0);

    return val->data[0].v_double;
}
void
x_value_set_double      (xValue         *val,
                         xdouble        d)
{
    x_return_if_fail (X_VALUE_HOLDS_DOUBLE(val));

    val->data[0].v_double = d;
}
xint64
x_value_get_int64       (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_INT64(val), 0);

    return val->data[0].v_int64;
}
void
x_value_set_int64       (xValue         *val,
                         xint64         i)
{
    x_return_if_fail (X_VALUE_HOLDS_INT64(val));

    val->data[0].v_int64 = i;
}
xuint64
x_value_get_uint64      (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_UINT64(val), 0);

    return val->data[0].v_uint64;
}
void
x_value_set_uint64      (xValue         *val,
                         xuint64         i)
{
    x_return_if_fail (X_VALUE_HOLDS_UINT64(val));

    val->data[0].v_uint64 = i;
}
xint
x_value_get_enum        (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_ENUM (val), 0);

    return val->data[0].v_long;
}
void
x_value_set_enum        (xValue         *val,
                         xint           e)
{
    x_return_if_fail (X_VALUE_HOLDS_ENUM (val));

    val->data[0].v_ulong = e;
}  
xuint
x_value_get_flags       (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_FLAGS (val), 0);

    return val->data[0].v_ulong;
}    
void
x_value_set_flags       (xValue         *val,
                         xuint           f)
{
    x_return_if_fail (X_VALUE_HOLDS_FLAGS (val));

    val->data[0].v_ulong = f;
}
xptr
x_value_get_ptr         (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_PTR (val), NULL);

    return val->data[0].v_pointer;
}
xbool
x_value_fits_ptr        (const xValue   *val)
{
    xValueTable *value_table;

    x_return_val_if_fail (X_IS_VALUE (val), FALSE);

    value_table = x_value_table_peek (X_VALUE_TYPE (val));

    return value_table->value_peek != NULL;
}

xptr
x_value_peek_ptr        (const xValue   *val)
{
    xValueTable *value_table;

    x_return_val_if_fail (X_IS_VALUE (val), NULL);

    value_table = x_value_table_peek (X_VALUE_TYPE (val));
    if (!value_table->value_peek) {
        x_return_val_if_fail (x_value_fits_ptr (val) == TRUE, NULL);
        return NULL;
    }

    return value_table->value_peek (val);
}
void
x_value_set_ptr         (xValue         *val,
                         xptr           p)
{
    x_return_if_fail (X_VALUE_HOLDS_PTR (val));

    val->data[0].v_pointer = p;
}
void
x_value_set_str         (xValue         *val,
                         xcstr          str)
{
    xstr   new_val;

    x_return_if_fail (X_VALUE_HOLDS_STR (val));

    new_val = x_strdup (str);

    if (val->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)
        val->data[1].v_uint = 0;
    else
        x_free (val->data[0].v_pointer);

    val->data[0].v_pointer = new_val;
}
void
x_value_set_static_str  (xValue         *val,
                         xcstr          str)
{
    x_return_if_fail (X_VALUE_HOLDS_STR (val));

    if (!(val->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS))
        x_free (val->data[0].v_pointer);
    val->data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;
    val->data[0].v_pointer = (xstr) str;
}
xcstr
x_value_get_str         (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_STR (val), NULL);

    return val->data[0].v_pointer;
}
xstr
x_value_dup_str         (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_STR (val), NULL);

    return x_strdup (val->data[0].v_pointer);
}

void
x_value_take_str        (xValue         *val,
                         xstr           str)
{
    x_return_if_fail (X_VALUE_HOLDS_STR (val));

    if (val->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS)
        val->data[1].v_uint = 0;
    else
        x_free (val->data[0].v_pointer);
    val->data[0].v_pointer = str;
}
static inline void
value_set_boxed         (xValue         *val,
                         xcptr          boxed,
                         xbool          need_copy,
                         xbool          need_free)
{
    if (!boxed) {
        x_value_reset (val);
        return;
    }

    if (val->data[0].v_pointer && !(val->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS))
        x_boxed_free (X_VALUE_TYPE (val), val->data[0].v_pointer);
    val->data[1].v_uint = need_free ? 0 : X_VALUE_NOCOPY_CONTENTS;
    val->data[0].v_pointer = need_copy ? x_boxed_copy (X_VALUE_TYPE (val), boxed) : (xptr) boxed;
}

void
x_value_set_boxed       (xValue         *val,
                         xcptr          boxed)
{
    x_return_if_fail (X_VALUE_HOLDS_BOXED (val));
    x_return_if_fail (X_IS_VALUE (val));

    value_set_boxed (val, boxed, TRUE, TRUE);
}
void
x_value_set_static_boxed(xValue         *val,
                         xcptr          boxed)
{
    x_return_if_fail (X_VALUE_HOLDS_BOXED (val));
    x_return_if_fail (X_IS_VALUE (val));

    value_set_boxed (val, boxed, FALSE, FALSE);
}
void
x_value_take_boxed      (xValue         *val,
                         xcptr          boxed)
{
    x_return_if_fail (X_VALUE_HOLDS_BOXED (val));
    x_return_if_fail (X_IS_VALUE (val));

    value_set_boxed (val, boxed, FALSE, TRUE);
}
xcptr
x_value_get_boxed       (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_BOXED (val), NULL);
    x_return_val_if_fail (X_IS_VALUE (val), NULL);
    return val->data[0].v_pointer;
}

xptr
x_value_dup_boxed       (const xValue   *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_BOXED (val), NULL);
    x_return_val_if_fail (X_IS_VALUE (val), NULL);
    return val->data[0].v_pointer ?
        x_boxed_copy (X_VALUE_TYPE (val), val->data[0].v_pointer) : NULL;
}

xptr
x_value_get_param       (xValue         *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_PARAM (val), NULL);

    return val->data[0].v_pointer;
}  
void
x_value_set_param       (xValue         *val,
                         xParam         *param)
{
    x_value_take_param (val, x_param_ref (param));
}
void
x_value_take_param      (xValue         *val,
                         xParam         *param)
{
    x_return_if_fail (X_VALUE_HOLDS_PARAM (val));
    if (param)
        x_return_if_fail (X_IS_PARAM (param));

    if (val->data[0].v_pointer)
        x_param_unref (val->data[0].v_pointer);
    val->data[0].v_pointer = param;
}
xptr
x_value_get_object      (xValue         *val)
{
    x_return_val_if_fail (X_VALUE_HOLDS_OBJECT (val), NULL);

    return val->data[0].v_pointer;
}
void
x_value_set_object      (xValue         *val,
                         xptr           object)
{
    x_value_take_object (val, x_object_ref (object));
}
void
x_value_take_object     (xValue         *val,
                         xObject        *object)
{
    x_return_if_fail (X_VALUE_HOLDS_OBJECT (val));

    if (val->data[0].v_pointer) {
        x_object_unref (val->data[0].v_pointer);
        val->data[0].v_pointer = NULL;
    }

    if (object) {
        x_return_if_fail (X_IS_OBJECT (object));
        x_return_if_fail (x_value_type_compatible (X_INSTANCE_TYPE (object),
                                                   X_VALUE_TYPE (val)));

        val->data[0].v_pointer = object; 
    }
}
void
x_value_set_instance    (xValue         *val,
                         xptr           instance)
{
    xType       type;
    xValueTable *value_table;
    xCValue     cvalue;
    xstr        error_msg;

    x_return_if_fail (X_IS_VALUE (val));
    if (instance) {
        x_return_if_fail (X_INSTANCE_CHECK (instance));
        x_return_if_fail (x_value_type_compatible (X_INSTANCE_TYPE (instance),
                                                   X_VALUE_TYPE (val)));
    }

    type = X_VALUE_TYPE (val);
    value_table = x_value_table_peek (type);

    x_return_if_fail (strcmp (value_table->collect_format,
                              X_VALUE_COLLECT_PTR) == 0);

    memset (&cvalue, 0, sizeof (cvalue));
    cvalue.v_pointer = instance;

    /* make sure value's value is free()d */
    if (value_table->value_clear)
        value_table->value_clear (val);

    /* setup and collect */
    value_meminit (val, type);
    error_msg = value_table->value_collect (val, 1, &cvalue, 0);
    if (error_msg) {
        x_warning ("%s: %s", X_STRLOC, error_msg);
        x_free (error_msg);

        /* we purposely leak the value here, it might not be
         * in a sane state if an error condition occoured
         */
        value_meminit (val, type);
        value_table->value_init (val);
    }
}
/* X_TYPE_CHAR / X_TYPE_UCHAR */
#define value_init_char  value_init_long
#define value_copy_char  value_copy_long
#define value_collect_char value_collect_int
static xstr
value_lcopy_char        (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xchar *int8_p = collect_values[0].v_pointer;

    if (!int8_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *int8_p = val->data[0].v_int;

    return NULL;
}
/* X_TYPE_BOOL */
#define value_init_bool  value_init_long
#define value_copy_bool  value_copy_long
#define value_collect_bool value_collect_int
static xstr
value_lcopy_bool        (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xbool *bool_p = collect_values[0].v_pointer;

    if (!bool_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *bool_p = val->data[0].v_int;

    return NULL;
}
/* X_TYPE_INT / X_TYPE_UINT */
#define value_init_int  value_init_long
#define value_copy_int  value_copy_long
static xstr
value_collect_int       (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    val->data[0].v_int = collect_values[0].v_int;

    return NULL;
}
static xstr
value_lcopy_int         (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xint *int_p = collect_values[0].v_pointer;

    if (!int_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *int_p = val->data[0].v_int;

    return NULL;
}
/* X_TYPE_LONG / X_TYPE_ULONG */
static void
value_init_long         (xValue         *val)
{
    val->data[0].v_long = 0;
}
static void
value_copy_long         (const xValue   *src,
                         xValue         *dest)
{
    dest->data[0].v_long = src->data[0].v_long;
}
static xstr
value_collect_long      (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    val->data[0].v_long = collect_values[0].v_long;

    return NULL;
}
static xstr
value_lcopy_long        (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xlong *long_p = collect_values[0].v_pointer;

    if (!long_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *long_p = val->data[0].v_long;

    return NULL;
}
/* X_TYPE_INT64 / X_TYPE_UINT64 */
static void
value_init_int64        (xValue         *val)
{
    val->data[0].v_int64 = 0;
}
static void
value_copy_int64        (const xValue   *src,
                         xValue         *dest)
{
    dest->data[0].v_int64 = src->data[0].v_int64;
}
static xstr
value_collect_int64     (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    val->data[0].v_int64 = collect_values[0].v_int64;

    return NULL;
}
static xstr
value_lcopy_int64       (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xint64 *int64_p = collect_values[0].v_pointer;

    if (!int64_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *int64_p = val->data[0].v_int64;

    return NULL;
}
/* X_TYPE_FLOAT */
static void
value_init_float        (xValue         *val)
{
    val->data[0].v_float = 0;
}
static void
value_copy_float        (const xValue   *src,
                         xValue         *dest)
{
    dest->data[0].v_float = src->data[0].v_float;
}
static xstr
value_collect_float     (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    val->data[0].v_float = (xfloat)collect_values[0].v_double;

    return NULL;
}
static xstr
value_lcopy_float       (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xfloat *float_p = collect_values[0].v_pointer;

    if (!float_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *float_p = val->data[0].v_float;

    return NULL;
}
/* X_TYPE_DOUBLE */
static void
value_init_double       (xValue         *val)
{
    val->data[0].v_double = 0;
}
static void
value_copy_double       (const xValue   *src,
                         xValue         *dest)
{
    dest->data[0].v_double = src->data[0].v_double;
}
static xstr
value_collect_double    (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    val->data[0].v_double = collect_values[0].v_double;

    return NULL;
}
static xstr
value_lcopy_double      (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xdouble *double_p = collect_values[0].v_pointer;

    if (!double_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *double_p = val->data[0].v_double;

    return NULL;
}
/* X_TYPE_STR */
static void
value_init_str          (xValue         *val)
{
    val->data[0].v_pointer = NULL;
}
static void
value_free_str          (xValue         *val)
{
    if (!(val->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS))
        x_free (val->data[0].v_pointer);
}
static void
value_copy_str          (const xValue   *src,
                         xValue         *dest)
{
    dest->data[0].v_pointer = x_strdup (src->data[0].v_pointer);
}
static xstr
value_collect_str       (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    if (!collect_values[0].v_pointer)
        val->data[0].v_pointer = NULL;
    else if (collect_flags & X_VALUE_NOCOPY_CONTENTS) {
        val->data[0].v_pointer = collect_values[0].v_pointer;
        val->data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;
    }
    else
        val->data[0].v_pointer = x_strdup (collect_values[0].v_pointer);

    return NULL;
}
static xstr
value_lcopy_str         (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xstr *str_p = collect_values[0].v_pointer;

    if (!str_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    if (!val->data[0].v_pointer)
        *str_p = NULL;
    else if (collect_flags & X_VALUE_NOCOPY_CONTENTS)
        *str_p = val->data[0].v_pointer;
    else
        *str_p = x_strdup (val->data[0].v_pointer);

    return NULL;
}
/* X_TYPE_PTR */
static void
value_init_ptr          (xValue         *val)
{
    val->data[0].v_pointer = NULL;
}
static void
value_copy_ptr          (const xValue   *src,
                         xValue         *dest)
{
    dest->data[0].v_pointer = src->data[0].v_pointer;
}
#define value_peek_str  value_peek_ptr
static xptr
value_peek_ptr          (const xValue   *val)
{
    return val->data[0].v_pointer;
}
static xstr
value_collect_ptr       (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    val->data[0].v_pointer = collect_values[0].v_pointer;

    return NULL;
}
static xstr
value_lcopy_ptr         (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xptr *ptr_p = collect_values[0].v_pointer;

    if (!ptr_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (val));

    *ptr_p = val->data[0].v_pointer;

    return NULL;
}
void
_x_value_types_init     (void)
{
    xType type;
    xTypeInfo info = {0,};
    xFundamentalInfo finfo = {X_TYPE_DERIVABLE, };
    _transform_ht = x_hash_table_new_full(transform_entry_hash,
                                          transform_entry_cmp,
                                          transform_entry_free,
                                          NULL);
    /* X_TYPE_CHAR / X_TYPE_UCHAR */
    {
        const xValueTable value_table = {
            value_init_char,
            NULL,
            value_copy_char,
            NULL,
            X_VALUE_COLLECT_INT,
            value_collect_char,
            X_VALUE_COLLECT_PTR,
            value_lcopy_char,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_CHAR,
                                            x_intern_static_str ("xchar"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_CHAR == type);

        type = x_type_register_fundamental (X_TYPE_UCHAR,
                                            x_intern_static_str ("xuchar"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_UCHAR == type);
    }
    /* X_TYPE_BOOL */
    {
        const xValueTable value_table = {
            value_init_bool,
            NULL,
            value_copy_bool,
            NULL,
            X_VALUE_COLLECT_INT,
            value_collect_bool,
            X_VALUE_COLLECT_PTR,
            value_lcopy_bool,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_BOOL,
                                            x_intern_static_str ("xbool"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_BOOL == type);
    }
    /* X_TYPE_INT / X_TYPE_UINT */
    {
        const xValueTable value_table = {
            value_init_int,
            NULL,
            value_copy_int,
            NULL,
            X_VALUE_COLLECT_INT,
            value_collect_int,
            X_VALUE_COLLECT_PTR,
            value_lcopy_int,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_INT,
                                            x_intern_static_str ("xint"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_INT == type);
        type = x_type_register_fundamental (X_TYPE_UINT,
                                            x_intern_static_str ("xuint"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_UINT == type);
    }
    /* X_TYPE_LONG / X_TYPE_ULONG */
    {
        const xValueTable value_table = {
            value_init_long,
            NULL,
            value_copy_long,
            NULL,
            X_VALUE_COLLECT_LONG,
            value_collect_long,
            X_VALUE_COLLECT_PTR,
            value_lcopy_long,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_LONG,
                                            x_intern_static_str ("xlong"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_LONG == type);
        type = x_type_register_fundamental (X_TYPE_ULONG,
                                            x_intern_static_str ("xulong"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_ULONG == type);
    }
    /* X_TYPE_INT64 / X_TYPE_UINT64 */
    {
        const xValueTable value_table = {
            value_init_int64,
            NULL,
            value_copy_int64,
            NULL,
            X_VALUE_COLLECT_INT64,
            value_collect_int64,
            X_VALUE_COLLECT_PTR,	
            value_lcopy_int64,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_INT64,
                                            x_intern_static_str ("xint64"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_INT64 == type);
        type = x_type_register_fundamental (X_TYPE_UINT64,
                                            x_intern_static_str ("xuint64"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_UINT64 == type);
    }
    /* X_TYPE_FLOAT */
    {
        const xValueTable value_table = {
            value_init_float,
            NULL,
            value_copy_float,
            NULL,
            X_VALUE_COLLECT_DOUBLE,
            value_collect_float,
            X_VALUE_COLLECT_PTR,
            value_lcopy_float,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_FLOAT,
                                            x_intern_static_str ("xfloat"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_FLOAT == type);
    }

    /* X_TYPE_DOUBLE */
    {
        const xValueTable value_table = {
            value_init_double,
            NULL,
            value_copy_double,
            NULL,
            X_VALUE_COLLECT_DOUBLE,
            value_collect_double,
            X_VALUE_COLLECT_PTR,
            value_lcopy_double,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_DOUBLE,
                                            x_intern_static_str ("xdouble"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_DOUBLE == type);
    }

    /* X_TYPE_STR */
    {
        const xValueTable value_table = {
            value_init_str,
            value_free_str,
            value_copy_str,
            value_peek_str,
            X_VALUE_COLLECT_PTR,
            value_collect_str,
            X_VALUE_COLLECT_PTR,
            value_lcopy_str,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_STR,
                                            x_intern_static_str ("xstr"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_STR == type);
    }
    /* X_TYPE_PTR */
    {
        const xValueTable value_table = {
            value_init_ptr,
            NULL,
            value_copy_ptr,
            value_peek_ptr,
            X_VALUE_COLLECT_PTR,
            value_collect_ptr,
            X_VALUE_COLLECT_PTR,
            value_lcopy_ptr,
        };
        info.value_table = &value_table;
        type = x_type_register_fundamental (X_TYPE_PTR,
                                            x_intern_static_str ("xptr"),
                                            &info, &finfo, 0);
        x_assert (X_TYPE_PTR == type);
    }
}
