#include "config.h"
#include "xqueue.h"
#include "xthread.h"
#include "xatomic.h"
#include "xslist.h"
#include "xmem.h"
#include "xmsg.h"
#include "xthread.h"
#include "xasyncqueue.h"
#include "xslice.h"
#include "xtest.h"
struct rw_node
{
    volatile struct rw_node*    next;
    volatile xbool              ready;
};
#define RW_NODE_NEW(queue)                                              \
    (struct rw_node*)((xbyte*)x_slice_alloc                             \
                      (queue->elt_size+sizeof(struct rw_node))          \
                      + queue->elt_size)
#define RW_NODE_DELETE(queue, node)                                     \
    x_slice_free1 (queue->elt_size+sizeof(struct rw_node),              \
                   (xbyte*)rw_node - queue->elt_size);

struct _xRWQueue
{
    xFreeFunc                   free_func;
    xint                        ref_count;
    xsize                       elt_size;
    xsize                       read_count;
    xsize                       write_count;
    volatile struct rw_node     *read_ptr;
    volatile struct rw_node     *read_head;
    volatile struct rw_node     *write_head;
    volatile struct rw_node     *write_ptr;
};

xRWQueue*
x_rwqueue_new_full      (xsize          element_size,
                         xsize          reserved_size,
                         xFreeFunc      free_func)
{
    volatile struct rw_node  *rw_node;
    xRWQueue *queue;

    element_size = (((element_size) + 0xf) & ~(xsize) 0xf);
    reserved_size = MIN(reserved_size, 2);

    queue = x_new (xRWQueue, 1);
    queue->ref_count = 1;
    queue->elt_size = element_size;
    queue->free_func = free_func;
    queue->read_count = 0;
    queue->write_count = 0;

    /* 构成环形单向链表 */
    rw_node = RW_NODE_NEW(queue);
    rw_node->ready = FALSE;
    queue->write_ptr = queue->read_ptr = rw_node;
    while (reserved_size > 1) {
        rw_node = rw_node -> next = RW_NODE_NEW(queue);
        rw_node->ready = FALSE;
        --reserved_size;
    }
    rw_node->next = queue->write_ptr;

    queue->read_head  = queue->read_ptr;
    queue->write_head = queue->write_ptr;
    return queue;
}


xRWQueue*
x_rwqueue_ref           (xRWQueue       *queue)
{
    x_return_val_if_fail (queue != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&queue->ref_count) > 0, NULL);

    x_atomic_int_inc (&queue->ref_count);

    return queue;
}

