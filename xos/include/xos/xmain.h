#ifndef __X_MAIN_H__
#define __X_MAIN_H__
#include "xtype.h"
X_BEGIN_DECLS
#define X_PRIORITY_DEFAULT          0
#define X_PRIORITY_DEFAULT_IDLE     200

xMainContext*   x_main_context_new      (void);

xMainContext*   x_main_context_ref      (xMainContext   *context);

xint            x_main_context_unref    (xMainContext   *context);

xMainContext*   x_main_context_default  (void);


xMainLoop*      x_main_loop_new         (xMainContext   *context,
                                         xbool          running);

void            x_main_loop_run         (xMainLoop      *loop);

void            x_main_loop_quit        (xMainLoop      *loop);

xMainLoop*      x_main_loop_ref         (xMainLoop      *loop);

xint            x_main_loop_unref       (xMainLoop      *loop);

xMainContext*   x_main_loop_context     (xMainLoop      *loop);

xSource*        x_source_new            (xSourceFuncs   *source_funcs,
                                         xsize          struct_size);

void            x_source_unref          (xSource        *source);

xuint           x_source_attach         (xSource        *source,
                                         xMainContext   *context);

void            x_source_set_priority   (xSource        *source,
                                         xint           priority);

void            x_source_set_callback   (xSource        *source,
                                         xptr           data,
                                         xSourceCallback*callback);

void            x_source_set_func       (xSource        *source,
                                         xSourceFunc    func,
                                         xptr           data,
                                         xFreeFunc      notify);

xuint           x_idle_add_full         (xuint          priority,
                                         xSourceFunc    source_func,
                                         xptr           user_data,
                                         xFreeFunc      free_func);

#define         x_idle_add(source_func, user_data)                      \
    x_idle_add_full (0, source_func, user_data, NULL)

xuint           x_timeout_add_full      (xuint          priority,
                                         xuint          interval,
                                         xSourceFunc    source_func,
                                         xptr           user_data,
                                         xFreeFunc      free_func);

#define         x_timeout_add(intervalMs, source_func, user_data)           \
    x_timeout_add_full (0, intervalMs, (xSourceFunc)source_func, user_data, NULL)


xbool           x_source_remove         (xuint          id);

typedef struct _xSourcePrivate      xSourcePrivate;
struct _xSource
{
    /*< private >*/
    xptr callback_data;
    xSourceCallback *callback_funcs;

    xSourceFuncs *source_funcs;
    xint ref_count;

    xMainContext *context;

    xint priority;
    xuint flags;
    xuint source_id;
    xchar    *name;
    xSourcePrivate *priv;
};
struct _xSourceCallback
{
    void (*ref)   (xptr     cb_data);
    void (*unref) (xptr     cb_data);
    void (*get)   (xptr     cb_data,
                   xSource     *source, 
                   xSourceFunc *func,
                   xptr        *data);
};

struct _xSourceFuncs
{
    xbool (*prepare)  (xSource *source, xint *timeout);
    xbool (*check)    (xSource *source);
    xbool (*dispatch) (xSource *source, xSourceFunc cb, xptr user_data);
    void  (*finalize) (xSource *source); /* can be NULL */

    /*< private >*/
    /* For use by x_source_set_closure */
    xSourceFunc closure_callback;
    void (*closure_marshal)(); /* Really is of type GClosureMarshal */
};

X_END_DECLS
#endif /* __X_MAIN_H__ */
