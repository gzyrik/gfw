#ifndef __X_TIME_H__
#define __X_TIME_H__
#include "xtype.h"
X_BEGIN_DECLS

struct _xTimeVal
{
    xlong           sec;
    xlong           usec;
};
struct _xTimer
{
    xint64          start;
    xint64          end;
};
void        x_usleep                (xulong         microseconds);

void        x_ctime                 (xTimeVal       *result);
/* system wall-clock microseconds */
xint64      x_rtime                 (void);
/* system monotonic microseconds */
xint64      x_mtime                 (void);

xcstr       x_getenv                (xcstr          name);

void        x_putenv                (xcstr          exp);

xcstr       x_get_prgname           (void);

void        x_set_prgname           (xcstr          prgname);

xcstr       x_get_charset           (void);

void        x_set_charset           (xcstr          charset);

#define     x_timer_start(timer)    ((timer)->start = x_mtime())

#define     x_timer_stop(timer)     ((timer)->end = x_mtime())

#define     x_timer_elapsed(timer)  ((timer)->end - (timer)->start)

#define     x_timer_continue(timer) ((timer)->start =                   \
                                     x_mtime() - x_timer_elapsed())
X_END_DECLS
#endif /* __X_TIME_H__ */
