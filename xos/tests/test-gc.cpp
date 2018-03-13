#include "add-test.h"
static xsize _freed_count = 0;
static void log(xptr p)
{
    _freed_count++;
}
static void test_wtable(void)
{
    xGC *gc;
    xptr p[6],wp;
    xptr wtable;
    xsize i;

    gc = x_gc_new();
    wtable = x_gc_wtable_new(gc, NULL);
    _freed_count = 0;
    {
        x_gc_enter(gc);

        for(i=0;i<3;++i)
            p[i] = x_gc_alloc(gc, 4, wtable, X_VOID_FUNC(log));
        {
            x_gc_enter(gc);
            for(i=3;i<6;++i)
                p[i] = x_gc_alloc(gc, 4, wtable, X_VOID_FUNC(log));
            x_gc_leave(gc, p[3], NULL);
            x_gc_collect(gc);
        }
        i=0;
        wp = x_gc_wtable_next(gc,wtable, &i);
        while (wp) {
            x_assert(wp == p[i-1]);
            wp = x_gc_wtable_next(gc,wtable, &i);
        }
        x_assert_int(i, ==, 4);
        x_gc_leave(gc, NULL);
        x_gc_collect(gc);
    }

    i = 0;
    wp = x_gc_wtable_next(gc,wtable, &i);
    x_assert(wp == NULL);

    x_assert_int (_freed_count, == , 6);

    x_gc_delete(gc);
}
static void thread_gc (xuint thread_id, xGC* gc)
{
    xptr p[6],wp;
    xsize i;
    xptr wtable;

    wtable = x_gc_wtable_new(gc, NULL);
    x_gc_enter(gc);
    for(i=0;i<6;++i)
        p[i] = x_gc_alloc(gc, 4, wtable, X_VOID_FUNC(log));
    x_gc_leave(gc, NULL);

    wp = x_gc_wtable_next(gc,wtable, &i);
    while (wp) {
        x_assert(wp == p[i-1]);
        wp = x_gc_wtable_next(gc,wtable, &i);
    }
    x_assert_int(i, ==, 6);
}
static void test_thread ()
{
    xGC *gc;
    xThreadPool *pool;
    xint i;
    gc = x_gc_new();

    _freed_count = 0;
    pool = x_thread_pool_new (X_VOID_FUNC(thread_gc), gc);
    for(i=0;i<10;++i)
        x_thread_pool_push(pool, X_SIZE_TO_PTR(i|2<<8), 0);
    x_thread_pool_delete(pool, FALSE, TRUE);
    x_gc_collect(gc);
    x_gc_delete (gc);
    x_assert_int (_freed_count, == , 6*10);
}
ADD_TEST("/xos/gc/thread", test_thread);
ADD_TEST("/xos/gc/wtable", test_wtable);
