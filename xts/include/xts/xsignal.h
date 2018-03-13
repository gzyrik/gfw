#ifndef __X_SIGNAL_H__
#define __X_SIGNAL_H__
#include <stdarg.h>
#include "xclosure.h"
X_BEGIN_DECLS

typedef enum {
    X_SIGNAL_RUN_FIRST      = 1 << 0,
    X_SIGNAL_RUN_LAST       = 1 << 1,
    X_SIGNAL_RUN_CLEANUP    = 1 << 2,
    X_SIGNAL_NO_RECURSE     = 1 << 3,
    X_SIGNAL_DETAILED       = 1 << 4,
    X_SIGNAL_ACTION         = 1 << 5,
    X_SIGNAL_NO_HOOKS       = 1 << 6,
    X_SIGNAL_MUST_COLLECT   = 1 << 7,
    X_SIGNAL_DEPRECATED     = 1 << 8
} xSignalFlags;

#define X_SIGNAL_FLAGS_MASK             0x1ff
#define	X_SIGNAL_TYPE_STATIC_SCOPE      1

typedef enum {
    X_SIGNAL_CONNECT_AFTER	= 1 << 0,
    X_SIGNAL_CONNECT_SWAPPED= 1 << 1
} xSignalConnectFlags;

typedef enum {
    X_SIGNAL_MATCH_ID	    = 1 << 0,
    X_SIGNAL_MATCH_DETAIL   = 1 << 1,
    X_SIGNAL_MATCH_CLOSURE  = 1 << 2,
    X_SIGNAL_MATCH_FUNC	    = 1 << 3,
    X_SIGNAL_MATCH_DATA	    = 1 << 4,
    X_SIGNAL_MATCH_UNBLOCKED= 1 << 5,
    X_SIGNAL_MATCH_BLOCKED  = 1 << 6
} xSignalMatchType;

#define X_SIGNAL_MATCH_MASK  0x7f

typedef xbool (*xSignalAccumulator) (xSignalInvocationHint *ihint,
                                     xValue         *return_accu,
                                     const xValue   *handler_return,
                                     xptr           data);
typedef xbool (*xSignalEmissionHook) (xSignalInvocationHint *ihint,
                                      xsize			n_param_values,
                                      const xValue	       *param_values,
                                      xptr		data);

xuint       x_signal_new            (xcstr          signal_name,
                                     xType          itype,
                                     xSignalFlags   signal_flags,
                                     xuint          class_offset,
                                     xSignalAccumulator  accumulator,
                                     xptr           accu_data,
                                     xType          return_type,
                                     xuint          n_params,
                                     ...);

xuint       x_signal_new_valist     (xcstr          signal_name,
                                     xType          itype,
                                     xSignalFlags   signal_flags,
                                     xClosure       *class_closure,
                                     xSignalAccumulator accumulator,
                                     xptr           accu_data,
                                     xType          return_type,
                                     xuint          n_params,
                                     va_list        args);

xuint       x_signal_newv           (xcstr          signal_name,
                                     xType          itype,
                                     xSignalFlags   signal_flags,
                                     xClosure       *class_closure,
                                     xSignalAccumulator accumulator,
                                     xptr           accu_data,
                                     xType          return_type,
                                     xuint          n_params,
                                     xType          *param_types);

void        x_signal_clean          (xptr           instance);

xuint       x_signal_connect_data   (xptr           instance,
                                     xcstr          detailed_signal,
                                     xCallback      c_handler,
                                     xptr           data,
                                     xNotify        destroy_data,
                                     xuint          connect_flags);

#define x_signal_connect(instance, detailed_signal, c_handler, data)    \
    x_signal_connect_data ((instance), (detailed_signal), (c_handler),  \
                           (data), NULL, 0)

#define x_signal_connect_after(instance, detailed_signal, c_handler, data) \
    x_signal_connect_data ((instance), (detailed_signal), (c_handler),  \
                           (data), NULL, X_SIGNAL_CONNECT_AFTER)

#define x_signal_connect_swapped(instance, detailed_signal, c_handler, data) \
    x_signal_connect_data ((instance), (detailed_signal), (c_handler),  \
                           (data), NULL, X_SIGNAL_CONNECT_SWAPPED)

void         x_signal_emit          (xptr           instance,
                                     xuint          signal_id,
                                     xQuark         detail,
                                     ...);

void        x_signal_emit_by_name   (xptr           instance,
                                     xcstr          detailed_signal,
                                     ...);

void        x_signal_emit_valist    (xptr           instance,
                                     xuint          signal_id,
                                     xQuark         detail,
                                     va_list        var_args);

void        x_signal_disconnect     (xptr           instance,
                                     xulong         handler_id);

xsize       x_signal_disconnect_matched (xptr       instance,
                                         xuint      match_mask,
                                         xuint      signal_id,
                                         xQuark     detail,
                                         xClosure   *closure,
                                         xptr       func,
                                         xptr       data);

#define     x_signal_disconnect_func(instance, func, data)              \
    x_signal_disconnect_matched (instance,                              \
                                 X_SIGNAL_MATCH_FUNC|X_SIGNAL_MATCH_DATA,\
                                 0, 0, NULL, func, data)

void        x_signal_block          (xptr           instance,
                                     xulong         handler_id);

void        x_signal_unblock        (xptr           instance,
                                     xulong         handler_id);

xsize       x_signal_block_matched  (xptr           instance,
                                     xuint          match_mask,
                                     xuint          signal_id,
                                     xQuark         detail,
                                     xClosure       *closure,
                                     xptr           func,
                                     xptr           data);
#define     x_signal_block_func(instance, func, data)                   \
    x_signal_block_matched (instance,                                   \
                            X_SIGNAL_MATCH_FUNC|X_SIGNAL_MATCH_DATA,    \
                            0, 0, NULL, func, data)

xsize       x_signal_unblock_matched(xptr           instance,
                                     xuint          match_mask,
                                     xuint          signal_id,
                                     xQuark         detail,
                                     xClosure       *closure,
                                     xptr           func,
                                     xptr           data);

#define     x_signal_unblock_func(instance, func, data)                 \
    x_signal_unblock_matched (instance,                                 \
                              X_SIGNAL_MATCH_FUNC|X_SIGNAL_MATCH_DATA,    \
                              0, 0, NULL, func, data)

xbool       x_singal_accumulator_or (xSignalInvocationHint *ihint,
                                     xValue                *return_accu,
                                     const xValue          *handler_return,
                                     xptr                   dummy);
struct _xSignalInvocationHint
{
    xuint           signal_id;
    xQuark          detail;
    xSignalFlags    run_type;
};

X_END_DECLS
#endif /* __X_SIGNAL_H__ */
