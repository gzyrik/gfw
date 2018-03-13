#ifndef __X_WAKEUP_H__
#define __X_WAKEUP_H__
#include "xtype.h"
X_BEGIN_DECLS
xWakeup*    x_wakeup_new            (void);

void        x_wakeup_delete         (xWakeup        *wakeup);

void        x_wakeup_signal         (xWakeup        *wakeup);

void        x_wakeup_ack            (xWakeup        *wakeup);

void        x_wakeup_get_pollfd     (xWakeup        *wakeup,
                                     xPollFd        *poll_fd);
X_END_DECLS
#endif /*__X_WAKEUP_H__ */

