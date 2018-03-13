#ifndef __X_PARAM_H__
#define __X_PARAM_H__
#include "xtype.h"
X_BEGIN_DECLS
#define X_TYPE_IS_PARAM(type)       (X_TYPE_FUNDAMENTAL (type) == X_TYPE_PARAM)
#define X_PARAM(pspec)              (X_INSTANCE_CAST ((pspec), X_TYPE_PARAM, xParam))
#define X_IS_PARAM(pspec)           (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM))
#define X_PARAM_CLASS(klass)        (X_CLASS_CAST ((klass), X_TYPE_PARAM, xParamClass))
#define X_IS_PARAM_CLASS(klass)     (X_CLASS_IS_TYPE ((klass), X_TYPE_PARAM))
#define X_PARAM_GET_CLASS(pspec)    (X_INSTANCE_GET_CLASS ((pspec), X_TYPE_PARAM, xParamClass))

#define X_PARAM_VALUE_TYPE(pspec)   (X_PARAM (pspec)->value_type)
#define X_PARAM_TYPE(pspec)         (X_INSTANCE_TYPE (pspec))
#define X_PARAM_TYPE_NAME(pspec)    (x_type_name (X_PARAM_TYPE (pspec)))

#define X_TYPE_PARAM_CHAR           (x_param_types()[0])
#define X_IS_PARAM_CHAR(pspec)      (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_CHAR))
#define X_PARAM_CHAR(pspec)         (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_CHAR, xParamChar))

#define X_TYPE_PARAM_UCHAR          (x_param_types()[1])
#define X_IS_PARAM_UCHAR(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_UCHAR))
#define X_PARAM_UCHAR(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_UCHAR, xParamUChar))

#define X_TYPE_PARAM_BOOL           (x_param_types()[2])
#define X_IS_PARAM_BOOL(pspec)      (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_BOOL))
#define X_PARAM_BOOL(pspec)         (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_BOOL, xParamBool))

#define X_TYPE_PARAM_INT            (x_param_types()[3])
#define X_IS_PARAM_INT(pspec)       (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_INT))
#define X_PARAM_INT(pspec)          (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_INT, xParamInt))

#define X_TYPE_PARAM_UINT           (x_param_types()[4])
#define X_IS_PARAM_UINT(pspec)      (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_UINT))
#define X_PARAM_UINT(pspec)         (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_UINT, xParamUInt))

#define X_TYPE_PARAM_LONG           (x_param_types()[5])
#define X_IS_PARAM_LONG(pspec)      (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_LONG))
#define X_PARAM_LONG(pspec)         (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_LONG, xParamLong))

#define X_TYPE_PARAM_ULONG          (x_param_types()[6])
#define X_IS_PARAM_ULONG(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_ULONG))
#define X_PARAM_ULONG(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_ULONG, xParamULong))

#define X_TYPE_PARAM_INT64          (x_param_types()[7])
#define X_IS_PARAM_INT64(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_INT64))
#define X_PARAM_INT64(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_INT64, xParamInt64))

#define X_TYPE_PARAM_UINT64         (x_param_types()[8])
#define X_IS_PARAM_UINT64(pspec)    (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_UINT64))
#define X_PARAM_UINT64(pspec)       (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_UINT64, xParamUInt64))

#define X_TYPE_PARAM_WCHAR          (x_param_types()[9])
#define X_IS_PARAM_WCHAR(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_WCHAR))
#define X_PARAM_WCHAR(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_WCHAR, xParamWChar))

#define X_TYPE_PARAM_ENUM           (x_param_types()[10])
#define X_IS_PARAM_ENUM(pspec)      (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_ENUM))
#define X_PARAM_ENUM(pspec)         (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_ENUM, xParamEnum))

#define X_TYPE_PARAM_FLAGS          (x_param_types()[11])
#define X_IS_PARAM_FLAGS(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_FLAGS))
#define X_PARAM_FLAGS(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_FLAGS, xParamFlags))

#define X_TYPE_PARAM_FLOAT          (x_param_types()[12])
#define X_IS_PARAM_FLOAT(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_FLOAT))
#define X_PARAM_FLOAT(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_FLOAT, xParamFloat))

#define X_TYPE_PARAM_DOUBLE         (x_param_types()[13])
#define X_IS_PARAM_DOUBLE(pspec)    (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_DOUBLE))
#define X_PARAM_DOUBLE(pspec)       (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_DOUBLE, xParamDouble))

#define X_TYPE_PARAM_STR            (x_param_types()[14])
#define X_IS_PARAM_STR(pspec)       (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_STR))
#define X_PARAM_STR(pspec)          (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_STR, xParamStr))

#define X_TYPE_PARAM_PARAM          (x_param_types()[15])
#define X_IS_PARAM_PARAM(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_PARAM))
#define X_PARAM_PARAM(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_PARAM, xParamParam))

#define X_TYPE_PARAM_BOXED          (x_param_types()[16])
#define X_IS_PARAM_BOXED(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_BOXED))
#define X_PARAM_BOXED(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_BOXED, xParamBoxed))

