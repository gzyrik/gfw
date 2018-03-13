#ifndef __X_DATA_H__
#define __X_DATA_H__
#include "xquark.h"
X_BEGIN_DECLS
#define     X_DATA_LIST_FLAGS_MASK   3

void        x_data_list_init         (xDataList      **datalist);

void        x_data_list_clear        (xDataList      **datalist);

void        x_data_list_id_set_data_full (xDataList      **datalist,
                                          xQuark         key_id,
                                          xptr           data,
                                          xFreeFunc      free_func);

xptr        x_data_list_id_remove_no_notify  (xDataList  **datalist,
                                              xQuark    key_id);

void        x_data_list_foreach      (xDataList      **datalist,
                                      xVisitFunc     func,
                                      xptr           user_data);

xptr        x_data_list_id_dup_data  (xDataList      **datalist,
                                      xQuark         key_id,
                                      xDupFunc       dup_func,
                                      xptr           user_data);

void        x_data_list_set_flags    (xDataList      **datalist,
                                      xuint          flags);

void        x_data_list_unset_flags  (xDataList      **datalist,
                                      xuint          flags);

xuint       x_data_list_get_flags    (xDataList      **datalist);

#define     x_data_list_id_set_data(datalist, key_id, data)             \
    x_data_list_id_set_data_full (datalist, key_id, data, NULL)

#define     x_data_list_id_remove_data(datalist, key_id)                \
    x_data_list_id_set_data (datalist, key_id, NULL)

#define     x_data_list_id_get_data(datalist, key_id)                   \
    x_data_list_id_dup_data (datalist, key_id, NULL, NULL)

#define     x_data_list_set_data_full(datalist, key_str, data, free)    \
    x_data_list_id_set_data_full (datalist, x_quark_from (key_str),     \
                                  data, free)

#define     x_data_list_set_data(datalist, key_str, data)               \
    x_data_list_set_data_full (datalist, key_str, data, NULL)

#define     x_data_list_remove_data(datalist, key_str)                  \
    x_data_list_id_set_data (datalist, x_quark_try (key_str), NULL)

#define     x_data_list_remove_no_notify(datalist, key_str)             \
    x_data_list_id_remove_no_notify (datalist, x_quark_try (key_str))

#define     x_data_list_get_data(datalist, key_str)                     \
    x_data_list_id_get_data (datalist, x_quark_try (key_str))

void        x_data_set_destroy       (xcptr          location);

xptr        x_data_set_id_get_data   (xcptr          location,
                                      xQuark         key_id);

void        x_data_set_id_set_data_full  (xcptr          location,
                                          xQuark         key_id,
                                          xptr           data,
                                          xFreeFunc      free_func);

xptr        x_data_set_id_remove_no_notify   (xcptr      location,
                                              xQuark     key_id);

void        x_data_set_foreach       (xcptr          location,
                                      xVisitFunc     func,
                                      xptr           user_data);

#define     x_data_set_id_set_data(dataset, key_id, data)               \
    x_data_set_id_set_data_full (dataset, key_id, data, NULL)

#define     x_data_set_id_remove_data(dataset, key_id)                  \
    x_data_set_id_set_data (dataset, key_id, NULL)

#define     x_data_set_set_data_full(dataset, key_str, data, free)      \
    x_data_set_id_set_data_full (dataset, x_quark_from (key_str),       \
                                 data, free)

#define     x_data_set_set_data(dataset, key_str, data)                 \
    x_data_set_set_data_full (dataset, key_str, data, NULL)

#define     x_data_set_remove_data(dataset, key_str)                    \
    x_data_set_id_set_data (dataset, x_quark_try (key_str), NULL)

#define     x_data_set_get_data(dataset, key_str)                       \
    x_data_set_id_get_data (dataset, x_quark_try (key_str))

#define     x_data_set_remove_no_notify(dataset, key_str)               \
    x_data_set_id_remove_no_notify (dataset, x_quark_try (key_str))

X_END_DECLS
#endif /* __X_DATA_H__ */
