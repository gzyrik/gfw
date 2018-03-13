#ifndef __X_VALIST_H__
#define __X_VALIST_H__
#include "xtype.h"
X_BEGIN_DECLS
#define X_VALUE_COLLECT_MAX_LENGTH      (8)
#define X_VALUE_COLLECT_INT             "i"
#define X_VALUE_COLLECT_LONG            "l"
#define X_VALUE_COLLECT_INT64           "q"
#define X_VALUE_COLLECT_DOUBLE          "d"
#define X_VALUE_COLLECT_PTR             "p"

#define X_VALIST_INIT(value, _value_type, var_args, flags, __error)		\
    G_STMT_START {										\
    GValue *_val = (value);								\
    guint _flags = (flags);								\
    GTypeValueTable *_vtab = g_type_value_table_peek (_value_type);			\
    const gchar *_collect_format = _vtab->collect_format;					\
    GTypeCValue _cvalues[G_VALUE_COLLECT_FORMAT_MAX_LENGTH] = { { 0, }, };		\
    guint _n_values = 0;									\
    \
    _val->g_type = _value_type;		/* value_meminit() from gvalue.c */		\
    while (*_collect_format)								\
{											\
    GTypeCValue *_cvalue = _cvalues + _n_values++;					\
    \
    switch (*_collect_format++)							\
{										\
    case G_VALUE_COLLECT_INT:							\
    _cvalue->v_int = va_arg ((var_args), gint);					\
    break;									\
    case G_VALUE_COLLECT_LONG:							\
    _cvalue->v_long = va_arg ((var_args), glong);					\
    break;									\
    case G_VALUE_COLLECT_INT64:							\
    _cvalue->v_int64 = va_arg ((var_args), gint64);				\
    break;									\
    case G_VALUE_COLLECT_DOUBLE:							\
    _cvalue->v_double = va_arg ((var_args), gdouble);				\
    break;									\
    case G_VALUE_COLLECT_POINTER:							\
    _cvalue->v_pointer = va_arg ((var_args), gpointer);				\
    break;									\
    default:									\
    g_assert_not_reached ();							\
}										\
}											\
    *(__error) = _vtab->collect_value (_val,						\
    _n_values,					\
    _cvalues,					\
    _flags);						\
} G_STMT_END


#define G_VALUE_COLLECT(value, var_args, flags, __error) G_STMT_START {			\
    GValue *_value = (value);								\
    GType _value_type = G_VALUE_TYPE (_value);						\
    GTypeValueTable *_vtable = g_type_value_table_peek (_value_type);			\
    \
    if (_vtable->value_free)								\
    _vtable->value_free (_value);							\
    memset (_value->data, 0, sizeof (_value->data));					\
    \
    G_VALUE_COLLECT_INIT(value, _value_type, var_args, flags, __error);			\
} G_STMT_END
}

#define G_VALUE_COLLECT_SKIP(_value_type, var_args)					\
    G_STMT_START {										\
    GTypeValueTable *_vtable = g_type_value_table_peek (_value_type);			\
    const gchar *_collect_format = _vtable->collect_format;				\
    \
    while (*_collect_format)								\
    {											\
    switch (*_collect_format++)							\
    {										\
    case G_VALUE_COLLECT_INT:							\
    va_arg ((var_args), gint);							\
    break;									\
    case G_VALUE_COLLECT_LONG:							\
    va_arg ((var_args), glong);							\
    break;									\
    case G_VALUE_COLLECT_INT64:							\
    va_arg ((var_args), gint64);							\
    break;									\
    case G_VALUE_COLLECT_DOUBLE:							\
    va_arg ((var_args), gdouble);							\
    break;									\
    case G_VALUE_COLLECT_POINTER:							\
    va_arg ((var_args), gpointer);						\
    break;									\
    default:									\
    g_assert_not_reached ();							\
}										\
}											\
} G_STMT_END

#define G_VALUE_LCOPY(value, var_args, flags, __error)					\
    G_STMT_START {										\
    const GValue *_value = (value);							\
    guint _flags = (flags);								\
    GType _value_type = G_VALUE_TYPE (_value);						\
    GTypeValueTable *_vtable = g_type_value_table_peek (_value_type);			\
    const gchar *_lcopy_format = _vtable->lcopy_format;					\
    GTypeCValue _cvalues[G_VALUE_COLLECT_FORMAT_MAX_LENGTH] = { { 0, }, };		\
    guint _n_values = 0;									\
    \
    while (*_lcopy_format)								\
    {											\
    GTypeCValue *_cvalue = _cvalues + _n_values++;					\
    \
    switch (*_lcopy_format++)								\
    {										\
    case G_VALUE_COLLECT_INT:							\
    _cvalue->v_int = va_arg ((var_args), gint);					\
    break;									\
    case G_VALUE_COLLECT_LONG:							\
    _cvalue->v_long = va_arg ((var_args), glong);					\
    break;									\
    case G_VALUE_COLLECT_INT64:							\
    _cvalue->v_int64 = va_arg ((var_args), gint64);				\
    break;									\
    case G_VALUE_COLLECT_DOUBLE:							\
    _cvalue->v_double = va_arg ((var_args), gdouble);				\
    break;									\
    case G_VALUE_COLLECT_POINTER:							\
    _cvalue->v_pointer = va_arg ((var_args), gpointer);				\
    break;									\
    default:									\
    g_assert_not_reached ();							\
}										\
}											\
    *(__error) = _vtable->lcopy_value (_value,						\
    _n_values,						\
    _cvalues,						\
    _flags);						\
} G_STMT_END


X_END_DECLS
#endif /* __X_VALIST_H__ */
