#ifndef __X_SIGNAL_H__
#define __X_SIGNAL_H__
#include <stdarg.h>
#include "xtype.h"
X_BEGIN_DECLS
v

#define X_SIGNAL_INT            "i"
#define X_SIGNAL_LONG           "l"
#define X_SIGNAL_INT64          "q"
#define X_SIGNAL_DOUBLE         "d"
#define X_SIGNAL_PTR            "p"
typedef enum {
    X_CON_AFTER             = 1 << 0,
    X_CON_SWAPPED           = 1 << 1,
    X_SIG_FIRST             = 1 << 2,
    X_SIG_LAST              = 1 << 3,
    X_SIG_CLEAN             = 1 << 4
} xSigConFlg;

typedef struct _xClosure    xClosure;
typedef void (*xMarshal)    (xptr inst, xCallback fun, xint *ret,
                             va_list argv, xptr data, xbool swap);

struct _xSignal
{
    xcstr               name;
    xcstr               format;
    xsize               offset;
    xuint               flags;
    xMarshal            marshal;
    xType               owner_type;
};
xuint       x_signal_new            (xcstr          signal_name,
                                     xType          itype,
                                     xSignalFlags   signal_flags,
                                     xuint          class_offset,
                                     xSignalAccumulator	 accumulator,
                                     xptr		    accu_data,
                                     xSignalCMarshaller  c_marshaller,
                                     xType          return_type,
                                     xuint          n_params,
                                     ...);

xuint       x_signal_new_valist     (xcstr          signal_name,
                                     xType          itype,
                                     xSignalFlags   signal_flags,
                                     xClosure       *class_closure,
                                     xSignalAccumulator accumulator,
                                     xptr		    accu_data,
                                     xSignalCMarshaller c_marshaller,
                                     xType          return_type,
                                     xuint          n_params,
                                     va_list        args);

xuint       x_signal_newv           (xcstr          signal_name,
                                     xType          itype,
                                     xSignalFlags   signal_flags,
                                     xClosure       *class_closure,
                                     xSignalAccumulator accumulator,
                                     xptr		    accu_data,
                                     xSignalCMarshaller c_marshaller,
                                     xType		    return_type,
                                     xuint          n_params,
                                     xType		    *param_types);

oid         x_signal_emit           (xptr           instance,
                                     xuint          signal_id,
                                     xQuark         detail,
                                     ...);

void        x_signal_emit_by_name   (xptr           instance,
                                     xcstr          detailed_signal,
                                     ...);


void        x_signal_insert         (xType          owner_type,
                                     xSignal        *signals,
                                     xuint          n_signals);

void        x_signal_remove         (xSignal        *signals,
                                     xuint          n_signals);

xSignal*    x_signal_lookup         (xcstr          detail_signal,
                                     xType          owner_type,
                                     xcstr          *detail,
                                     xbool          walk_ancestors);

xClosure*   x_signal_connect_full   (xInstance      *instance,
                                     xSignal        *signal,
                                     xQuark         detail,
                                     xCallback      callback,
                                     xptr           data,
                                     xFreeFunc      destroy_data,
                                     xuint          signal_flags);

xClosure*   x_signal_connect_data   (xInstance      *instance,
                                     xcstr          detail_signal,
                                     xCallback      callback,
                                     xptr           data,
                                     xFreeFunc      destroy_data,
                                     xuint          signal_flags);


#define     x_signal_connect_after(instance, detail_signal, callback, data) \
    x_signal_connect_data   (X_INSTANCE(instance), detail_signal,           \
                             callback, data, NULL, X_CON_AFTER)

#define     x_signal_connect(instance, detail_signal, callback, data)       \
    x_signal_connect_data   (X_INSTANCE(instance), detail_signal,           \
                             callback, data, NULL, 0) 

xint        x_signal_emit_full      (xInstance      *instance,
                                     xSignal        *signal,
                                     xQuark         detail,
                                     ...);

xint        x_signal_emit_valist    (xInstance      *instance,
                                     xSignal        *signal,
                                     xQuark         detail,
                                     va_list        argv);

xint        x_signal_emit           (xInstance      *instance,
                                     xcstr          detail_signal,
                                     ...);

void        x_signal_clean          (xInstance      *instance);

xint        x_closure_block         (xClosure       *closure);

xint        x_closure_unblock       (xClosure       *closure);

xint        x_closure_delete        (xClosure       *closure);

X_END_DECLS
#endif /* __X_SIGNAL_H__ */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