#define X_TYPE_PARAM_PTR            (x_param_types()[17])
#define X_IS_PARAM_PTR(pspec)       (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_PTR))
#define X_PARAM_PTR(pspec)          (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_PTR, xParamPtr))

#define X_TYPE_PARAM_OBJECT         (x_param_types()[18])
#define X_IS_PARAM_OBJECT(pspec)    (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_OBJECT))
#define X_PARAM_OBJECT(pspec)       (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_OBJECT, xParamObject))

#define X_TYPE_PARAM_OVERRIDE       (x_param_types()[19])
#define X_IS_PARAM_OVERRIDE(pspec)  (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_OVERRIDE))
#define X_PARAM_OVERRIDE(pspec)     (X_INSTANCE_CAST ((pspec), X_TYPE_PARAM_OVERRIDE, xParamOverride))

#define X_TYPE_PARAM_XTYPE          (x_param_types()[20])
#define X_IS_PARAM_XTYPE(pspec)     (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_XTYPE))
#define X_PARAM_XTYPE(pspec)        (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_XTYPE, xParamXType))

#define X_TYPE_PARAM_STRV           (x_param_types()[21])
#define X_IS_PARAM_STRV(pspec)      (X_INSTANCE_IS_TYPE ((pspec), X_TYPE_PARAM_STR))
#define X_PARAM_STRV(pspec)         (X_INSTANCE_CAST((pspec), X_TYPE_PARAM_STR, xParamStr))

typedef enum {
    X_PARAM_READABLE                = 1 << 0,
    X_PARAM_WRITABLE                = 1 << 1,
    X_PARAM_CONSTRUCT               = 1 << 2,
    X_PARAM_STATIC_NAME             = 1 << 3,
    X_PARAM_STATIC_NICK             = 1 << 4,
    X_PARAM_STATIC_BLURB            = 1 << 5,
    X_PARAM_LAX_VALIDATION          = 1 << 6,
    X_PARAM_CONSTRUCT_ONLY          = 1 << 7,
    X_PARAM_DEPRECATED              = 1 << 31,
    X_PARAM_READWRITE               = (X_PARAM_READABLE | X_PARAM_WRITABLE),
    X_PARAM_STATIC_STR              = (X_PARAM_STATIC_NAME | X_PARAM_STATIC_NICK | X_PARAM_STATIC_BLURB)
} xParamFlag;
#define X_PARAM_USER_SHIFT          (8)
#define X_PARAM_USER_MASK           (~0 << X_PARAM_USER_SHIFT)
#define X_PARAM_MASK                (0x000000ff)

xParam*     x_param_sink            (xParam         *pspec);

xParam*     x_param_ref             (xParam         *pspec);

xint        x_param_unref           (xParam         *pspec);

void        x_param_value_init      (const xParam   *pspec,
                                     xValue         *value);

xbool       x_param_value_default   (const xParam   *pspec,
                                     const xValue   *value);

xbool       x_param_value_validate  (const xParam   *pspec,
                                     xValue         *value);

xbool       x_param_value_convert   (const xParam   *pspec,
                                     const xValue   *src_value,
                                     xValue         *dest_value,
                                     xbool          strict_validation);

xint        x_param_value_cmp       (const xParam   *pspec,
                                     const xValue   *value1,
                                     const xValue   *value2);

xParam*     x_param_get_redirect    (xParam         *pspec);

xType       x_param_register_static (xcstr          name,
                                     const xParamInfo   *pspec_info);

const xType*x_param_types           (void);


xptr        x_param_new		        (xType          param_type,
                                     xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xuint          flags);

xParam*     x_param_char_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xchar          minimum,
                                     xchar          maximum,
                                     xchar          default_value,
                                     xuint          flags);

xParam*     x_param_uchar_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xuchar         minimum,
                                     xuchar         maximum,
                                     xuchar         default_value,
                                     xuint          flags);

xParam*     x_param_bool_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xbool          default_value,
                                     xuint          flags);

xParam*     x_param_int_new		    (xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xint           minimum,
                                     xint           maximum,
                                     xint           default_value,
                                     xuint          flags);

xParam*     x_param_uint_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xuint          minimum,
                                     xuint          maximum,
                                     xuint          default_value,
                                     xuint          flags);

xParam*     x_param_long_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xlong          minimum,
                                     xlong          maximum,
                                     xlong          default_value,
                                     xuint          flags);

xParam*     x_param_ulong_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xulong         minimum,
                                     xulong         maximum,
                                     xulong         default_value,
                                     xuint          flags);

xParam*     x_param_int64_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xint64         minimum,
                                     xint64         maximum,
                                     xint64         default_value,
                                     xuint          flags);

xParam*     x_param_uint64_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xuint64        minimum,
                                     xuint64        maximum,
                                     xuint64        default_value,
                                     xuint          flags);

xParam*     x_param_wchar_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xwchar         default_value,
                                     xuint          flags);

xParam*     x_param_enum_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xType          enum_type,
                                     xint           default_value,
                                     xuint          flags);

xParam*     x_param_flags_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xType          flags_type,
                                     xuint          default_value,
                                     xuint          flags);

