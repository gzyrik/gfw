#ifndef __X_CLOSURE_H__
#define __X_CLOSURE_H__
#include "xtype.h"
X_BEGIN_DECLS
typedef void  (*xClosureMarshal)	(xClosure	    *closure,
                                     xValue/*out*/  *return_value,
                                     xsize          n_param_values,
                                     const xValue   *param_values,
                                     xptr           invocation_hint,
                                     xptr	        marshal_data);


xClosure*   x_cclosure_new          (xCallback      callback_func,
                                     xptr           user_data,
                                     xNotify        destroy_data);

xClosure*   x_cclosure_new_swap     (xCallback      callback_func,
                                     xptr           user_data,
                                     xNotify        destroy_data);

xClosure*   x_type_cclosure_new     (xType          type,
                                     xlong          struct_offset);

xClosure*   x_closure_new_simple    (xsize          sizeof_closure,
                                     xptr           data);

void        x_closure_sink          (xClosure       *closure);

xClosure*   x_closure_ref           (xClosure       *closure);

xint        x_closure_unref         (xClosure       *closure);

void        x_closure_invalidate    (xClosure       *closure);

void	    x_closure_set_marshal   (xClosure	    *closure,
                                     xClosureMarshal marshal);

void        x_closure_set_meta_marshal  (xClosure        *closure,
                                         xptr            marshal_data,
                                         xClosureMarshal meta_marshal);

void	    x_closure_on_finalize   (xClosure       *closure,
                                     xptr	        user_data,
                                     xNotify	    destroy_data);

void	    x_closure_off_finalize  (xClosure       *closure,
                                     xptr	        user_data,
                                     xNotify	    destroy_data);

void	    x_closure_on_invalidate (xClosure       *closure,
                                     xptr	        user_data,
                                     xNotify	    destroy_data);

void	    x_closure_off_invalidate(xClosure       *closure,
                                     xptr	        user_data,
                                     xNotify	    destroy_data);

void        x_closure_add_guards    (xClosure       *closure,
                                     xptr           pre_guard_data,
                                     xNotify        pre_guard,
                                     xptr           post_guard_data,
                                     xNotify        post_guard);

void        x_closure_invoke        (xClosure       *closure,
                                     xValue         *return_value,
                                     xsize          n_param_values,
                                     const xValue   *param_values,
                                     xptr           invocation_hint);

struct _xClosure
{
    /*< private >*/
    volatile    xint        ref_count : 15;
    volatile    xuint       meta_marshal_nouse : 1;
    volatile    xuint       n_guards : 1;
    volatile    xuint       n_fnotifiers : 2;  /* finalization notifiers */
    volatile    xuint       n_inotifiers : 8;  /* invalidation notifiers */
    volatile    xuint       in_inotify : 1;
    volatile    xuint       floating : 1;
    /*< protected >*/
    volatile    xuint       derivative_flag : 1;
    /*< public >*/
    volatile    xuint       in_marshal : 1;
    volatile    xuint       is_invalid : 1;

    /*< private >*/
    xClosureMarshal         marshal;

    /*< protected >*/
    xptr data;

    /*< private >*/
    xClosureNotifyData *notifiers;
};
struct _xClosureNotifyData
{
    xptr            data;
    xNotify         notify;
};
struct _xCClosure
{
    xClosure        closure;
    xCallback       callback;
};

X_END_DECLS
#endif /* __X_CLOSURE_H__ */
