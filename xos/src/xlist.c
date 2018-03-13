#include "config.h"
#include "xlist.h"
#include "xslice.h"
#include "xmsg.h"
void
x_list_free_full        (xList          *list,
                         xFreeFunc      free_func)
{
    x_list_foreach (list, free_func, NULL);
    x_list_free (list);
}

xList*
x_list_append           (xList          *list,
                         xptr           data)
{
    xList *new_list;
    xList *last;

    new_list = x_list_alloc ();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        last = x_list_last (list);
        last->next = new_list;
        new_list->prev = last;
        return list;
    }
    else {
        new_list->prev = NULL;
        return new_list;
    }
}

xList*
x_list_prepend          (xList          *list,
                         xptr           data)
{
    xList *new_list;

    new_list = x_list_alloc ();
    new_list->data = data;
    new_list->next = list;

    if (list) {
        new_list->prev = list->prev;
        if (list->prev)
            list->prev->next = new_list;
        list->prev = new_list;
    }
    else
        new_list->prev = NULL;

    return new_list;

}
xList*
x_list_insert_before    (xList          *list,
                         xList          *sibling,
                         xptr           data)
{
    if (!list) {
        list = x_list_alloc ();
        list->data = data;
        x_return_val_if_fail (sibling == NULL, list);
        return list;
    }
    else if (sibling) {
        xList *node;

        node = x_list_alloc ();
        node->data = data;
        node->prev = sibling->prev;
        node->next = sibling;
        sibling->prev = node;
        if (node->prev) {
            node->prev->next = node;
            return list;
        }
        else {
            x_return_val_if_fail (sibling == list, node);
            return node;
        }
    }
    else {
        xList *last;

        last = list;
        while (last->next)
            last = last->next;

        last->next = x_list_alloc ();
        last->next->data = data;
        last->next->prev = last;
        last->next->next = NULL;

        return list;
    }
}
xList*
x_list_insert_sorted    (xList          *list,
                         xptr           data,
                         xCmpFunc       func,
                         xptr           user_data)
{
    xList *tmp_list = list;
    xList *new_list;
    xint cmp;

    x_return_val_if_fail (func != NULL, list);

    if (!list) {
        new_list = x_list_alloc ();
        new_list->data = data;
        return new_list;
    }

    cmp = func (data, tmp_list->data, user_data);

    while ((tmp_list->next) && (cmp > 0)) {
        tmp_list = tmp_list->next;
        cmp = func(data, tmp_list->data, user_data);
    }

    new_list = x_list_alloc ();
    new_list->data = data;

    if ((!tmp_list->next) && (cmp > 0)) {
        tmp_list->next = new_list;
        new_list->prev = tmp_list;
        return list;
    }

    if (tmp_list->prev) {
        tmp_list->prev->next = new_list;
        new_list->prev = tmp_list->prev;
    }
    new_list->next = tmp_list;
    tmp_list->prev = new_list;

    if (tmp_list == list)
        return new_list;
    return list;
}

xList*
x_list_concat           (xList          *list1,
                         xList          *list2)
{
    xList *tmp_list;

    if (list2) {
        tmp_list = x_list_last (list1);
        if (tmp_list)
            tmp_list->next = list2;
        else
            list1 = list2;
        list2->prev = tmp_list;
    }

    return list1;
}

xList*
x_list_remove           (xList          *list,
                         xcptr          data)
{
    xList *tmp;

    tmp = list;
    while (tmp) {
        if (tmp->data != data)
            tmp = tmp->next;
        else {
            if (tmp->prev)
                tmp->prev->next = tmp->next;
            if (tmp->next)
                tmp->next->prev = tmp->prev;

            if (list == tmp)
                list = list->next;

            x_list_free1 (tmp);

            break;
        }
    }
    return list;
}

xList*
x_list_last             (xList          *list)
{
    if (list) {
        while (list->next)
            list = list->next;
    }
    return list;
}

void
x_list_foreach          (xList	        *list,
                         xVisitFunc	    func,
                         xptr           user_data)
{
    while (list) {
        xList *next = list->next;
        func(list->data, user_data);
        list = next;
    }
}

xList*
x_list_find             (xList          *list,
                         xcptr          data)
{
    while (list) {
        if (list->data == data)
            break;
        list = list->next;
    }
    return list;
}
