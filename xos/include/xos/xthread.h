#ifndef __X_THREAD_H__
#define __X_THREAD_H__
#include "xtype.h"
X_BEGIN_DECLS
#define X_LOCK_NAME(name)           x_ ## name ## _lock
#define X_LOCK_DEFINE_STATIC(name)  static X_LOCK_DEFINE (name)
#define X_LOCK_DEFINE(name)         xMutex X_LOCK_NAME (name)
#define X_LOCK_EXTERN(name)         extern xMutex X_LOCK_NAME (name)

#define X_LOCK(name)                x_mutex_lock    (&X_LOCK_NAME (name))
#define X_UNLOCK(name)              x_mutex_unlock  (&X_LOCK_NAME (name))
#define X_TRYLOCK(name)             x_mutex_try_lock (&X_LOCK_NAME (name))

#define X_RWLOCK_NAME(name)         x_ ## name ## _rwlock
#define X_RWLOCK_DEFINE_STATIC(name) static X_RWLOCK_DEFINE (name)
#define X_RWLOCK_DEFINE(name)       xRWLock X_RWLOCK_NAME (name)
#define X_RWLOCK_EXTERN(name)       extern xRWLock X_RWLOCK_NAME (name)

#define X_READ_LOCK(name)           x_rwlock_rlock   (&X_RWLOCK_NAME (name))
#define X_READ_UNLOCK(name)         x_rwlock_unrlock (&X_RWLOCK_NAME (name))
#define X_WRITE_LOCK(name)          x_rwlock_wlock   (&X_RWLOCK_NAME (name))
#define X_WRITE_UNLOCK(name)        x_rwlock_unwlock (&X_RWLOCK_NAME (name))

#define X_RECLOCK_NAME(name)        x_ ## name ## _reclock
#define X_RECLOCK_DEFINE_STATIC(name) static X_RECLOCK_DEFINE (name)
#define X_RECLOCK_DEFINE(name)      xRecMutex X_RECLOCK_NAME (name)
#define X_RECLOCK_EXTERN(name)      extern xRecMutex X_RECLOCK_NAME (name)

#define X_REC_LOCK(name)            x_recmutex_lock   (&X_RECLOCK_NAME (name))
#define X_REC_UNLOCK(name)          x_recmutex_unlock (&X_RECLOCK_NAME (name))

#define X_PRIVATE_INIT(notify)      { NULL, X_VOID_FUNC(notify), { NULL, NULL} }

xQuark      x_thread_domain         (void);
xThread*    x_thread_new            (xcstr          name,
                                     xThreadFunc    func,
                                     xptr           data);
xThread*    x_thread_try_new        (xcstr          name,
                                     xThreadFunc    func,
                                     xptr           data,
                                     xError         **error);
xThread*    x_thread_ref            (xThread        *thread);
xint        x_thread_unref          (xThread        *thread);
xThread*    x_thread_self           (void);
void        x_thread_exit           (xptr           retval);
xptr        x_thread_join           (xThread        *thread);
void        x_thread_yield          (void);

void        x_mutex_init            (xMutex         *mutex);
xbool       x_mutex_try_lock        (xMutex         *mutex);
void        x_mutex_lock            (xMutex         *mutex);
void        x_mutex_unlock          (xMutex         *mutex);
void        x_mutex_clear           (xMutex         *mutex);

void        x_rwlock_init           (xRWLock        *rwlock);
xbool       x_rwlock_try_wlock      (xRWLock        *rwlock);
void        x_rwlock_wlock          (xRWLock        *rwlock);
void        x_rwlock_unwlock        (xRWLock        *rwlock);
xbool       x_rwlock_try_rlock      (xRWLock        *rwlock);
void        x_rwlock_rlock          (xRWLock        *rwlock);
void        x_rwlock_unrlock        (xRWLock        *rwlock);
void        x_rwlock_clear          (xRWLock        *rwlock);

void        x_recmutex_init         (xRecMutex      *recmutex);
xbool       x_recmutex_try_lock     (xRecMutex      *recmutex);
void        x_recmutex_lock         (xRecMutex      *recmutex);
void        x_recmutex_unlock       (xRecMutex      *recmutex);
void        x_recmutex_clear        (xRecMutex      *recmutex);


void        x_cond_init             (xCond          *cond);
xbool       x_cond_wait_timeout     (xCond          *cond,
                                     xMutex         *mutex,
                                     xuint          timeout);
#define     x_cond_wait(cond, mutex)                                    \
    x_cond_wait_timeout (cond, mutex, -1)

void        x_cond_signal           (xCond          *cond);
void        x_cond_broadcast        (xCond          *cond);
void        x_cond_clear            (xCond          *cond);

xptr        x_private_get           (xPrivate       *key);
void        x_private_set           (xPrivate       *key,
                                     xptr           value);

void        x_private_replace       (xPrivate       *key,
                                     xptr           value);

union _xMutex
{
    /*< private >*/
    xptr            p;
    xuint           i[2];
};

struct _xRWLock
{
    /*< private >*/
    xptr            p;
    xuint           i[2];
};

struct _xCond
{
    /*< private >*/
    xptr            p;
    xuint           i[2];
};

struct _xRecMutex
{
    /*< private >*/
    xptr            p;
    xuint           i[2];
};

struct _xPrivate
{
    /*< private >*/
    xptr            p;
    xFreeFunc       notify;
    xptr            future[2];
};

struct  _xThread
{
  /*< private >*/
  xThreadFunc       func;
  xptr              data;
  xbool             joinable;
  xint              priority;
};
X_END_DECLS
#endif /*__X_THREAD_H__ */
