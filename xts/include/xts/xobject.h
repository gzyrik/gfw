#ifndef __X_OBJECT_H__
#define __X_OBJECT_H__
#include "xclass.h"
X_BEGIN_DECLS

#define X_OBJECT(object)            (X_INSTANCE_CAST ((object), X_TYPE_OBJECT, xObject))
#define X_OBJECT_CLASS(oclass)      (X_CLASS_CAST ((oclass), X_TYPE_OBJECT, xObjectClass))
#define X_IS_OBJECT(object)         (X_INSTANCE_IS_TYPE ((object), X_TYPE_OBJECT))
#define X_IS_OBJECT_CLASS(oclass)   (X_CLASS_IS_TYPE ((oclass), X_TYPE_OBJECT))
#define X_OBJECT_GET_CLASS(object)  (X_INSTANCE_GET_CLASS ((object), X_TYPE_OBJECT, xObjectClass))
#define X_OBJECT_TYPE(object)       (X_INSTANCE_TYPE (object))
#define X_OBJECT_TYPE_NAME(object)  (X_INSTANCE_TYPE_NAME (object))
#define X_OBJECT_CLASS_NAME(oclass) (X_CLASS_NAME (oclass))
#define X_OBJECT_CLASS_TYPE(oclass) (X_CLASS_TYPE(oclass))

xptr        x_object_new_json       (xType          type,
                                     xcstr          json);

xptr        x_object_new            (xType          type,
                                     xcstr          first_property,
                                     ...);

xptr        x_object_new_valist     (xType          type,
                                     xcstr          first_property,
                                     va_list        argv);

xptr        x_object_new_valist1    (xType          type,
                                     xcstr          first_property,
                                     va_list        argv1,
                                     xcstr          other_property,
                                     ...);

xptr        x_object_new_valist2    (xType          type,
                                     xcstr          first_property,
                                     va_list        argv1,
                                     xcstr          other_property,
                                     va_list        argv2);

xptr        x_object_newv           (xType          type,
                                     xsize          n_parameters,
                                     xParamValue    *parameters);

xbool       x_object_floating       (xptr           object);

void        x_object_unsink         (xptr           object);

xptr        x_object_sink           (xptr           object);

xptr        x_object_ref            (xptr           object);

xint        x_object_unref          (xptr           object);

xptr        x_object_weak_ref       (xptr           object,
                                     xCallback      callback,
                                     xptr           user);

void        x_object_weak_unref     (xptr           object,
                                     xCallback      callback,
                                     xptr           user);

void        x_object_add_weakptr    (xptr           object, 
                                     xptr           *weakptr_location);

void        x_object_remove_weakptr (xptr           object, 
                                     xptr           *weakptr_location);

void        x_object_dispose        (xObject        object);

void        x_oclass_install_params (xptr           oclass,
                                     xsize          n_pspecs,
                                     xParam         **pspecs);

void        x_oclass_install_param  (xptr           oclass,
                                     xuint          param_id,
                                     xParam         *pspec);

void        x_iface_install_param   (xptr           iface,
                                     xParam         *pspec);

void        x_oclass_override_param (xptr           oclass,
                                     xuint          param_id,
                                     xcstr          property);

void        x_object_get            (xptr           object,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

void        x_object_set            (xptr           object,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

void        x_object_get_valist     (xptr           object,
                                     xcstr          first_property,
                                     va_list        args);

void        x_object_set_valist     (xptr           object,
                                     xcstr          first_property,
                                     va_list        args);

void        x_object_set_str        (xptr           object,
                                     xcstr          property,
                                     xcstr          value);

void        x_object_get_str        (xptr           object,
                                     xcstr          property,
                                     xstr           *value);

void        x_object_get_value      (xptr           object,
                                     xcstr          property,
                                     xValue         *value);

void        x_object_set_value      (xptr           object,
                                     xcstr          property,
                                     const xValue   *value);

xptr        x_object_get_qdata      (xptr           object,
                                     xQuark         key);

void        x_object_set_qdata_full (xptr           object,
                                     xQuark         key,
                                     xptr           val,
                                     xFreeFunc      free_func);

void        x_object_notify         (xptr           object,
                                     xcstr          property_name);

void        x_object_notify_param   (xptr           object,
                                     xParam         *pspec);

struct _xConstructParam
{
    xParam                  *pspec;
    xValue                  *value;
};
/**
 * xObject:
 * 
 * All the fields in the <structname>xObject</structname> structure are private 
 * to the #xObject implementation and should never be accessed directly.
 */
struct _xObject
{
    xInstance               parent;
    /*< private >*/
    xint                    ref_count;
    xDataList               *qdata;
};

struct _xObjectClass
{
    xClass                  parent;
    xuint                   flags;
    xSList                  *construct_properties;
    xptr    (*constructor)  (xType              type,
                             xsize              n_param,
                             xConstructParam    *param);
    void    (*set_property) (xObject            *object,
                             xuint              property_id,
                             xValue             *value,
                             xParam             *pspec);
    void    (*get_property) (xObject            *object,
                             xuint              property_id,
                             xValue             *value,
                             xParam             *pspec);
    void    (*dispose)      (xObject            *object);
    void    (*finalize)     (xObject            *object);
    void    (*dispatch_properties_changed)     (xObject            *object,
                                                xsize              n_pspecs,
                                                xParam             **pspecs);
    void    (*notify)       (xObject            *object,
                             xParam             *pspec);
    void    (*constructed)  (xObject            *object);
};

X_END_DECLS
#endif /* __X_OBJECT_H__ */
