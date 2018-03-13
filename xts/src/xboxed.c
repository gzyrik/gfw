#include "config.h"
#include "xboxed.h"
#include "xclass.h"
#include "xclass-prv.h"
#include "xvalue.h"
#include "xvalist.h"
#include <string.h>
static inline void
value_meminit           (xValue         *value,
                         xType          value_type)
{
    value->type = value_type;
    memset (value->data, 0, sizeof (value->data));
}

static xValue*
value_copy              (xValue         *src)
{
    xValue *dest = x_new0 (xValue, 1);

    if (X_VALUE_TYPE (src)) {
        x_value_init (dest, X_VALUE_TYPE (src));
        x_value_copy (src, dest);
    }
    return dest;
}

static void
value_free              (xValue         *value)
{
    if (X_VALUE_TYPE (value))
        x_value_clear (value);
    x_free (value);
}

X_DEFINE_BOXED_TYPE (xValue, x_value, value_copy, value_free);
X_DEFINE_BOXED_TYPE (xstrv, x_strv, x_strv_dup, x_strv_free);
X_DEFINE_PTR_TYPE (xType,x_type);
static void
boxed_value_init        (xValue         *value)
{
    value->data[0].v_pointer = NULL;
}

static void
boxed_value_clear       (xValue         *value)
{
    if (value->data[0].v_pointer &&
        !(value->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS))
        _x_type_boxed_free (X_VALUE_TYPE (value),
                            value->data[0].v_pointer);
}

static void
boxed_value_copy        (const xValue   *src,
                         xValue         *dest)
{
    if (src->data[0].v_pointer)
        dest->data[0].v_pointer =
            _x_type_boxed_copy (X_VALUE_TYPE (src),
                                src->data[0].v_pointer);
    else
        dest->data[0].v_pointer = NULL;
}

static xptr
boxed_value_peek        (const xValue   *value)
{
    return value->data[0].v_pointer;
}

static xstr
boxed_value_collect     (xValue         *value,
                         xsize          n_collect,
                         xCValue        *collect,
                         xuint          flags)
{
    if (!collect[0].v_pointer)
        value->data[0].v_pointer = NULL;
    else
    {
        if (flags & X_VALUE_NOCOPY_CONTENTS)
        {
            value->data[0].v_pointer = collect[0].v_pointer;
            value->data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;
        }
        else
            value->data[0].v_pointer = 
                _x_type_boxed_copy (X_VALUE_TYPE (value),
                                    collect[0].v_pointer);
    }

    return NULL;
}

static xstr
boxed_value_lcopy       (const xValue   *value,
                         xsize          n_collect,
                         xCValue        *collect,
                         xuint          flags)
{
    xptr *boxed_p = collect[0].v_pointer;

    if (!boxed_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (value));

    if (!value->data[0].v_pointer)
        *boxed_p = NULL;
    else if (flags & X_VALUE_NOCOPY_CONTENTS)
        *boxed_p = value->data[0].v_pointer;
    else
        *boxed_p = _x_type_boxed_copy (X_VALUE_TYPE (value),
                                       value->data[0].v_pointer);

    return NULL;
}

