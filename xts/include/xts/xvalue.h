#ifndef __X_VALUE_H__
#define __X_VALUE_H__
#include "xtype.h"
X_BEGIN_DECLS

#define	X_VALUE_TYPE(val)		    (((xValue*) (val))->type)
#define X_IS_VALUE(val)             (val && x_value_type_check (X_VALUE_TYPE(val)))
#define X_VALUE_TYPE_NAME(val)      (x_type_name (X_VALUE_TYPE (val)))
#define X_VALUE_INIT                { 0, { { 0 } } }

#define X_VALUE_FLAGS_MASK          (0x7FFFFFFF)
#define X_VALUE_NOCOPY_CONTENTS     (1 << 31)

#define X_VALUE_HOLDS(val,type)     x_value_holds(val, type)
#define X_VALUE_HOLDS_BOOL(val)     X_VALUE_HOLDS(val, X_TYPE_BOOL)
#define X_VALUE_HOLDS_CHAR(val)     X_VALUE_HOLDS(val, X_TYPE_CHAR)
#define X_VALUE_HOLDS_UCHAR(val)    X_VALUE_HOLDS(val, X_TYPE_UCHAR)
#define X_VALUE_HOLDS_INT(val)      X_VALUE_HOLDS(val, X_TYPE_INT)
#define X_VALUE_HOLDS_UINT(val)     X_VALUE_HOLDS(val, X_TYPE_UINT)
#define X_VALUE_HOLDS_LONG(val)     X_VALUE_HOLDS(val, X_TYPE_LONG)
#define X_VALUE_HOLDS_ULONG(val)    X_VALUE_HOLDS(val, X_TYPE_ULONG)
#define X_VALUE_HOLDS_FLOAT(val)    X_VALUE_HOLDS(val, X_TYPE_FLOAT)
#define X_VALUE_HOLDS_DOUBLE(val)   X_VALUE_HOLDS(val, X_TYPE_DOUBLE)
#define X_VALUE_HOLDS_INT64(val)    X_VALUE_HOLDS(val, X_TYPE_INT64)
#define X_VALUE_HOLDS_UINT64(val)   X_VALUE_HOLDS(val, X_TYPE_UINT64)
#define X_VALUE_HOLDS_ENUM(val)     X_VALUE_HOLDS(val, X_TYPE_ENUM)
#define X_VALUE_HOLDS_FLAGS(val)    X_VALUE_HOLDS(val, X_TYPE_FLAGS)
#define X_VALUE_HOLDS_PTR(val)      X_VALUE_HOLDS(val, X_TYPE_PTR)
#define X_VALUE_HOLDS_STR(val)      X_VALUE_HOLDS(val, X_TYPE_STR)
#define X_VALUE_HOLDS_BOXED(val)    X_VALUE_HOLDS(val, X_TYPE_BOXED)
#define X_VALUE_HOLDS_PARAM(val)    X_VALUE_HOLDS(val, X_TYPE_PARAM)
#define X_VALUE_HOLDS_OBJECT(val)   X_VALUE_HOLDS(val, X_TYPE_OBJECT)

typedef void (*xValueTransformFunc)   (const xValue     *src_data,
                                       xValue           *dst_data);
xValue*     x_value_init            (xValue         *val,
                                     xType          type);

xValue*     x_value_reset           (xValue         *val);

void        x_value_clear           (xValue         *val);

void        x_value_copy            (const xValue   *src,
                                     xValue         *dest);

xbool       x_value_transform       (const xValue   *src,
                                     xValue         *dst);

xbool       x_value_type_compatible (xType          src,
                                     xType          dst);

xbool       x_value_type_transformable  (xType          src,
                                         xType          dst);

void        x_value_register_transform  (xType          src,
                                         xType          dst,
                                         xValueTransformFunc transform);

xstr        x_strdup_value_contents (const xValue   *val);

