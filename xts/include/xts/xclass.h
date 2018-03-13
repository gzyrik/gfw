#ifndef __X_CLASS_H__
#define __X_CLASS_H__
#include "xtype.h"
X_BEGIN_DECLS

#define X_CLASS(klass)              ((xClass*)klass)
#define X_IFACE(iface)              ((xIFace*)iface)
#define X_INSTANCE(instance)        ((xInstance*)instance)
#define X_INSTANCE_CLASS(inst)      (X_INSTANCE(inst)->klass)
#define X_INSTANCE_TYPE(inst)       (X_INSTANCE_CLASS(inst)->type)
#define X_CLASS_TYPE(klass)         (X_CLASS(klass)->type)
#define X_IFACE_TYPE(iface)         (X_IFACE(iface)->type)

typedef enum    /*< skip >*/
{
    X_TYPE_CLASSED                  = (1 << 0),
    X_TYPE_INSTANTIATABLE           = (1 << 1),
    X_TYPE_DERIVABLE                = (1 << 2),
    X_TYPE_DEEP_DERIVABLE           = (1 << 3)
} xFundamentalFlags;

typedef enum    /*< skip >*/
{
    X_TYPE_ABSTRACT		            = (1 << 4),
    X_TYPE_VALUE_ABSTRACT	        = (1 << 5),
} xTypeFlags;

xType       x_type_register_static  (xcstr          type_name,
                                     xType          parent_type,
                                     const xTypeInfo    *info,
                                     xTypeFlags     flags);

xType       x_type_register_dynamic (xcstr          type_name,
                                     xType          parent_type,
                                     xTypePlugin    *plugin,
                                     xTypeFlags     flags);

xType  x_type_register_fundamental  (xType          type_id,
                                     xcstr          type_name,
                                     const xTypeInfo    *info,
                                     const xFundamentalInfo *finfo,
                                     xTypeFlags     flags);

void        x_type_add_iface_static (xType          instance_type,
                                     xType          iface_type,
                                     const xIFaceInfo   *info);

void        x_type_add_iface_dynamic(xType          instance_type,
                                     xType          iface_type,
                                     xTypePlugin    *plugin);

xbool       x_type_test_flags       (xType          type,
                                     xuint          flags);

xcstr       x_type_name             (xType          type);

xType       x_type_from_mime        (xType          type,
                                     xcstr          mime);

xTypePlugin*x_type_plugin           (xType          type);

xTypePlugin*x_type_iface_plugin     (xType          type,
                                     xType          iface_type);

xType       x_type_from_name        (xcstr          name);

xType       x_type_fundamental      (xType          type);

xType       x_type_parent           (xType          type);

xsize       x_type_depth            (xType          type);

xType*      x_type_children         (xType          type,
                                     xsize          *n_children);

xType*      x_type_ifaces           (xType          type,
                                     xsize          *n_ifaces);

void        x_type_set_private      (xType          type,
                                     xsize          private_size);

void        x_type_set_mime         (xType          type,
                                     xcstr          mime);

xbool       x_type_is               (xType          type,
                                     xType          ancestor);

xbool       x_class_is              (const xClass   *klass,
                                     xType          type);

xClass*     x_class_cast            (const xClass   *klass,
                                     xType          type);

xptr        x_class_peek            (xType          type);

xptr        x_class_peek_parent     (xcptr          klass);

xptr        x_iface_peek            (xcptr          klass,
                                     xType          iface_type);

void        x_iface_add_prerequisite(xType          iface,
                                     xType          prerequisite_type);

xptr        x_iface_peek_parent     (xcptr           iface);

xptr        x_class_ref             (xType          type);

void        x_class_unref           (xptr           klass);

void        x_class_set_private     (xcptr          klass,
                                     xsize          private_size);

xbool       x_instance_check        (xcptr          instance);

xbool       x_instance_is           (xcptr          instance,
                                     xType          type);

xInstance*  x_instance_cast         (xcptr          instance,
                                     xType          type);

xptr        x_instance_new          (xType          type);

void        x_instance_delete       (xptr           instance);

xptr        x_instance_private      (xcptr          instance,
                                     xType          private_type);

xbool       x_value_type_check      (xType          type);

xbool       x_value_holds           (const xValue   *value,
                                     xType          type);

xValueTable*x_value_table_peek      (xType          type);

#define X_TYPE_FUNDAMENTAL(type)	(x_type_fundamental (type))

#define X_TYPE_IS_INSTANTIATABLE(type)                                  \
    (x_type_test_flags ((type), X_TYPE_INSTANTIATABLE))

#define X_TYPE_IS_ABSTRACT(type)                                        \
    (x_type_test_flags ((type), X_TYPE_ABSTRACT))

