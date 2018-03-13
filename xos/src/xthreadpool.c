#include "config.h"
#include "xasyncqueue.h"
#include "xthreadpool.h"
#include "xmsg.h"
#include "xthread.h"
#include "xerror.h"
#include "xmem.h"
#include "xatomic.h"
#include "xutil.h"

typedef struct {
    xWorkFunc           func;
    xptr                user_data;
    xbool               exclusive;
    xASQueue            *queue;
    xCond               cond;
    xssize              max_threads;
    xssize              num_threads;
    xbool               running   :1;
    xbool               immediate :1;
    xbool               waiting   :1;
    xCmpFunc            sort_func;
    xptr                sort_data;
} RealThreadPool;

#define DEBUG_MSG(x)    //x_printf x
static xASQueue         *_unused_thread_queue = NULL;
static xint             _unused_threads = 0;
static xint             _max_unused_threads = 2;
static xint             _max_idle_time = 15 * 1000;
static xint             _wakeup_thread_serial = 0;
static xint             _kill_unused_threads = 0;
static xptr             _wakeup_thread_marker = &_unused_threads;
static void
thread_pool_delete      (RealThreadPool         *pool)
{
    x_return_if_fail (pool);
    x_return_if_fail (pool->running == FALSE);
    x_return_if_fail (pool->num_threads == 0);

    x_asqueue_unref (pool->queue);
    x_cond_clear (&pool->cond);

    x_free (pool);
}

static xptr
thread_pool_wait_for_new_task   (RealThreadPool *pool)
{
    xptr task = NULL;

    if (pool->running || (!pool->immediate &&
                          x_asqueue_length_unlocked (pool->queue) > 0)) {
        /* This thread pool is still active. */
        if (pool->num_threads > pool->max_threads && pool->max_threads != -1) {
            /* This is a superfluous thread, so it goes to the global pool. */
            DEBUG_MSG (("superfluous thread %p in pool %p.",
                        x_thread_self (), pool));
        }
        else if (pool->exclusive) {
            /* Exclusive threads stay attached to the pool. */
            task = x_asqueue_pop_unlocked (pool->queue);

            DEBUG_MSG (("thread %p in exclusive pool %p waits for task "
                        "(%d running, %d unprocessed).",
                        x_thread_self (), pool, pool->num_threads,
                        x_asqueue_length_unlocked (pool->queue)));
        }
        else {
            /* A thread will wait for new tasks for at most 1/2
             * second before going to the global pool.
             */
            DEBUG_MSG (("thread %p in pool %p waits for up to a 1/2 second for task "
                        "(%d running, %d unprocessed).",
                        x_thread_self (), pool, pool->num_threads,
                        x_asqueue_length_unlocked (pool->queue)));

            task = x_asqueue_pop_timeout_unlocked (pool->queue, 5000);
        }
    }
    else
    {
        /* This thread pool is inactive, it will no longer process tasks. */
        DEBUG_MSG (("pool %p not active, thread %p will go to global pool "
                    "(running: %s, immediate: %s, len: %d).",
                    pool, x_thread_self (),
                    pool->running ? "true" : "false",
                    pool->immediate ? "true" : "false",
                    x_asqueue_length_unlocked (pool->queue)));
    }

    return task;
}
static void
thread_pool_wakeup_and_stop_all_L (RealThreadPool     *pool)
{
    xint i;

    x_return_if_fail (pool);
    x_return_if_fail (pool->running == FALSE);
    x_return_if_fail (pool->num_threads != 0);

    pool->immediate = TRUE;

    for (i = 0; i < pool->num_threads; i++)
        x_asqueue_push_unlocked (pool->queue, X_SIZE_TO_PTR (1),NULL,NULL);
}

