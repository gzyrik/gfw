#ifndef __X_VALIST_H__
#define __X_VALIST_H__
#include "xvalue.h"
X_BEGIN_DECLS
#define X_VALUE_COLLECT_MAX_LENGTH  (8)
#define X_VALUE_COLLECT_INT         "i"
#define X_VALUE_COLLECT_LONG        "l"
#define X_VALUE_COLLECT_INT64       "q"
#define X_VALUE_COLLECT_DOUBLE      "d"
#define X_VALUE_COLLECT_PTR         "p"

#define X_VALUE_COLLECT_INIT(val,_type,argv,flags,error) X_STMT_START { \
    xValueTable     *vtable;                                            \
    xcstr           collect_format;                                     \
    xCValue         cvalues[X_VALUE_COLLECT_MAX_LENGTH];                \
    xsize           n_values = 0;                                       \
                                                                        \
    vtable = x_value_table_peek (_type);                                \
    collect_format = vtable->collect_format;                            \
    (val)->type = _type;                                                \
    while (*collect_format) {                                           \
        xCValue *cvalue = cvalues + n_values++;                         \
        switch (*collect_format++) {                                    \
        case 'i':                                                       \
            cvalue->v_int = va_arg (argv, xint);                        \
            break;                                                      \
        case 'l':                                                       \
            cvalue->v_long = va_arg (argv, xlong);                      \
            break;                                                      \
        case 'q':                                                       \
            cvalue->v_int64 = va_arg (argv, xint64);                    \
            break;                                                      \
        case 'd':                                                       \
            cvalue->v_double = va_arg (argv, xdouble);                  \
            break;                                                      \
        case 'p':                                                       \
            cvalue->v_pointer = va_arg (argv, xptr);                    \
            break;                                                      \
        default:                                                        \
            x_assert_not_reached ();                                    \
        }                                                               \
    }                                                                   \
    error = vtable->value_collect (val, n_values, cvalues, flags);      \
} X_STMT_END

#define X_VALUE_COLLECT(val, argv, flags, error) X_STMT_START {         \
    xValueTable     *vtable;                                            \
    xType value_type = X_VALUE_TYPE (val);                              \
    vtable = x_value_table_peek (value_type);                           \
    if (vtable->free)                                                   \
        vtable->free (val);                                             \
    memset (val->data, 0, sizeof (val->data));                          \
    X_VALUE_COLLECT_INIT(val, value_type, argv, flags, error);          \
} X_STMT_END


#define X_VALUE_COLLECT_SKIP(type, argv) X_STMT_START {                 \
    xValueTable     *vtable;                                            \
    xcstr           collect_format;                                     \
    xsize           n_values;                                           \
                                                                        \
    vtable = x_value_table_peek (type);                                 \
    collect_format = vtable->collect_format;                            \
    n_values = 0;                                                       \
    while (*collect_format) {											\
        switch (*collect_format++){	                                    \
        case 'i':							                            \
            va_arg (argv, xint);							            \
            break;									                    \
        case 'l':							                            \
            va_arg (argv, xlong);							            \
            break;									                    \
        case 'q':							                            \
            va_arg (argv, xint64);							            \
            break;									                    \
        case 'd':							                            \
            va_arg (argv, xdouble);							            \
            break;									                    \
        case 'p':							                            \
            va_arg (argv, xptr);						                \
            break;									                    \
        default:									                    \
            x_assert_not_reached ();		                            \
        }                                                               \
    }                                                                   \
} X_STMT_END

#define X_VALUE_LCOPY(val, argv, flags, error) X_STMT_START {           \
    xValueTable     *vtable;                                            \
    xcstr           lcopy_format;                                       \
    xCValue         cvalues[X_VALUE_COLLECT_MAX_LENGTH];                \
    xsize           n_values;                                           \
                                                                        \
    vtable = x_value_table_peek (X_VALUE_TYPE (val));                   \
    lcopy_format = vtable->lcopy_format;                                \
    n_values = 0;                                                       \
    while (*lcopy_format) {											    \
        xCValue *cvalue = cvalues + n_values++;					        \
        switch (*lcopy_format++){										\
        case 'i':							                            \
            cvalue->v_int = va_arg (argv, xint);					    \
            break;									                    \
        case 'l':							                            \
            cvalue->v_long = va_arg (argv, xlong);					    \
            break;									                    \
        case 'q':							                            \
            cvalue->v_int64 = va_arg (argv, xint64);				    \
            break;									                    \
        case 'd':							                            \
            cvalue->v_double = va_arg (argv, xdouble);				    \
            break;									                    \
        case 'p':							                            \
            cvalue->v_pointer = va_arg (argv, xptr);				    \
            break;									                    \
        default:									                    \
            x_assert_not_reached ();							        \
        }										                        \
    }											                        \
    error = vtable->value_lcopy(val, n_values, cvalues, flags);	        \
} X_STMT_END

X_END_DECLS
#endif /* __X_VALIST_H__ */
