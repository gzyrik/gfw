#ifndef __X_BOXED_H__
#define __X_BOXED_H__
#include "xclass.h"
X_BEGIN_DECLS

#define X_TYPE_XTYPE            x_type_type()
#define X_TYPE_STRV             x_strv_type()
#define X_TYPE_IS_BOXED(type)   (X_TYPE_FUNDAMENTAL (type) == X_TYPE_BOXED)
#define X_TYPE_IS_ENUM(type)    (X_TYPE_FUNDAMENTAL (type) == X_TYPE_ENUM)
#define X_IS_ENUM_CLASS(klass)  (X_CLASS_CHECK_TYPE ((klass), X_TYPE_ENUM))
#define X_TYPE_IS_FLAGS(type)   (X_TYPE_FUNDAMENTAL (type) == X_TYPE_FLAGS)
#define X_DEFINE_BOXED_TYPE(TypeName, type_name, copy, free)            \
    X_DEFINE_BOXED_TYPE_WITH_CODE(TypeName, type_name, copy, free, ;)

#define X_DEFINE_BOXED_TYPE_WITH_CODE(TypeName, type_name, copy, free, Codes)   \
    _X_DEFINE_BOXED_TYPE_BEGIN (TypeName, type_name, copy, free) {      \
        Codes;                                                          \
    } _X_DEFINE_BOXED_TYPE_END()


#define _X_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy, free)     \
    \
xType type_name##_type (void)                                           \
{                                                                       \
    static volatile xType x_type = 0;                                   \
    if ( !x_atomic_ptr_get (&x_type) && x_once_init_enter (&x_type) ){  \
        xcstr   name = x_intern_static_str(#TypeName);                  \
        x_type = x_boxed_register_static (name,                         \
                                          (xDupFunc)  copy,             \
                                          (xFreeFunc) free);
#define _X_DEFINE_BOXED_TYPE_END()                                      \
        x_once_init_leave (&x_type);                                    \
    }                                                                   \
    return x_type;                                                      \
} /* closes type_name##_type(void) */

#define X_DEFINE_PTR_TYPE(TypeName, type_name)                          \
    X_DEFINE_PTR_TYPE_WITH_CODE(ypeName, type_name, ;)

#define X_DEFINE_PTR_TYPE_WITH_CODE(TypeName, type_name, Codes)         \
    _X_DEFINE_PTR_TYPE_BEGIN (TypeName, type_name) {                    \
        Codes;                                                          \
    } _X_DEFINE_PTR_TYPE_END()


#define _X_DEFINE_PTR_TYPE_BEGIN(TypeName, type_name)                   \
    \
xType type_name##_type (void)                                           \
{                                                                       \
    static volatile xType x_type = 0;                                   \
    if ( !x_atomic_ptr_get (&x_type) && x_once_init_enter (&x_type) ){  \
        xcstr   name = x_intern_static_str(#TypeName);                  \
        x_type = x_ptr_register_static (name);
#define _X_DEFINE_PTR_TYPE_END()                                        \
        x_once_init_leave (&x_type);                                    \
    }                                                                   \
    return x_type;                                                      \
} /* closes type_name##_type(void) */

xType       x_boxed_register_static (xcstr          name,
                                     xDupFunc       boxed_copy,
                                     xFreeFunc      boxed_free);

xType       x_ptr_register_static   (xcstr          name);

xptr        x_boxed_copy            (xType          boxed_type,
                                     xcptr          src_boxed);

void        x_boxed_free            (xType          boxed_type,
                                     xptr           boxed);

xEnumValue* x_enum_get_value        (xEnumClass     *enum_class,
                                     xint	        value);

xType       x_enum_register_static  (xcstr	        name,
                                     xEnumValue     *const_static_values);

xType       x_closure_type          (void) X_CONST;

xType       x_value_type            (void) X_CONST;

xType       x_type_type             (void) X_CONST;

xType       x_strv_type             (void) X_CONST;

struct _xEnumValue
{
  xint	            value;
  xcstr             value_name;
  xcstr             value_nick;
};

struct _xFlagsValue
{
  xint	            value;
  xcstr             value_name;
  xcstr             value_nick;
};
struct _xEnumClass
{
    xClass          parent;

    /*< public >*/  
    xint	        minimum;
    xint	        maximum;
    xuint	        n_values;
    xEnumValue      *values;
};
struct _xFlagsClass
{
    xClass          parent;

    /*< public >*/  
    xuint	        mask;
    xuint	        n_values;
    xFlagsValue     *values;
};

X_END_DECLS
#endif /* __X_BOXED_H__ */