static RealThreadPool*
thread_pool_wait_for_new_pool (void)
{
    RealThreadPool *pool;
    xint local_wakeup_thread_serial;
    xint local_max_unused_threads;
    xint local_max_idle_time;
    xint last_wakeup_thread_serial;
    xbool   have_relayed_thread_marker = FALSE;

    local_max_unused_threads = x_atomic_int_get (&_max_unused_threads);
    local_max_idle_time = x_atomic_int_get (&_max_idle_time);
    last_wakeup_thread_serial = x_atomic_int_get (&_wakeup_thread_serial);

    x_atomic_int_inc (&_unused_threads);

    do {
        if (x_atomic_int_get (&_unused_threads) >= local_max_unused_threads) {
            /* If this is a superfluous thread, stop it. */
            pool = NULL;
        }
        else if (local_max_idle_time > 0) {
            /* If a maximal idle time is given, wait for the given time. */
            DEBUG_MSG (("thread %p waiting in global pool for %f seconds.",
                        x_thread_self (), local_max_idle_time / 1000.0));

            pool = x_asqueue_pop_timeout (_unused_thread_queue,
                                          local_max_idle_time * 1000);
        }
        else {
            /* If no maximal idle time is given, wait indefinitely. */
            DEBUG_MSG (("thread %p waiting in global pool.", x_thread_self ()));
            pool = x_asqueue_pop (_unused_thread_queue);
        }

        if (pool == _wakeup_thread_marker) {
            local_wakeup_thread_serial = x_atomic_int_get (&_wakeup_thread_serial);
            if (last_wakeup_thread_serial == local_wakeup_thread_serial)
            {
                if (!have_relayed_thread_marker)
                {
                    /* If this wakeup marker has been received for
                     * the second time, relay it.
                     */
                    DEBUG_MSG (("thread %p relaying wakeup message to "
                                "waiting thread with lower serial.",
                                x_thread_self ()));

                    x_asqueue_push (_unused_thread_queue,
                                    _wakeup_thread_marker,NULL, NULL);
                    have_relayed_thread_marker = TRUE;

                    /* If a wakeup marker has been relayed, this thread
                     * will get out of the way for 100 microseconds to
                     * avoid receiving this marker again.
                     */
                    x_usleep (100);
                }
            }
            else {
                if (x_atomic_int_add (&_kill_unused_threads, -1) > 0)
                {
                    pool = NULL;
                    break;
                }

                DEBUG_MSG (("thread %p updating to new limits.",
                            x_thread_self ()));

                local_max_unused_threads = 
                    x_atomic_int_get (&_max_unused_threads);
                local_max_idle_time = x_atomic_int_get (&_max_idle_time);
                last_wakeup_thread_serial = local_wakeup_thread_serial;

                have_relayed_thread_marker = FALSE;
            }
        }
    } while (pool == _wakeup_thread_marker);

    x_atomic_int_add (&_unused_threads, -1);

    return pool;
}

static xptr
thread_pool_thread      (xptr           data)
{
    RealThreadPool *pool = data;

    DEBUG_MSG (("thread %p started for pool %p.", x_thread_self (), pool));

    x_asqueue_lock (pool->queue);

    while (TRUE) {
        xptr task;

        task = thread_pool_wait_for_new_task (pool);
        if (task) {
            if (pool->running || !pool->immediate) {
                /* A task was received and the thread pool is active,
                 * so execute the function.
                 */
                x_asqueue_unlock (pool->queue);
                DEBUG_MSG (("thread %p in pool %p calling func.",
                            x_thread_self (), pool));
                pool->func (task, pool->user_data);
                x_asqueue_lock (pool->queue);
            }
        }
        else {
            /* No task was received, so this thread goes to the global pool. */
            xbool free_pool = FALSE;

            DEBUG_MSG (("thread %p leaving pool %p for global pool.",
                        x_thread_self (), pool));
            pool->num_threads--;

            if (!pool->running) {
                if (!pool->waiting) {
                    if (pool->num_threads == 0)
                    {
                        /* If the pool is not running and no other
                         * thread is waiting for this thread pool to
                         * finish and this is the last thread of this
                         * pool, free the pool.
                         */
                        free_pool = TRUE;
                    }
                    else {
                        /* If the pool is not running and no other
                         * thread is waiting for this thread pool to
                         * finish and this is not the last thread of
                         * this pool and there are no tasks left in the
                         * queue, wakeup the remaining threads.
                         */
                        if (x_asqueue_length_unlocked (pool->queue) ==
                            - pool->num_threads)
                            thread_pool_wakeup_and_stop_all_L (pool);
                    }
                }
                else if (pool->immediate ||
                         x_asqueue_length_unlocked (pool->queue) <= 0)
                {
                    /* If the pool is not running and another thread is
                     * waiting for this thread pool to finish and there
                     * are either no tasks left or the pool shall stop
                     * immediately, inform the waiting thread of a change
                     * of the thread pool state.
                     */
                    x_cond_broadcast (&pool->cond);
                }
            }

            x_asqueue_unlock (pool->queue);

            if (free_pool)
                thread_pool_delete (pool);

            if ((pool = thread_pool_wait_for_new_pool ()) == NULL)
                break;

            x_asqueue_lock (pool->queue);

            DEBUG_MSG (("thread %p entering pool %p from global pool.",
                        x_thread_self (), pool));

            /* pool->num_threads++ is not done here, but in
             * g_thread_pool_start_thread to make the new started
             * thread known to the pool before itself can do it.
             */
        }
    }

    return NULL;
}
static xbool
thread_pool_start_thread(RealThreadPool *pool,
                         xError         **error)
{
    xbool success = FALSE;

    if (pool->num_threads >= pool->max_threads && pool->max_threads != -1)
        /* Enough threads are already running */
        return TRUE;

    x_asqueue_lock (_unused_thread_queue);

    if (x_asqueue_length_unlocked (_unused_thread_queue) < 0) {
        x_asqueue_push_unlocked (_unused_thread_queue, pool, NULL, NULL);
        success = TRUE;
    }

    x_asqueue_unlock (_unused_thread_queue);

    if (!success) {
        xThread *thread;

        /* No thread was found, we have to start a new one */
        thread = x_thread_try_new ("pool",
                                   X_PTR_FUNC(thread_pool_thread),
                                   pool,
                                   error);

        if (thread == NULL)
            return FALSE;

        x_thread_unref (thread);
    }

    /* See comment in g_thread_pool_thread_proxy as to why this is done
     * here and not there
     */
    pool->num_threads++;

    return TRUE;
}

