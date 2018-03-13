#include "config.h"
#include "xqueue.h"
#include "xslice.h"
#include "xlist.h"
#include "xmsg.h"
void
x_queue_init            (xQueue         *queue)
{
    x_return_if_fail (queue != NULL);
    queue->head = queue->tail = NULL;
    queue->length = 0;
}

void
x_queue_clear           (xQueue         *queue,
                         xFreeFunc      free_func)
{
    x_return_if_fail (queue != NULL);

    if (free_func)
        x_queue_foreach (queue, free_func, NULL);

    x_list_free (queue->head);
    x_queue_init (queue);
}
void
x_queue_delete          (xQueue         *queue)
{
    x_return_if_fail (queue != NULL);

    x_list_free (queue->head);
    x_slice_free (xQueue, queue);
}
void
x_queue_push_head       (xQueue         *queue,
                         xptr           data)
{
    x_return_if_fail (queue != NULL);

    queue->head = x_list_prepend (queue->head, data);
    if (!queue->tail)
        queue->tail = queue->head;
    queue->length++;
}

void
x_queue_push_tail       (xQueue         *queue,
                         xptr           data)
{
    x_return_if_fail (queue != NULL);

    queue->tail = x_list_append (queue->tail, data);
    if (queue->tail->next)
        queue->tail = queue->tail->next;
    else
        queue->head = queue->tail;
    queue->length++;
}
xptr
x_queue_pop_head        (xQueue         *queue)
{
    x_return_val_if_fail (queue != NULL, NULL);

    if (queue->head) {
        xList *node = queue->head;
        xptr data = node->data;

        queue->head = node->next;
        if (queue->head)
            queue->head->prev = NULL;
        else
            queue->tail = NULL;
        x_list_free1 (node);
        queue->length--;

        return data;
    }

    return NULL;

}

xptr
x_queue_pop_tail        (xQueue         *queue)
{
    x_return_val_if_fail (queue != NULL, NULL);

    if (queue->tail) {
        xList *node = queue->tail;
        xptr data = node->data;

        queue->tail = node->prev;
        if (queue->tail)
            queue->tail->next = NULL;
        else
            queue->head = NULL;
        queue->length--;
        x_list_free (node);

        return data;
    }

    return NULL;
}
void
x_queue_foreach         (xQueue         *queue,
                         xVisitFunc     func,
                         xptr           user_data)
{
    xList *list;

    x_return_if_fail (queue != NULL);
    x_return_if_fail (func != NULL);

    list = queue->head;
    while (list) {
        xList *next = list->next;
        func (list->data, user_data);
        list = next;
    }
}
void
x_queue_insert_sorted   (xQueue         *queue,
                         xptr           data,
                         xCmpFunc       func,
                         xptr           user_data)
{
    xList *list;

    x_return_if_fail (queue != NULL);

    list = queue->head;
    while (list && func (list->data, data, user_data) < 0)
        list = list->next;

    if (list)
        x_queue_insert_before (queue, list, data);
    else
        x_queue_push_tail (queue, data);
}
void
x_queue_insert_before   (xQueue         *queue,
                         xList          *sibling,
                         xptr           data)
{
    x_return_if_fail (queue != NULL);
    x_return_if_fail (sibling != NULL);

    queue->head = x_list_insert_before (queue->head, sibling, data);
    queue->length++;
}

void
x_queue_insert_after    (xQueue         *queue,
                         xList          *sibling,
                         xptr           data)
{
    x_return_if_fail (queue != NULL);
    x_return_if_fail (sibling != NULL);

    if (sibling == queue->tail)
        x_queue_push_tail (queue, data);
    else
        x_queue_insert_before (queue, sibling->next, data);
}
