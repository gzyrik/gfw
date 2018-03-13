#include <xos/xgc.h>
#include <xos/xprintf.h>
#include <xos/xatomic.h>
#include <xos/xthreadpool.h>
struct test {
    struct test *next;
};

static void
log_ptr(void *p)
{
    x_printf("* free %p\n",p);
}

static struct test *
new_test(struct test *parent)
{
    struct test *ret=(struct test*)
        x_gc_malloc(sizeof(struct test),parent,log_ptr);
    x_printf("* new %p\n",ret);
    if (parent) {
        ret->next=parent->next;
        parent->next=ret;
    }
    else {
        ret->next=0;
    }
    return ret;
}
xint      _level;
static void *
test(xptr weak)
{
    struct test *p;
    int i;

    x_gc_enter(); ++_level;
    x_printf("#%d after enter:\n", _level);
    x_gc_report();

    x_gc_enter(); ++_level;
    x_printf("#%d after enter:\n", _level);
    x_gc_report();

    for (i=0;i<4;i++) {
        p=new_test(0);
        x_gc_link(weak,0,p);
        x_printf("#%d after link to weak: %p is linked to weak %p\n", _level, p, weak);
        x_gc_report();
    }

    x_gc_leave(p,NULL); --_level;
    x_printf("#%d after leave: only %p leave in stack\n", _level, p);
    x_gc_report();

    x_gc_collect();
    x_printf("#%d after collected: %p will not be collected\n", _level, p);
    x_gc_report();

    p->next=new_test(0);

    /* one node can be linked to parent more than once */
    x_gc_link(p,0,p->next);
    x_printf("#%d after link: %p is linked to %p\n", _level, p->next, p);
    x_gc_report();

    x_gc_link(p,p->next,0);
    x_printf("#%d after unlink: %p is unlinked to %p\n",_level, p->next, p);
    x_gc_report();

    x_gc_leave(p->next,0); --_level;
    x_printf("#%d after leave: only %p leave in stack, link to global\n",_level, p->next);
    x_gc_report();

    x_gc_link(weak,0,p->next);
    x_printf("#%d after link to weak: %p is linked to weak table %p\n", _level, p->next, weak);
    x_gc_report();

    return p->next;
}

static void
iterate_weak_table(xptr weak)
{
    int iter=0;
    void *p;

    x_gc_enter(); ++_level;
    x_printf("#%d after enter: to iterate weak table\n", _level);
    x_gc_report();

    while ((p=x_gc_weak_next(weak,&iter)) != 0) {
        x_printf("* %p is alive\n",p);
    }
    x_gc_leave(NULL); --_level;
    x_printf("#%d after leave: from iterate weak table\n", _level);
    x_gc_report();
}
static xint _count = 2;
void gc_link_test (xptr data1, xptr data2)
{
    struct test *p;
    xint id = X_PTR_TO_UINT(data1);
    xint level=0;

    x_gc_report("$%d-#%d start:\n", id, level);

    x_gc_enter(); ++level;
    x_gc_report("$%d-#%d after enter:\n", id, level);

    p=new_test(0);
    x_gc_report("$%d-#%d after malloc: %p auto link to stack %d\n",id, level, p,level);


    x_gc_link (0, 0, p);
    x_gc_report("$%d-#%d after link: %p link to global\n",id, level, p);

    x_gc_link (0, 0, p);
    x_gc_report("$%d-#%d after link twice: %p link to global\n",id, level, p);

    x_gc_link (0, p, 0);
    x_gc_report("$%d-#%d after unlink: %p unlink from global\n",id, level, p);

    x_gc_link (0, p, 0);
    x_gc_report("$%d-#%d after unlink twice: %p unlink from global\n",id, level, p);

    x_gc_leave(0);--level;
    x_gc_report("$%d-#%d after leave:\n",id, level);

    x_gc_collect();
    x_gc_report("$%d-#%d after collect:\n",id, level);

    --_count;
}
void gc_thread_test ()
{
    xint i,n=_count;
    xThreadPool *pool = x_thread_pool_new (X_CALLBACK(gc_link_test),
        "title",
        -1,
        FALSE,
        NULL);
    for(i=1;i<=n;++i)
        x_thread_pool_push(pool, X_UINT_TO_PTR(i), NULL);
    while(x_atomic_int_get(&_count) > 0)
        x_usleep(10000);
}
void     
gc_test()
{
    struct test *p;
    xptr weak;

    x_printf("#%d gc_test start:\n", _level);
    x_gc_report();

    weak=x_gc_weak_table (0);
    x_printf("#%d create weak table: %p linked to global\n", _level, weak);
    x_gc_report();

    p=test(weak);
    x_gc_link(weak,p,0);
    x_printf("#%d after unlink: %p unlink from weak %p\n",_level, p, weak);
    x_gc_report();

    x_gc_enter(); ++_level;
    x_printf("#%d after enter:\n", _level);
    x_gc_report();

    iterate_weak_table(weak);

    x_gc_leave(0);--_level;
    x_printf("#%d after leave:\n",_level);
    x_gc_report();

    x_gc_collect();
    x_printf("#%d after collect:\n",_level);
    x_gc_report();

    iterate_weak_table(weak);

    x_gc_clear ();
}
