#ifndef __X_HOOK_H__
#define __X_HOOK_H__
#include "xtype.h"
X_BEGIN_DECLS

typedef enum {
    X_HOOK_FLAG_ACTIVE	    = 1 << 0,
    X_HOOK_FLAG_IN_CALL	    = 1 << 1,
    X_HOOK_FLAG_MASK	    = 0x0f
} xHookFlagMask;
#define X_HOOK_FLAG_USER_SHIFT	(4)


#define X_HOOK(hook)            ((xHook*) (hook))
#define X_HOOK_FLAGS(hook)      (X_HOOK (hook)->flags)
#define X_HOOK_ACTIVE(hook)     (X_HOOK_FLAGS (hook) & X_HOOK_FLAG_ACTIVE)
#define X_HOOK_IN_CALL(hook)    (X_HOOK_FLAGS (hook) & X_HOOK_FLAG_IN_CALL)

#define X_HOOK_IS_VALID(hook)                                           \
    (X_HOOK (hook)->hook_id != 0 && X_HOOK_ACTIVE(hook))

#define X_HOOK_IS_UNLINKED(hook)                                        \
    (X_HOOK (hook)->next == NULL && \
     X_HOOK (hook)->prev == NULL && \
     X_HOOK (hook)->hook_id == 0 && \
     X_HOOK (hook)->ref_count == 0)

typedef void		(*xHookFinalizeFunc)	(xHookList      *hook_list,
                                             xHook          *hook);

xHookList*  x_hook_list_new                 (xsize          hook_size,
                                             xHookFinalizeFunc finalize);

void        x_hook_list_delete              (xHookList      *hook_list);

xHook*      x_hook_first_valid              (xHookList      *hook_list,
                                             xbool          may_be_in_call);

xHook*      x_hook_next_valid               (xHookList      *hook_list,
                                             xHook          *hook,
                                             xbool          may_be_in_call);

xHook*      x_hook_get                      (xHookList      *hook_list,
                                             xuint          hook_id);

xHook *     x_hook_ref                      (xHookList      *hook_list,
                                             xHook          *hook);

xint        x_hook_unref                    (xHookList      *hook_list,
                                             xHook	        *hook);

void        x_hook_destroy_link             (xHookList      *hook_list,
                                             xHook          *hook);

struct _xHook
{
    xptr        data;
    xHook       *next;
    xHook       *prev;
    xint        ref_count;
    xuint       hook_id;
    xuint       flags;
    xptr        func;
    xFreeFunc   destroy;
};

X_END_DECLS
#endif /* __X_HOOK_H__ */