xint
x_rwqueue_unref         (xRWQueue       *queue)
{
    volatile struct rw_node  *rw_node, *next;
    xint ref_count;

    x_return_val_if_fail (queue, -1);
    x_return_val_if_fail (x_atomic_int_get (&queue->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&queue->ref_count);
    if X_LIKELY(ref_count != 0)
        return ref_count;
    /* [read_head, write_ptr) 为有效结点 */
    if (queue->free_func) {
        rw_node = queue->read_head;
        while (rw_node != queue->write_ptr) {
            x_assert(rw_node->ready==TRUE);

            queue->free_func(((xbyte*)rw_node - queue->elt_size));
            rw_node = rw_node->next;
        }
    }
    /* 释放所有结点 */
    rw_node = queue->write_ptr;
    do {
        next=rw_node->next;
        RW_NODE_DELETE(queue, rw_node);
        rw_node=next;
    }while (rw_node != queue->write_ptr);

    x_free (queue);
    return 0;
}

xptr
x_rwqueue_write_lock    (xRWQueue       *queue,
                         xbool          try_lock)
{
    volatile struct rw_node  *rw_node;
    x_return_val_if_fail (queue != NULL, NULL);

    if(queue->write_head->next == queue->read_ptr
       || queue->write_head->next->ready)
    {
        if (try_lock)
            return NULL;
        rw_node = RW_NODE_NEW(queue);
        rw_node->ready = FALSE;
        rw_node->next = queue->write_head->next;
        queue->write_head->next = rw_node;
    }
    rw_node = queue->write_head;
    queue->write_head = queue->write_head->next;

    return (xbyte*)rw_node - queue->elt_size;
}

void
x_rwqueue_write_unlock  (xRWQueue       *queue)
{
    x_return_if_fail (queue != NULL);
    x_assert (queue->write_ptr->ready == FALSE);

    queue->write_ptr->ready = TRUE;
    queue->write_ptr = queue->write_ptr->next;
    queue->write_count++;
}

void
x_rwqueue_write_cancel  (xRWQueue       *queue,
                         xptr           element)
{
    struct rw_node  *rw_node;
    x_return_if_fail (queue != NULL);

    rw_node = (struct rw_node*)((xbyte*)element+queue->elt_size);
    x_assert (rw_node->ready == FALSE);

    queue->write_head = rw_node;
}

xptr
x_rwqueue_read_lock     (xRWQueue       *queue)
{
    volatile struct rw_node  *rw_node;
    x_return_val_if_fail (queue != NULL, NULL);

    if(queue->read_head==queue->write_ptr
       || !queue->read_head->ready) {
        return NULL;
    }
    rw_node=queue->read_head;
    queue->read_head =queue->read_head->next;
    return (xbyte*)rw_node - queue->elt_size;
}

void
x_rwqueue_read_unlock   (xRWQueue       *queue)
{
    x_return_if_fail (queue != NULL);
    x_assert (queue->read_ptr->ready == TRUE);

    queue->read_ptr->ready = FALSE;
    queue->read_ptr = queue->read_ptr->next;
    queue->read_count++;
}

void
x_rwqueue_read_cancel   (xRWQueue       *queue,
                         xptr           element)
{
    struct rw_node  *rw_node;
    x_return_if_fail (queue != NULL);

    rw_node = (struct rw_node*)((xbyte*)element+queue->elt_size);
    x_assert (rw_node->ready == TRUE);

    queue->read_head = rw_node;
}

void
x_rwqueue_shrink        (xRWQueue       *queue)
{
    volatile struct rw_node  *rw_node, *next;

    x_return_if_fail (queue != NULL);
    /* [write_head, read_ptr) 为空闲结点 */
    rw_node = queue->write_head->next;
    while(rw_node != queue->read_ptr) {
        x_assert(rw_node->ready==FALSE);

        next=rw_node->next;
        RW_NODE_DELETE(queue, rw_node);
        rw_node=next;
    }
    queue->write_head->next = queue->read_ptr;
}

xsize
x_rwqueue_len           (xRWQueue       *queue)
{
    x_return_val_if_fail (queue != NULL, 0);
    return queue->write_count - queue->read_count;
}

struct _xASQueue
{
    xMutex          mutex;
    xCond           cond;
    xQueue          queue;
    xFreeFunc       item_free_func;
    xuint           waiting_threads;
    xint            ref_count;
};
typedef struct {
    xCmpFunc       sort_func;
    xptr           sort_data;
} SortData;

xASQueue*
x_asqueue_new_full      (xFreeFunc      item_free_func)
{
    xASQueue *queue;

    queue = x_new (xASQueue, 1);
    x_mutex_init (&queue->mutex);
    x_cond_init (&queue->cond);
    x_queue_init (&queue->queue);
    queue->waiting_threads = 0;
    queue->ref_count = 1;
    queue->item_free_func = item_free_func;

    return queue;
}  

void
x_asqueue_push          (xASQueue    *queue,
                         xptr           data,
                         xCmpFunc       sort_func,
                         xptr           sort_data)
{
    x_return_if_fail (queue != NULL);
    x_return_if_fail (data  != NULL);


    x_mutex_lock (&queue->mutex);
    x_asqueue_push_unlocked (queue, data, sort_func, sort_data);
    x_mutex_unlock (&queue->mutex);
}

void
x_asqueue_lock          (xASQueue    *queue)
{
    x_return_if_fail (queue  != NULL);
    x_mutex_lock (&queue->mutex);
}

void
x_asqueue_unlock        (xASQueue    *queue)
{
    x_return_if_fail (queue  != NULL);
    x_mutex_unlock (&queue->mutex);
}

static xint
asqueue_cmp             (xptr           v1,
                         xptr           v2,
                         SortData       *sd)
{
    return -sd->sort_func (v1, v2, sd->sort_data);
}
void
x_asqueue_push_unlocked (xASQueue    *queue,
                         xptr           data,
                         xCmpFunc       sort_func,
                         xptr           sort_data)
{
    x_return_if_fail (queue != NULL);
    x_return_if_fail (data  != NULL);
    if (!sort_func) {
        x_queue_push_head (&queue->queue, data);
    }
    else {
        SortData sd={sort_func, sort_data};
        x_queue_insert_sorted (&queue->queue, data,
                               (xCmpFunc)asqueue_cmp,
                               &sd);
    }
}

xssize
x_asqueue_length_unlocked(xASQueue   *queue)
{
    x_return_val_if_fail (queue != NULL, 0);

    return queue->queue.length - queue->waiting_threads;
}

xptr
x_asqueue_pop_timeout   (xASQueue    *queue,
                         xuint32        timeout_ms)
{
    xptr retval;
    x_return_val_if_fail (queue != NULL, NULL);

    x_mutex_lock (&queue->mutex);
    retval = x_asqueue_pop_timeout_unlocked (queue, timeout_ms);
    x_mutex_unlock (&queue->mutex);

    return retval;
}

xptr
x_asqueue_pop_timeout_unlocked  (xASQueue    *queue,
                                 xuint32        timeout_ms)
{
    if (!x_queue_peek_tail_link (&queue->queue) && timeout_ms != 0) {
        queue->waiting_threads++;
        while (!x_queue_peek_tail_link (&queue->queue)) {
            if(!x_cond_wait_timeout (&queue->cond,
                                     &queue->mutex,
                                     timeout_ms)){
                queue->waiting_threads--;
                return NULL;
            }
        }
        queue->waiting_threads--;
    }

    return x_queue_pop_tail (&queue->queue);
}

xASQueue*
x_asqueue_ref           (xASQueue    *queue)
{
    x_return_val_if_fail (queue != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&queue->ref_count) > 0, NULL);

    x_atomic_int_inc (&queue->ref_count);

    return queue;
}    

xint
x_asqueue_unref         (xASQueue    *queue)
{
    xint ref_count;

    x_return_val_if_fail (queue, -1);
    x_return_val_if_fail (x_atomic_int_get (&queue->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&queue->ref_count);
    if X_LIKELY(ref_count != 0)
        return ref_count;

    x_return_val_if_fail (queue->waiting_threads == 0, 0);
    x_mutex_clear (&queue->mutex);
    x_cond_clear (&queue->cond);
    x_queue_clear (&queue->queue, queue->item_free_func);
    x_free (queue);

    return ref_count;
}  
