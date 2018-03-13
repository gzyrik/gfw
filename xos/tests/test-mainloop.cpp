#include <xos.h>
#include "add-test.h"
static xbool
timeout (xptr obj[2])
{
    x_timer_stop((xTimer*)obj[1]);
    x_main_loop_quit((xMainLoop*)obj[0]);
    return FALSE;
}
static void mainloop_timeout_test()
{
    xMainLoop* loop = x_main_loop_new(x_main_context_default(), FALSE);
    xint64 deltaMs;
    xTimer timer;
    xptr obj[2]={loop, &timer};
    x_timeout_add (100, timeout, obj);
    x_timer_start(&timer);
    x_main_loop_run(loop);
    
    deltaMs = x_timer_elapsed(&timer)/1000;
    x_assert_int(deltaMs, <= ,110);
    x_assert(x_main_loop_unref (loop) == 0);
}
ADD_TEST("/xos/mainloop/timeout", mainloop_timeout_test, "Timeout");