#define X_TYPE_IS_CLASSED(type)                                         \
    (x_type_test_flags ((type), X_TYPE_CLASSED))

#define X_TYPE_IS_IFACE(type)                                           \
    (X_TYPE_FUNDAMENTAL (type) == X_TYPE_IFACE)

#define X_TYPE_IS_OBJECT(type)                                          \
    (X_TYPE_FUNDAMENTAL (type) == X_TYPE_OBJECT)

#define X_TYPE_IS_VALUE(type)                                           \
    (x_value_type_check (type))

#define X_INSTANCE_CHECK(instance)                                      \
    (x_instance_check(instance))
/*< utitly macro >*/
#ifndef X_DISABLE_CAST_CHECKS

#define X_INSTANCE_CAST(instance, X_Type, C_Type)                       \
    ((C_Type*) x_instance_cast (X_INSTANCE (instance), X_Type))

#define X_CLASS_CAST(klass, X_Type, C_Type)                             \
    ((C_Type*) x_class_cast (X_CLASS (klass), X_Type))

#else /* !X_DISABLE_CAST_CHECKS */

#define X_INSTANCE_CAST(instance, X_Type, C_Type)                       \
    ((C_Type*) X_INSTANCE (instance))

#define X_CLASS_CAST(klass, X_Type, C_Type)                             \
    ((C_Type*) X_CLASS (klass))

#endif /* X_DISABLE_CAST_CHECKS */

#define X_CLASS_CHECK_TYPE(klass, type)                                 \
    (x_class_is (X_CLASS(klass), (type)))

#define X_INSTANCE_TYPE_NAME(instance)                                  \
    (x_type_name (X_INSTANCE_TYPE(instance)))

#define X_CLASS_NAME(klass)                                             \
    (x_type_name (X_CLASS_TYPE(klass)))

#define X_INSTANCE_IS_TYPE(instance, X_Type)                            \
    (x_instance_is (X_INSTANCE(instance), X_Type))

#define X_CLASS_IS_TYPE(klass, X_Type)                                  \
    (x_class_cast (X_CLASS(klass), X_Type) != NULL)

#define X_INSTANCE_GET_CLASS(instance, X_Type, C_Type)                  \
    (X_CLASS_CAST (x_class_peek (X_INSTANCE_TYPE(instance)), X_Type, C_Type))

#define X_INSTANCE_GET_IFACE(instance, X_Type, C_Type)                  \
    ((C_Type*) x_iface_peek (X_INSTANCE(instance)->klass, X_Type))

#define X_CLASS_GET_IFACE(inst, X_Type, C_Type)                         \
    ((C_Type*) x_class_iface (X_CLASS_TYPE(inst), X_Type))

#define X_INSTANCE_GET_PRIVATE(inst, X_Type, C_Type)                    \
    ((C_Type*)x_instance_private (X_INSTANCE(inst),X_Type))

/* define class */
#define X_DEFINE_TYPE(TypeName, type_name, TYPE_PARENT)                 \
    X_DEFINE_TYPE_EXTENDED (TypeName, type_name, TYPE_PARENT, 0, ;)

#define X_DEFINE_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, Codes)\
    _X_DEFINE_TYPE_BEGIN(TypeName, type_name,TYPE_PARENT, flags){       \
        Codes;                                                          \
    } _X_DEFINE_TYPE_END(TypeName, type_name)

#define _X_DEFINE_TYPE_BEGIN(TypeName, type_name, TYPE_PARENT, flags)   \
    \