xThreadPool*
x_thread_pool_new_full  (xWorkFunc      func,
                         xptr           user_data,
                         xssize         max_threads,
                         xbool          exclusive,
                         xError         **error)
{
    RealThreadPool *retval;

    x_return_val_if_fail (func, NULL);
    x_return_val_if_fail (!exclusive || max_threads != -1, NULL);
    x_return_val_if_fail (max_threads >= -1, NULL);

    retval = x_new (RealThreadPool, 1);

    retval->func = func;
    retval->user_data = user_data;
    retval->exclusive = exclusive;
    retval->queue = x_asqueue_new ();
    x_cond_init (&retval->cond);
    retval->max_threads = max_threads;
    retval->num_threads = 0;
    retval->running = TRUE;
    retval->immediate = FALSE;
    retval->waiting = FALSE;
    retval->sort_func = NULL;
    retval->sort_data = NULL;

    if ( !x_atomic_ptr_get (&_unused_thread_queue)
         && x_once_init_enter (&_unused_thread_queue) ){
        _unused_thread_queue = x_asqueue_new ();
        x_once_init_leave (&_unused_thread_queue); 
    }

    if (retval->exclusive) {
        x_asqueue_lock (retval->queue);

        while (retval->num_threads < retval->max_threads) {
            xError *local_error = NULL;

            if (!thread_pool_start_thread (retval, &local_error)) {
                x_propagate_error (error, local_error);
                break;
            }
        }
        x_asqueue_unlock (retval->queue);
    }

    return (xThreadPool*) retval;
}

void
x_thread_pool_delete    (xThreadPool    *pool,
                         xbool          immediate,
                         xbool          wait_)
{
    RealThreadPool *real; 

    real = (RealThreadPool*) pool;

    x_return_if_fail (real);
    x_return_if_fail (real->running);

    /* If there's no thread allowed here, there is not much sense in
     * not stopping this pool immediately, when it's not empty
     */
    x_return_if_fail (immediate ||
                      real->max_threads != 0);

    x_asqueue_lock (real->queue);

    real->running = FALSE;
    real->immediate = immediate;
    real->waiting = wait_;

    if (wait_) {
        while (x_asqueue_length_unlocked (real->queue)
               != -real->num_threads &&
               !(immediate && real->num_threads == 0))
            x_cond_wait (&real->cond, (xMutex*)(real->queue));
    }

    if (immediate || x_asqueue_length_unlocked (real->queue)
        == -real->num_threads) {
        /* No thread is currently doing something (and nothing is left
         * to process in the queue)
         */
        if (real->num_threads == 0) {
            /* No threads left, we clean up */
            x_asqueue_unlock (real->queue);
            thread_pool_delete (real);
            return;
        }

        thread_pool_wakeup_and_stop_all_L (real);
    }

    /* The last thread should cleanup the pool */
    real->waiting = FALSE;
    x_asqueue_unlock (real->queue);
}

xbool
x_thread_pool_push      (xThreadPool    *pool,
                         xptr           data,
                         xError         **error)
{
    RealThreadPool *real;
    xbool       result;

    real = (RealThreadPool*) pool;

    x_return_val_if_fail (real, FALSE);
    x_return_val_if_fail (real->running, FALSE);

    result = TRUE;

    x_asqueue_lock (real->queue);

    if (x_asqueue_length_unlocked (real->queue) >= 0) {
        /* No thread is waiting in the queue */
        xError *local_error = NULL;

        if (!thread_pool_start_thread (real, &local_error)) {
            x_propagate_error (error, local_error);
            result = FALSE;
        }
    }

    x_asqueue_push_unlocked (real->queue, data,
                             real->sort_func, real->sort_data);
    x_asqueue_unlock (real->queue);

    return result;
}
