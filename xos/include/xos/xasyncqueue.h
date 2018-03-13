#ifndef __X_ASYNC_QUEUE_H__
#define __X_ASYNC_QUEUE_H__
#include "xtype.h"
X_BEGIN_DECLS
#define         x_rwqueue_new(elem_size) x_rwqueue_new_full (elem_size, 0, NULL)

xRWQueue*       x_rwqueue_new_full      (xsize          element_size,
                                         xsize          reserved_size,
                                         xFreeFunc      free_func);

xRWQueue*       x_rwqueue_ref           (xRWQueue       *queue);

xint            x_rwqueue_unref         (xRWQueue       *queue);

xptr            x_rwqueue_write_lock    (xRWQueue       *queue,
                                         xbool          try_lock);

void            x_rwqueue_write_unlock  (xRWQueue       *queue);

void            x_rwqueue_write_cancel  (xRWQueue       *queue,
                                         xptr           element);

xptr            x_rwqueue_read_lock     (xRWQueue       *queue);

void            x_rwqueue_read_unlock   (xRWQueue       *queue);

void            x_rwqueue_read_cancel   (xRWQueue       *queue,
                                         xptr           element);

void            x_rwqueue_shrink        (xRWQueue       *queue);

xsize           x_rwqueue_len           (xRWQueue       *queue);

xASQueue*       x_asqueue_new_full      (xFreeFunc      free_func);

#define         x_asqueue_new()         x_asqueue_new_full (NULL)

void            x_asqueue_push          (xASQueue    *queue,
                                         xptr           data,
                                         xCmpFunc       sort_func,
                                         xptr           sort_data);

xptr            x_asqueue_pop_timeout   (xASQueue    *queue,
                                         xuint32        timeout_ms);

#define         x_asqueue_try_pop(queue)                            \
    x_asqueue_pop_timeout (queue, 0)

#define         x_asqueue_pop(queue)                                \
    x_asqueue_pop_timeout (queue, X_MAXUINT32)

xASQueue*       x_asqueue_ref           (xASQueue    *queue);

xint            x_asqueue_unref         (xASQueue    *queue);

void            x_asqueue_lock          (xASQueue    *queue);

void            x_asqueue_unlock        (xASQueue    *queue);

void            x_asqueue_push_unlocked (xASQueue    *queue,
                                         xptr           data,
                                         xCmpFunc       sort_func,
                                         xptr           sort_data);

xssize          x_asqueue_length_unlocked(xASQueue   *queue);

xptr            x_asqueue_pop_timeout_unlocked  (xASQueue    *queue,
                                                 xuint32        timeout_ms);

#define         x_asqueue_try_pop_unlocked(queue)                   \
    x_asqueue_pop_timeout_unlocked (queue, 0)

#define         x_asqueue_pop_unlocked(queue)                       \
    x_asqueue_pop_timeout_unlocked (queue, X_MAXUINT32)

X_END_DECLS
#endif /* __X_ASYNC_QUEUE_H__ */
