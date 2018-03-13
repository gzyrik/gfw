#ifndef __X_TYPE_MODULE_H__
#define __X_TYPE_MODULE_H__
#include "xobject.h"
X_BEGIN_DECLS

#define X_TYPE_TYPE_MODULE          (x_type_module_type())
#define X_TYPE_MODULE(obj)          (X_INSTANCE_CAST    ((obj), X_TYPE_TYPE_MODULE, xTypeModule))
#define X_TYPE_MODULE_CLASS(klass)  (X_CLASS_CAST       ((klass), X_TYPE_TYPE_MODULE, xTypeModuleClass))
#define X_IS_TYPE_MODULE(obj)       (X_INSTANCE_IS_TYPE (((obj)), X_TYPE_TYPE_MODULE))
#define X_IS_TYPE_MODULE_CLASS(klass)   (X_CLASS_IS_TYPE  ((klass), X_TYPE_TYPE_MODULE))
#define X_TYPE_MODULE_GET_CLASS(obj)    (X_INSTANCE_GET_CLASS ((obj), X_TYPE_TYPE_MODULE, xTypeModuleClass))

xType       x_type_module_type      ();

xType       x_type_module_register  (xTypeModule        *module,
                                     xcstr              name,
                                     xType              parent,
                                     const xTypeInfo    *info,
                                     xTypeFlags         flags);

void        x_type_module_add_iface (xTypeModule        *module,
                                     xType              type,
                                     xType              iface_type,
                                     const xIFaceInfo   *iface_info);

xTypeModule*x_type_moudle_load      (xcstr              filename);

/* define class */
#define X_DEFINE_DYNAMIC_TYPE(TypeName, type_name, TYPE_PARENT)         \
    X_DEFINE_DYNAMIC_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, 0, ;)

#define X_DEFINE_DYNAMIC_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT,flags, Codes) \
    \
static void type_name##_init            (TypeName           *self);     \
static void type_name##_class_init      (TypeName##Class    *klass);    \
static void type_name##_class_finalize  (TypeName##Class    *klass);    \
static xptr type_name##_parent_class = NULL;                            \
static xType x_##type_name = 0;                                         \
static void type_name##_class_intern_init (TypeName##Class *klass)      \
{                                                                       \
    type_name##_parent_class = x_class_peek_parent (klass);             \
    type_name##_class_init ((TypeName##Class*) klass);                  \
}                                                                       \
xType type_name##_type (void)                                           \
{                                                                       \
    return x_##type_name;                                               \
}                                                                       \
static xType type_name##_register    (xTypeModule    *type_module)      \
{                                                                       \
    xType x_type;                                                       \
    const xTypeInfo type_info = {                                       \
        sizeof (TypeName##Class),                                       \
        NULL,                                                           \
        NULL,                                                           \
        (xClassInitFunc)type_name##_class_intern_init,                  \
        (xClassFinalizeFunc)type_name##_class_finalize,                 \
        NULL,                                                           \
        sizeof(TypeName),                                               \
        (xInstanceInitFunc)type_name##_init,                            \
        NULL                                                            \
    };                                                                  \
    xcstr   name = x_intern_static_str (#TypeName);                     \
    x_type  = x_type_module_register (type_module,                      \
                                      name,                             \
                                      TYPE_PARENT,                      \
                                      &type_info,                       \
                                      (xTypeFlags)flags);               \
    x_##type_name = x_type;                                             \
    { Codes; }                                                          \
    return x_type;                                                      \
}

#define X_IMPLEMENT_IFACE_DYNAMIC(TYPE_IFACE, iface_init) {             \
    const xIFaceInfo iface_info = {                                     \
        (xIFaceInitFunc) iface_init, NULL, NULL                         \
    };                                                                  \
    x_type_module_add_iface (type_module, x_type, TYPE_IFACE, &iface_info); \
}
typedef xbool (*xTypeModuleEntry) (xTypeModule *);
typedef struct _xTypeModulePrivate  xTypeModulePrivate;
struct _xTypeModule 
{
    xObject             parent;
    xTypeModulePrivate* module_private;
};

struct _xTypeModuleClass
{
    xObjectClass        parent;

    /*< protected >*/
    xbool (*load)       (xTypeModule *module);
    void  (*unload)     (xTypeModule *module);
};

X_END_DECLS
#endif /* __X_TYPE_MODULE_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
