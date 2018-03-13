#ifndef __X_CLASS_PRV_H__
#define __X_CLASS_PRV_H__
#include "xclass.h"
X_BEGIN_DECLS

X_INTERN_FUNC
void        _x_object_type_init     (void);

X_INTERN_FUNC
void        _x_value_types_init     (void);

X_INTERN_FUNC
void        _x_boxed_types_init     (void);

X_INTERN_FUNC
void        _x_param_types_init     (void);

X_INTERN_FUNC
void        _x_value_transforms_init(void);

X_INTERN_FUNC
void        _x_type_boxed_init      (xType          type,
                                     xDupFunc       copy_func,
                                     xFreeFunc      free_func);
X_INTERN_FUNC
void        _x_signal_init          (void);

X_INTERN_FUNC
xptr        _x_type_boxed_copy      (xType          type,
                                     xptr           boxed);

X_INTERN_FUNC
void        _x_type_boxed_free      (xType          type,
                                     xptr           boxed);

X_INTERN_FUNC
void        _x_signals_destroy      (xType          type);


X_END_DECLS
#endif /* __X_CLASS_PRV_H__ */
