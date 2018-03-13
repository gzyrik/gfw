#ifndef __X_LIST_H__
#define __X_LIST_H__
#include "xtype.h"
X_BEGIN_DECLS

#define     x_list_alloc()          x_slice_new0 (xList)           

#define     x_list_free(list)       x_slice_free_chain (xList, list, next)

#define     x_list_free1(list)      x_slice_free (xList, list);

void        x_list_free_full        (xList          *list,
                                     xFreeFunc      free_func);

xList*      x_list_append           (xList          *list,
                                     xptr           data) X_WARN_UNUSED_RET;

xList*      x_list_prepend          (xList          *list,
                                     xptr           data) X_WARN_UNUSED_RET;

xList*      x_list_insert_before    (xList          *list,
                                     xList          *sibling,
                                     xptr           data) X_WARN_UNUSED_RET;

xList*      x_list_insert_sorted    (xList          *list,
                                     xptr           data,
                                     xCmpFunc       func,
                                     xptr           user) X_WARN_UNUSED_RET;

xList*      x_list_concat           (xList          *list1,
                                     xList          *list2) X_WARN_UNUSED_RET;

xList*      x_list_remove           (xList          *list,
                                     xcptr          data) X_WARN_UNUSED_RET;

#define     x_list_previous(list)	((list) ? (((xList *)(list))->prev) : NULL)

#define     x_list_next(list)	    ((list) ? (((xList *)(list))->next) : NULL)

xList*      x_list_last             (xList          *list);

void        x_list_foreach          (xList	        *list,
                                     xVisitFunc	    visit,
                                     xptr           user_data);

xList*      x_list_find             (xList          *list,
                                     xcptr          data);

struct _xList
{
    xptr        data;
    xList       *next;
    xList       *prev;
};

X_END_DECLS
#endif /* __X_LIST_H__ */
