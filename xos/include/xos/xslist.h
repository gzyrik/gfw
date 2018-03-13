#ifndef __X_SLIST_H__
#define __X_SLIST_H__
#include "xtype.h"
#include "xslice.h"
X_BEGIN_DECLS

/** Allocat slist */
#define     x_slist_alloc()         x_slice_new0(xSList)           

/** Free slist */
#define     x_slist_free(list)      x_slice_free_chain (xSList, list, next)

/** Only free slist node */
#define     x_slist_free1(list)     x_slice_free (xSList, list);

/** Free slist with callback */
void        x_slist_free_full       (xSList         *list,
                                     xFreeFunc      free_func);

/** return next node */
#define     x_slist_next(slist)	    ((slist)                        \
                                     ? (((xSList *)(slist))->next)  \
                                     : NULL)
/** return last node, slowly */
xSList*     x_slist_last            (xSList         *list);

/** add node at rear, slowly */
xSList*     x_slist_append          (xSList         *list,
                                     xptr           data) X_WARN_UNUSED_RET;

/** add node at front, very fast */
xSList*     x_slist_prepend         (xSList         *list,
                                     xptr           data) X_WARN_UNUSED_RET;

/** unlink node  */
xSList*     x_slist_delete_link     (xSList         *list,
                                     xSList         *node) X_WARN_UNUSED_RET;

/** link two slist */
xSList*     x_slist_concat          (xSList         *list1,
                                     xSList         *list2)X_WARN_UNUSED_RET;

/** reverse slist, slowly */
xSList*     x_slist_reverse         (xSList         *list) X_WARN_UNUSED_RET;

xSList*     x_slist_find_full       (xSList         *list,
                                     xcptr          data,
                                     xCmpFunc       cmp_func);

#define     x_slist_find(list, data) x_slist_find_full (list, data, NULL)

xSList*     x_slist_remove          (xSList         *list,
                                     xcptr          data) X_WARN_UNUSED_RET;

void        x_slist_foreach         (xSList         *list,
                                     xVisitFunc     visit,
                                     xptr           data);

xSList*    x_slist_sort             (xSList         *list,
                                     xCmpFunc       cmp) X_WARN_UNUSED_RET;

xSList*     x_slist_copy_deep       (xSList         *list,
                                     xDupFunc       func,
                                     xptr           data) X_WARN_UNUSED_RET;

#define     x_slist_copy(list) x_slist_copy_deep(list, NULL, NULL)

struct _xSList
{
    xptr        data;
    xSList      *next;
};

X_END_DECLS
#endif /* __X_SLIST_H__ */

