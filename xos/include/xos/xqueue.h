#ifndef __X_QUEUE_H__
#define __X_QUEUE_H__
#include "xtype.h"
X_BEGIN_DECLS

#define     x_queue_new()           x_slice_new0 (xQueue)  

void        x_queue_init            (xQueue         *queue);

void        x_queue_clear           (xQueue         *queue,
                                     xFreeFunc      free_func);

void        x_queue_delete          (xQueue         *queue);

void        x_queue_push_head       (xQueue         *queue,
                                     xptr           data);

void        x_queue_push_tail       (xQueue         *queue,
                                     xptr           data);

xptr        x_queue_pop_head        (xQueue         *queue);

xptr        x_queue_pop_tail        (xQueue         *queue);

void        x_queue_foreach         (xQueue         *queue,
                                     xVisitFunc     visit,
                                     xptr           user_data);

void        x_queue_insert_before   (xQueue         *queue,
                                     xList          *sibling,
                                     xptr           data);

void        x_queue_insert_after    (xQueue         *queue,
                                     xList          *sibling,
                                     xptr           data);

void        x_queue_insert_sorted   (xQueue         *queue,
                                     xptr           data,
                                     xCmpFunc       cmp_func,
                                     xptr           user_data);

void        x_queue_unlink          (xQueue         *queue,
                                     xList          *link);

#define     x_queue_peek_tail_link(queue) ((queue)->tail)

#define     x_queue_peek_head_link(queue) ((queue)->head)

struct _xQueue
{
    xList       *head;
    xList       *tail;
    xsize       length;
};
X_END_DECLS
#endif /* __X_QUEUE_H__ */