xParam*     x_param_float_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xfloat         minimum,
                                     xfloat         maximum,
                                     xfloat         default_value,
                                     xuint          flags);

xParam*     x_param_double_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xdouble        minimum,
                                     xdouble        maximum,
                                     xdouble        default_value,
                                     xuint          flags);

xParam*     x_param_str_new		    (xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xcstr          default_value,
                                     xuint          flags);

xParam*     x_param_strv_new		(xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xuint          flags);

xParam*     x_param_boxed_new       (xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xType          boxed_type,
                                     xuint	        flags);

xParam*	    x_param_ptr_new         (xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xuint	        flags);

xParam*     x_param_object_new      (xcstr          name,
                                     xcstr          nick,
                                     xcstr          blurb,
                                     xType          object_type,
                                     xuint          flags);

xParam*     x_param_override_new    (xcstr          name,
                                     xParam*        overridden);

struct _xParam
{
    xInstance               parent;

    xQuark                  name;
    xstr                    nick;
    xstr                    blurb;
    xuint                   flags;
    xType                   value_type;
    xType                   owner_type;

    /*< private >*/
    xDataList               *qdata;
    xint                    ref_count;
    xuint                   param_id;
};

struct _xParamInfo
{
    xsize                   instance_size;
    void  (*instance_init)  (xParam*);
    xType                   value_type;
    void  (*finalize)       (xParam*);
    void  (*value_init)     (const xParam*, xValue*);
    xbool (*value_validate) (const xParam*, xValue*);
    xint  (*value_cmp)      (const xParam*, const xValue*, const xValue*);
};

struct _xParamClass
{
    xClass                  parent;

    xType                   value_type;
    void  (*finalize)       (xParam*);
    void  (*value_init)     (const xParam*, xValue*);
    xbool (*value_validate) (const xParam*, xValue*);
    xint  (*value_cmp)      (const xParam*, const xValue*, const xValue*);
};
struct _xParamChar  
{
    /*< private >*/
    xParam                  parent;
    xchar                   minimum;
    xchar                   maximum;
    xchar                   default_value;
};
struct _xParamUChar 
{
    /*< private >*/
    xParam                  parent;
    xuchar                  minimum;
    xuchar                  maximum;
    xuchar                  default_value;
};
struct _xParamBool  
{
    /*< private >*/
    xParam                  parent;
    xbool                   default_value;
};
struct _xParamInt   
{
    /*< private >*/
    xParam                  parent;
    xint                    minimum;
    xint                    maximum;
    xint                    default_value;
};
struct _xParamUInt  
{
    /*< private >*/
    xParam                  parent;
    xuint                   minimum;
    xuint                   maximum;
    xuint                   default_value;
};
struct _xParamLong  
{
    /*< private >*/
    xParam                  parent;
    xlong                   minimum;
    xlong                   maximum;
    xlong                   default_value;
};
struct _xParamULong 
{
    /*< private >*/
    xParam                  parent;
    xulong                  minimum;
    xulong                  maximum;
    xulong                  default_value;
};
struct _xParamInt64 
{
    /*< private >*/
    xParam                  parent;
    xint64                  minimum;
    xint64                  maximum;
    xint64                  default_value;
};
struct _xParamUInt64
{
    /*< private >*/
    xParam                  parent;
    xuint64                 minimum;
    xuint64                 maximum;
    xuint64                 default_value;
};
struct _xParamWChar
{
    xParam                  parent;
    xwchar                  default_value;
};
struct _xParamEnum  
{
    /*< private >*/
    xParam                  parent;
    xEnumClass              *enum_class;
    xint                    default_value;
};
struct _xParamFlags 
{
    /*< private >*/
    xParam                  parent;
    xFlagsClass             *flags_class;
    xuint                   default_value;
};
struct _xParamFloat 
{
    /*< private >*/
    xParam                  parent;
    xfloat                  minimum;
    xfloat                  maximum;
    xfloat                  default_value;
    xfloat                  epsilon;
};
struct _xParamDouble
{
    /*< private >*/
    xParam                  parent;
    xdouble                 minimum;
    xdouble                 maximum;
    xdouble                 default_value;
    xdouble                 epsilon;
};
struct _xParamStr   
{
    /*< private >*/
    xParam                  parent;
    xstr                    default_value;
    xstr                    cset_first;
    xstr                    cset_nth;
    xchar                   substitutor;
    xuint                   null_fold_if_empty : 1;
    xuint                   ensure_non_null : 1;
};
struct _xParamStrV
{
    /*< private >*/
    xParam                  parent;
};
struct _xParamParam 
{
    /*< private >*/
    xParam                  parent;
};
struct _xParamBoxed 
{
    /*< private >*/
    xParam                  parent;
};
struct _xParamPtr   
{
    /*< private >*/
    xParam                  parent;
};
struct _xParamObject
{
    /*< private >*/
    xParam                  parent;
};
struct _xParamOverride
{
    /*< private >*/
    xParam                  parent;
    xParam                  *overridden;
};

struct _xParamXType
{
    xParam                  parent;
    xType                   xtype;
};

X_END_DECLS
#endif /* __X_PARAM_H__ */
