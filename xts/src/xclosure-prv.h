#ifndef __X_CLOSURE_PRV_H__
#define __X_CLOSURE_PRV_H__
#include "xclosure.h"
X_BEGIN_DECLS
typedef void (* xVaClosureMarshal) (xClosure *closure,
                                    xValue   *return_value,
                                    xptr  instance,
                                    va_list   args,
                                    xptr  marshal_data,
                                    xsize       n_params,
                                    xType    *param_types);
typedef struct{
    xClosureMarshal meta_marshal;
    xptr meta_marshal_data;
    xVaClosureMarshal va_meta_marshal;
    xVaClosureMarshal va_marshal;
    xClosure closure;
} xRealClosure;

#define X_REAL_CLOSURE(_c)                                              \
    ((xRealClosure *)X_STRUCT_MEMBER_P                                  \
     ((_c), -X_STRUCT_OFFSET (xRealClosure, closure)))

#define	X_CLOSURE_SWAP_DATA(cclosure)	 (((xClosure*) (cclosure))->derivative_flag)
#define	X_CLOSURE_NEEDS_MARSHAL(closure) (((xClosure*) (closure))->marshal == NULL)

X_INTERN_FUNC
void        _x_closure_set_va_marshal (xClosure       *closure,
                                       xVaClosureMarshal marshal);
xbool       _x_closure_is_void      (xClosure       *closure,
                                     xptr           instance);
xbool       _x_closure_supports_invoke_va (xClosure       *closure);

void        _x_cclosure_marshal     (xClosure       *closure,
                                     xValue         *return_xvalue,
                                     xsize          n_param_values,
                                     const xValue   *param_values,
                                     xptr           invocation_hint,
                                     xptr           marshal_data);
void        _x_closure_va_marshal   (xClosure       *closure,
                                     xValue         *return_value,
                                     xptr           instance,
                                     va_list        args_list,
                                     xptr           marshal_data,
                                     xsize          n_params,
                                     xType          *param_types);
void        _x_closure_invoke_va    (xClosure       *closure,
                                     xValue         *return_value,
                                     xptr           instance,
                                     va_list        args,
                                     xsize          n_params,
                                     xType          *param_types);
X_END_DECLS
#endif /* __X_CLOSURE_PRV_H__ */
