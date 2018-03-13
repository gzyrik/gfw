#include "config.h"
#include "xslist.h"
#include "xslice.h"
static inline xSList*
slist_remove_link       (xSList             *list,
                         xSList             *link)
{
    xSList *tmp;
    xSList *prev;

    prev = NULL;
    tmp = list;

    while (tmp) {
        if (tmp == link) {
            if (prev)
                prev->next = tmp->next;
            if (list == tmp)
                list = list->next;

            tmp->next = NULL;
            break;
        }

        prev = tmp;
        tmp = tmp->next;
    }

    return list;
}

void
x_slist_free_full       (xSList         *list,
                         xFreeFunc      free_func)
{
    while (list) {
        xSList *next = list->next;
        free_func (list->data);
        x_slist_free1 (list);
        list = next;
    }
}

xSList*
x_slist_last            (xSList         *list)
{
    if (list) {
        while (list->next)
            list = list->next;
    }

    return list;
}

xSList*
x_slist_append          (xSList         *list,
                         xptr           data)
{
    xSList *new_list;
    xSList *last;

    new_list = x_slist_alloc();
    new_list->data = data;
    new_list->next = NULL;

    if (list) {
        last = x_slist_last (list);
        last->next = new_list;

        return list;
    }
    else
        return new_list;
}

xSList*
x_slist_prepend         (xSList         *list,
                         xptr           data)
{
    xSList *new_list;

    new_list = x_slist_alloc ();
    new_list->data = data;
    new_list->next = list;

    return new_list;
}

xSList*
x_slist_concat          (xSList         *list1,
                         xSList         *list2)
{
    if (list2) {
        if (list1)
            x_slist_last (list1)->next = list2;
        else
            list1 = list2;
    }

    return list1;
}
xSList* 
x_slist_delete_link     (xSList         *list,
                         xSList         *link)
{
    list = slist_remove_link (list, link);
    x_slist_free1 (link);

    return list;
}
xSList*
x_slist_reverse         (xSList         *list)
{
    xSList *prev = NULL;

    while (list) {
        xSList *next = list->next;

        list->next = prev;

        prev = list;
        list = next;
    }

    return prev;
}

xSList*
x_slist_find_full       (xSList         *list,
                         xcptr          data,
                         xCmpFunc       cmp_func)
{
    while (list) {
        if (list->data == data)
            break;
        else if (cmp_func && cmp_func(list->data, data) == 0)
            break;
        list = list->next;
    }

    return list;
}

xSList*
x_slist_remove          (xSList         *list,
                         xcptr          data)
{
    xSList *tmp, *prev = NULL;

    tmp = list;
    while (tmp) {
        if (tmp->data == data) {
            if (prev)
                prev->next = tmp->next;
            else
                list = tmp->next;

            x_slist_free1 (tmp);
            break;
        }
        prev = tmp;
        tmp = prev->next;
    }

    return list;
}

void
x_slist_foreach         (xSList         *list,
                         xVisitFunc      func,
                         xptr           user_data)
{
    while (list) {
        xSList *next = list->next;
        (*func) (list->data, user_data);
        list = next;
    }
}
static xSList *
x_slist_sort_merge      (xSList         *l1,
                         xSList         *l2,
                         xCmpFunc       cmp_func,
                         xptr           user_data)
{
    xSList list, *l;
    xint cmp;

    l=&list;

    while (l1 && l2) {
        cmp = cmp_func (l1->data, l2->data, user_data);

        if (cmp <= 0) {
            l=l->next=l1;
            l1=l1->next;
        }
        else {
            l=l->next=l2;
            l2=l2->next;
        }
    }
    l->next= l1 ? l1 : l2;

    return list.next;
}

static xSList *
x_slist_sort_real       (xSList         *list,
                         xCmpFunc       cmp_func,
                         xptr           user_data)
{
    xSList *l1, *l2;

    if (!list)
        return NULL;
    if (!list->next)
        return list;

    l1 = list;
    l2 = list->next;

    while ((l2 = l2->next) != NULL) {
        if ((l2 = l2->next) == NULL)
            break;
        l1=l1->next;
    }
    l2 = l1->next;
    l1->next = NULL;

    return x_slist_sort_merge (x_slist_sort_real (list, cmp_func, user_data),
                               x_slist_sort_real (l2, cmp_func, user_data),
                               cmp_func, user_data);
}

xSList *
x_slist_sort            (xSList         *list,
                         xCmpFunc       cmp)
{
    return x_slist_sort_real (list,  cmp, NULL);
}

xSList*
x_slist_copy_deep       (xSList         *list,
                         xDupFunc       func,
                         xptr           user_data)
{
    xSList *new_list = NULL;

    if (list) {
        xSList *last;

        new_list = x_slist_alloc ();
        if (func)
            new_list->data = func (list->data, user_data);
        else
            new_list->data = list->data;
        last = new_list;
        list = list->next;
        while (list) {
            last->next = x_slist_alloc ();
            last = last->next;
            if (func)
                last->data = func (list->data, user_data);
            else
                last->data = list->data;
            list = list->next;
        }
        last->next = NULL;
    }

    return new_list;
}