xbool       x_value_get_bool        (const xValue   *val);
void        x_value_set_bool        (xValue         *val,
                                     xbool          b);
xchar       x_value_get_char        (const xValue   *val);
void        x_value_set_char        (xValue         *val,
                                     xchar          c);
xuchar      x_value_get_uchar       (const xValue   *val);
void        x_value_set_uchar       (xValue         *val,
                                     xuchar         c);
xint        x_value_get_int         (const xValue   *val);
void        x_value_set_int         (xValue         *val,
                                     xint           i);
xuint       x_value_get_uint        (const xValue   *val);
void        x_value_set_uint        (xValue         *val,
                                     xuint          i);
xlong       x_value_get_long        (const xValue   *val);
void        x_value_set_long        (xValue         *val,
                                     xlong          l);
xulong      x_value_get_ulong       (const xValue   *val);
void        x_value_set_ulong       (xValue         *val,
                                     xulong         l);
xfloat      x_value_get_float       (const xValue   *val);
void        x_value_set_float       (xValue         *val,
                                     xfloat         f);
xdouble     x_value_get_double      (const xValue   *val);
void        x_value_set_double      (xValue         *val,
                                     xdouble        d);
xint64      x_value_get_int64       (const xValue   *val);
void        x_value_set_int64       (xValue         *val,
                                     xint64         i);
xuint64     x_value_get_uint64      (const xValue   *val);
void        x_value_set_uint64      (xValue         *val,
                                     xuint64         i);
xint        x_value_get_enum        (const xValue   *val);
void        x_value_set_enum        (xValue         *val,
                                     xint           e);
xuint       x_value_get_flags       (const xValue   *val);
void        x_value_set_flags       (xValue         *val,
                                     xuint           f);
xptr        x_value_get_ptr         (const xValue   *val);
xbool       x_value_fits_ptr        (const xValue   *val);
xptr        x_value_peek_ptr        (const xValue   *val);
void        x_value_set_ptr         (xValue         *val,
                                     xptr           p);
xcstr       x_value_get_str         (const xValue   *val);
xstr        x_value_dup_str         (const xValue   *val);
void        x_value_set_str         (xValue         *val,
                                     xcstr          str);
void        x_value_set_static_str  (xValue         *val,
                                     xcstr          str);
void        x_value_take_str        (xValue         *val,
                                     xstr           str);

xcptr       x_value_get_boxed       (const xValue   *val);
xptr        x_value_dup_boxed       (const xValue   *val);
void        x_value_set_boxed       (xValue         *val,
                                     xcptr          boxed);
void        x_value_set_static_boxed(xValue         *val,
                                     xcptr          boxed);
void        x_value_take_boxed      (xValue        *val,
                                     xcptr          boxed);

xptr        x_value_get_param       (xValue         *val);
void        x_value_set_param       (xValue         *val,
                                     xParam         *param);
void        x_value_take_param      (xValue         *val,
                                     xParam         *param);

xptr        x_value_get_object      (xValue         *val);
void        x_value_set_object      (xValue         *val,
                                     xptr           object);
void        x_value_take_object     (xValue         *val,
                                     xObject        *object);

void        x_value_set_instance    (xValue         *val,
                                     xptr           object);
struct _xValue
{
    xType type;
    union {
        xchar   v_char;
        xuchar  v_uchar;
        xint    v_int;
        xuint   v_uint;
        xlong   v_long;
        xulong  v_ulong;
        xint64  v_int64;
        xuint64 v_uint64;
        xfloat  v_float;
        xdouble v_double;
        xptr    v_pointer;
    } data[2];
};

struct _xCValue
{
    xint        v_int;
    xlong       v_long;
    xint64      v_int64;
    xfloat      v_float;
    xdouble     v_double;
    xptr        v_pointer;
};

struct _xParamValue
{
    xcstr                   name;
    xValue                  value;
};

X_END_DECLS
#endif /* __X_VALUE_H__ */
