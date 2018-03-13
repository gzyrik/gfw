#include "config.h"
#include "xhook.h"
#include "xmsg.h"
#include "xslice.h"
#include "xatomic.h"
struct _xHookList
{
    xuint	            seq_id;
    xsize		        hook_size;
    xHookFinalizeFunc   hook_finalize;
    xHook		        *hooks;
};
static void
hook_finalize           (xHookList      *hook_list,
                         xHook          *hook)
{
    xFreeFunc destroy = hook->destroy;

    if (destroy) {
        hook->destroy = NULL;
        destroy (hook->data);
    }
}

xHookList*
x_hook_list_new                 (xsize          hook_size,
                                 xHookFinalizeFunc finalize)
{
    xHookList   *hook_list;
    hook_list = x_slice_new (xHookList);
    hook_list->seq_id = 1;
    hook_list->hook_size = hook_size;
    hook_list->hooks = NULL;
    if(finalize != NULL)
        hook_list->hook_finalize = finalize; 
    else
        hook_list->hook_finalize = hook_finalize;
    return hook_list;
}
void
x_hook_list_delete              (xHookList      *hook_list)
{
    xHook *hook;
    x_return_if_fail (hook_list != NULL);

    hook = hook_list->hooks;
    while(hook) {
        xHook *tmp;
        x_hook_ref (hook_list, hook);
        x_hook_destroy_link (hook_list, hook);
        tmp = hook->next;
        x_hook_unref (hook_list, hook);
        hook = tmp;
    }
}
xHook*
x_hook_first_valid              (xHookList      *hook_list,
                                 xbool          may_be_in_call)
{
    xHook *hook;
    x_return_val_if_fail (hook_list != NULL, NULL);


    hook = hook_list->hooks;
    if (hook) {
        hook = x_hook_ref (hook_list, hook);
        if (X_HOOK_IS_VALID (hook)
            && (may_be_in_call || !X_HOOK_IN_CALL (hook)))
            return hook;
        else
            return x_hook_next_valid (hook_list, hook, may_be_in_call);
    }
    return NULL;
}

xHook*
x_hook_next_valid               (xHookList      *hook_list,
                                 xHook          *hook,
                                 xbool          may_be_in_call)
{
    xHook *ohook = hook;

    x_return_val_if_fail (hook_list != NULL, NULL);

    if (!hook)
        return NULL;

    hook = hook->next;
    while (hook) {
        if (X_HOOK_IS_VALID (hook)
            && (may_be_in_call || !X_HOOK_IN_CALL (hook))) {
            x_hook_ref (hook_list, hook);
            x_hook_unref (hook_list, ohook);

            return hook;
        }
        hook = hook->next;
    }
    x_hook_unref (hook_list, ohook);

    return NULL;
}

xHook*
x_hook_get                      (xHookList      *hook_list,
                                 xuint          hook_id)
{
    xHook *hook;

    x_return_val_if_fail (hook_list != NULL, NULL);
    x_return_val_if_fail (hook_id > 0, NULL);

    hook = hook_list->hooks;
    while (hook) {
        if (hook->hook_id == hook_id)
            return hook;
        hook = hook->next;
    }

    return NULL;
}
xHook*
x_hook_ref                      (xHookList      *hook_list,
                                 xHook          *hook)
{
    x_return_val_if_fail (hook_list != NULL, NULL);
    x_return_val_if_fail (hook != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&hook->ref_count) > 0, NULL);

    hook->ref_count++;

    return hook;
}
xint
x_hook_unref                    (xHookList      *hook_list,
                                 xHook	        *hook)
{
    xint ref_count;

    x_return_val_if_fail (hook_list != NULL, -1);
    x_return_val_if_fail (hook != NULL,-1);
    x_return_val_if_fail (x_atomic_int_get (&hook->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&hook->ref_count);
    if X_LIKELY(ref_count != 0)
        return ref_count;

    x_return_val_if_fail (hook->hook_id == 0, -1);
    x_return_val_if_fail (!X_HOOK_IN_CALL (hook), -1);

    if (hook->prev)
        hook->prev->next = hook->next;
    else
        hook_list->hooks = hook->next;
    if (hook->next) {
        hook->next->prev = hook->prev;
        hook->next = NULL;
    }
    hook->prev = NULL;

    if(hook_list->hook_finalize != NULL)
        hook_list->hook_finalize (hook_list, hook);
    x_slice_free1 (hook_list->hook_size, hook);

    return ref_count;
}
void
x_hook_destroy_link             (xHookList      *hook_list,
                                 xHook          *hook)
{
    x_return_if_fail (hook_list != NULL);
    x_return_if_fail (hook != NULL);

    hook->flags &= ~X_HOOK_FLAG_ACTIVE;
    if (hook->hook_id) {
        hook->hook_id = 0;
        x_hook_unref (hook_list, hook);
    }
}