static void type_name##_init        (TypeName        *self);            \
static void type_name##_class_init  (TypeName##Class *klass);           \
static xptr type_name##_parent_class = NULL;                            \
static void type_name##_class_intern_init (TypeName##Class *klass)      \
{                                                                       \
    type_name##_parent_class = x_class_peek_parent (klass);             \
    type_name##_class_init ((TypeName##Class*) klass);                  \
}                                                                       \
xType type_name##_type (void)                                           \
{                                                                       \
    static volatile xType x_type = 0;                                   \
    if ( !x_atomic_ptr_get (&x_type) && x_once_init_enter (&x_type) ){  \
        const xTypeInfo type_info = {                                   \
            sizeof (TypeName##Class),                                   \
            NULL,                                                       \
            NULL,                                                       \
            (xClassInitFunc)type_name##_class_intern_init,              \
            NULL,                                                       \
            NULL,                                                       \
            sizeof(TypeName),                                           \
            (xInstanceInitFunc)type_name##_init,                        \
            NULL                                                        \
        };                                                              \
        xcstr   name = x_intern_static_str(#TypeName);                  \
        x_type  = x_type_register_static (name,                         \
                                          TYPE_PARENT,                  \
                                          &type_info,                   \
                                          flags);
#define _X_DEFINE_TYPE_END(TypeName, type_name)                         \
        x_once_init_leave (&x_type);                                    \
    }                                                                   \
    return x_type;                                                      \
} /* closes type_name##_type(void) */

#define X_IMPLEMENT_IFACE(TYPE_IFACE, iface_init) {                     \
    const xIFaceInfo iface_info = {                                     \
        (xIFaceInitFunc) iface_init, NULL, NULL                         \
    };                                                                  \
    x_type_add_iface_static (x_type, TYPE_IFACE, &iface_info);          \
}

#define X_DEFINE_IFACE(TypeName, type_name)                             \
    X_DEFINE_IFACE_WITH_CODE (TypeName, type_name, 0, ;)

#define X_DEFINE_IFACE_WITH_CODE(TypeName, type_name,TYPE_PREREQ, Codes)\
    _X_DEFINE_IFACE_BEGIN(TypeName, type_name,TYPE_PREREQ){             \
        Codes;                                                          \
    } _X_DEFINE_IFACE_END()

#define _X_DEFINE_IFACE_BEGIN(TypeName, type_name, TYPE_PREREQ)         \
    \
static void     type_name##_default_init    (TypeName##IFace *klass);   \
xType type_name##_type (void)                                           \
{ \
    static volatile xType x_type = 0;                                   \
    if ( !x_atomic_ptr_get (&x_type) && x_once_init_enter (&x_type) ){  \
        const xTypeInfo type_info = {                                   \
            sizeof (TypeName##IFace),                                   \
            NULL,                                                       \
            NULL,                                                       \
            (xClassInitFunc)type_name##_default_init,                   \
            NULL,                                                       \
            NULL,                                                       \
            0,                                                          \
            NULL,                                                       \
            NULL                                                        \
        };                                                              \
        xcstr   name = x_intern_static_str (#TypeName);                 \
        x_type  = x_type_register_static (name,                         \
                                          X_TYPE_IFACE,                 \
                                          &type_info,                   \
                                          0);                           \
        if (TYPE_PREREQ) {                                              \
            x_iface_add_prerequisite (x_type, TYPE_PREREQ);             \
        }
#define _X_DEFINE_IFACE_END()                                           \
        x_once_init_leave (&x_type);                                    \
    }                                                                   \
    return x_type;                                                      \
} /* closes type_name##_type(void) */

struct _xValueTable
{
    void (*value_init)      (xValue*);
    void (*value_clear)     (xValue*);
    void (*value_copy)      (const xValue*, xValue*);

    /* varargs functionality (optional) */
    xptr (*value_peek)      (const xValue*);
    xcstr                   collect_format;
    xstr (*value_collect)   (xValue*, xsize, xCValue*, xuint);
    xcstr                   lcopy_format;
    xstr (*value_lcopy)     (const xValue*, xsize, xCValue*, xuint);
};
typedef void   (*xIFaceInitFunc)         (xptr         xiface,
                                          xptr         iface_data);
typedef void   (*xIFaceFinalizeFunc)     (xptr         xiface,
                                          xptr         iface_data);
struct _xFundamentalInfo
{
    xFundamentalFlags   type_flags;
};

struct _xIFaceInfo
{
    xIFaceInitFunc      iface_init;
    xIFaceFinalizeFunc  iface_finalize;
    xcptr               iface_data;
};
typedef void   (*xBaseInitFunc)              (xptr         xclass);
typedef void   (*xBaseFinalizeFunc)          (xptr         xclass);
typedef void   (*xClassInitFunc)             (xptr         xclass,
                                              xptr         class_data);
typedef void   (*xClassFinalizeFunc)         (xptr         xclass,
                                              xptr         class_data);
typedef void   (*xInstanceInitFunc)          (xInstance    *instance,
                                              xptr         xclass);
struct _xTypeInfo
{
    xuint16             class_size;
    xBaseInitFunc       base_init;
    xBaseFinalizeFunc   base_finalize;

    xClassInitFunc      class_init;
    xClassFinalizeFunc  class_finalize;
    xcptr               class_data;

    xuint16             instance_size;
    xInstanceInitFunc   instance_init;
    const xValueTable	*value_table;
};

struct _xIFace
{
    /*< protected >*/
    xType               type;
    xType               instance_type;
};

struct _xClass
{
    /*< protected >*/
    xType               type;
};

struct _xInstance
{
    /*< protected >*/
    xClass*             klass;
};


X_END_DECLS
#endif /* __X_CLASS_H__ */