xType
x_boxed_register_static (xcstr          name,
                         xDupFunc       boxed_copy,
                         xFreeFunc      boxed_free)
{
    const xValueTable vtable = {
        boxed_value_init,
        boxed_value_clear,
        boxed_value_copy,
        boxed_value_peek,
        X_VALUE_COLLECT_PTR,
        boxed_value_collect,
        X_VALUE_COLLECT_PTR,
        boxed_value_lcopy,
    };
    const xTypeInfo   type_info = {
        0,NULL,NULL,
        NULL,NULL,NULL,
        0,NULL,&vtable
    };
    xType       type;

    x_return_val_if_fail (name != NULL, 0);
    x_return_val_if_fail (boxed_copy != NULL, 0);
    x_return_val_if_fail (boxed_free != NULL, 0);
    x_return_val_if_fail (x_type_from_name (name) == 0, 0);

    type = x_type_register_static (name, X_TYPE_BOXED, &type_info, 0);

    if (type)
        _x_type_boxed_init (type, boxed_copy, boxed_free);

    return type;
}
xptr
x_boxed_copy            (xType          boxed_type,
                         xcptr          src_boxed)
{
    xValueTable *value_table;
    xptr        dest_boxed;

    x_return_val_if_fail (X_TYPE_IS_BOXED (boxed_type), NULL);
    x_return_val_if_fail (X_TYPE_IS_ABSTRACT (boxed_type) == FALSE, NULL);
    x_return_val_if_fail (src_boxed != NULL, NULL);

    value_table = x_value_table_peek (boxed_type);
    if (!value_table)
        x_return_val_if_fail (X_TYPE_IS_VALUE (boxed_type), NULL);

    if (value_table->value_copy == boxed_value_copy)
        dest_boxed = _x_type_boxed_copy (boxed_type, (xptr) src_boxed);
    else {
        xValue src_value, dest_value;
        value_meminit (&src_value, boxed_type);
        src_value.data[0].v_pointer = (xptr) src_boxed;
        src_value.data[1].v_uint = X_VALUE_NOCOPY_CONTENTS;

        value_meminit (&dest_value, boxed_type);
        value_table->value_copy (&src_value, &dest_value);

        if (dest_value.data[1].v_ulong)
            x_warning ("the copy_value() implementation of type `%s' "
                       "seems to make use of reserved GValue fields",
                       x_type_name (boxed_type));
        dest_boxed = dest_value.data[0].v_pointer;
    }

    return dest_boxed;
}  

void
x_boxed_free            (xType          boxed_type,
                         xptr           boxed)
{
    xValueTable *value_table;

    x_return_if_fail (X_TYPE_IS_BOXED (boxed_type));
    x_return_if_fail (X_TYPE_IS_ABSTRACT (boxed_type) == FALSE);
    x_return_if_fail (boxed != NULL);

    value_table = x_value_table_peek (boxed_type);
    if (!value_table)
        x_return_if_fail (X_TYPE_IS_VALUE (boxed_type));

    /* check if our proxying implementation is used, we can short-cut here */
    if (value_table->value_clear == boxed_value_clear)
        _x_type_boxed_free (boxed_type, boxed);
    else
    {
        xValue value;

        /* see g_boxed_copy() on why we think we can do this */
        value_meminit (&value, boxed_type);
        value.data[0].v_pointer = boxed;
        value_table->value_clear (&value);
    }
} 
xType
x_ptr_register_static   (xcstr          name)
{
    const xTypeInfo type_info = {
        0,			/* class_size */
        NULL,		/* base_init */
        NULL,		/* base_finalize */
        NULL,		/* class_init */
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        0,			/* instance_size */
        NULL,		/* instance_init */
        NULL		/* value_table */
    };
    xType type;

    x_return_val_if_fail (name != NULL, 0);
    x_return_val_if_fail (x_type_from_name (name) == 0, 0);

    type = x_type_register_static (name, X_TYPE_PTR,&type_info, 0);

    return type;
}
xEnumValue*
x_enum_get_value        (xEnumClass     *enum_class,
                         xint	      value)
{
    x_return_val_if_fail (X_IS_ENUM_CLASS (enum_class), NULL);

    if (enum_class->n_values) {
        xEnumValue *enum_value;

        for (enum_value = enum_class->values;
             enum_value->value_name; enum_value++)
            if (enum_value->value == value)
                return enum_value;
    }

    return NULL;
}

static void
value_init_flags_enum   (xValue         *value)
{
    value->data[0].v_long = 0;
}

static void
value_copy_flags_enum   (const xValue   *src_value,
                         xValue	        *dest_value)
{
    dest_value->data[0].v_long = src_value->data[0].v_long;
}

static xstr
value_collect_flags_enum(xValue         *value,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    value->data[0].v_long = collect_values[0].v_int;

    return NULL;
}

static xstr
value_lcopy_flags_enum  (const xValue   *value,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint         collect_flags)
{
    xint *int_p = collect_values[0].v_pointer;

    if (!int_p)
        return x_strdup_printf ("value location for `%s' passed as NULL",
                                X_VALUE_TYPE_NAME (value));

    *int_p = value->data[0].v_long;
    return NULL;
}
static void
x_enum_class_init       (xEnumClass     *klass,
                         xptr           class_data)
{
    x_return_if_fail (X_IS_ENUM_CLASS (klass));

    klass->minimum = 0;
    klass->maximum = 0;
    klass->n_values = 0;
    klass->values = class_data;

    if (klass->values) {
        xEnumValue *values;

        klass->minimum = klass->values->value;
        klass->maximum = klass->values->value;
        for (values = klass->values; values->value_name; values++) {
            klass->minimum = MIN (klass->minimum, values->value);
            klass->maximum = MAX (klass->maximum, values->value);
            klass->n_values++;
        }
    }
}
xType
x_enum_register_static  (xcstr	        name,
                         xEnumValue     *const_static_values)
{
    xTypeInfo enum_type_info = {
        sizeof (xEnumClass), /* class_size */
        NULL,                /* base_init */
        NULL,                /* base_finalize */
        (xClassInitFunc) x_enum_class_init,
        NULL,                /* class_finalize */
        NULL,                /* class_data */
        0,                   /* instance_size */
        NULL,                /* instance_init */
        NULL,		         /* value_table */
    };
    xType type;

    x_return_val_if_fail (name != NULL, 0);
    x_return_val_if_fail (const_static_values != NULL, 0);

    enum_type_info.class_data = const_static_values;

    type = x_type_register_static (name, X_TYPE_ENUM, &enum_type_info, 0);

    return type;
}
void
_x_boxed_types_init     (void)
{
    xType type;
    {
        const xTypeInfo info = {0,};
        const xFundamentalInfo finfo = { X_TYPE_DERIVABLE, };
        type = x_type_register_fundamental (X_TYPE_BOXED,
                                            x_intern_static_str ("xBoxed"),
                                            &info, &finfo,
                                            X_TYPE_ABSTRACT |
                                            X_TYPE_VALUE_ABSTRACT);
        x_assert (type == X_TYPE_BOXED);
    }
    {
        const xValueTable value_table = {
            value_init_flags_enum,
            NULL,
            value_copy_flags_enum,
            NULL,
            X_VALUE_COLLECT_INT,
            value_collect_flags_enum,
            X_VALUE_COLLECT_PTR,
            value_lcopy_flags_enum,
        };
        xTypeInfo info = {
            0,  
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            0,   
            NULL,
            &value_table, 
        };
        const xFundamentalInfo finfo = {
            X_TYPE_CLASSED | X_TYPE_DERIVABLE,
        };
        /* X_TYPE_ENUM */
        info.class_size = sizeof (xEnumClass);
        type = x_type_register_fundamental (X_TYPE_ENUM,
                                            x_intern_static_str("xEnum"),
                                            &info, &finfo,
                                            X_TYPE_ABSTRACT |
                                            X_TYPE_VALUE_ABSTRACT);
        x_assert (type == X_TYPE_ENUM);

        /* G_TYPE_FLAGS */
        info.class_size = sizeof (xFlagsClass);
        type = x_type_register_fundamental (X_TYPE_FLAGS,
                                            x_intern_static_str ("xFlags"),
                                            &info, &finfo,
                                            X_TYPE_ABSTRACT |
                                            X_TYPE_VALUE_ABSTRACT);
        x_assert (type == X_TYPE_FLAGS);

    }
}
