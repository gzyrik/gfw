#include "config.h"
#include "xdata.h"
#include "xatomic.h"
#include "xhash.h"
#include "xmsg.h"
#include "xbit.h"
#include "xmem.h"
#include "xthread.h"
#include "xslice.h"

#define DATA_LIST_LOCK_BIT               0x2
#define DATA_LIST_FLAGS_MASK_INTERNAL    0x7
/* datalist pointer accesses have to be carried out atomically */
#define DATA_LIST_GET_PTR(datalist)                                     \
    ((xDataList*) ((xsize) x_atomic_ptr_get (datalist) &                \
                   ~(xsize) DATA_LIST_FLAGS_MASK_INTERNAL))

#define DATA_LIST_SET_PTR(datalist, ptr) X_STMT_START {                 \
    xptr _oldv, _newv;                                                  \
    do {                                                                \
        _oldv = x_atomic_ptr_get (datalist);                            \
        _newv = (xptr) (((xsize) _oldv & DATA_LIST_FLAGS_MASK_INTERNAL) \
                        | (xsize) ptr);                                 \
    } while (!x_atomic_ptr_cas ((void**) datalist, _oldv, _newv));      \
} X_STMT_END

#define DATA_LIST_LOCK(datalist)                                        \
    x_ptr_bit_lock ((void **)datalist, DATA_LIST_LOCK_BIT)

#define DATA_LIST_UNLOCK(datalist)                                      \
    x_ptr_bit_unlock ((void **)datalist, DATA_LIST_LOCK_BIT)

typedef struct {
    xQuark              key;
    xptr                data;
    xFreeFunc           destroy;
} DataElt;

struct _xDataList
{
    xsize               len;     /* Number of elements */
    xsize               alloc;   /* Number of allocated elements */
    DataElt             data[1]; /* Flexible array */
};
typedef struct
{
    xcptr               location;
    xDataList           *datalist;
} DataSet;

X_LOCK_DEFINE_STATIC (dataset_ht);
static xHashTable     *dataset_ht = NULL;
/* should this be thread specific? */
static xPrivate dataset_cached;
static void
data_list_clear         (xDataList      **datalist)
{
    xDataList *data;
    xsize i;

    data = DATA_LIST_GET_PTR (datalist);
    DATA_LIST_SET_PTR (datalist, NULL);

    if (data) {
        X_UNLOCK (dataset_ht);
        for (i = 0; i < data->len; i++) {
            if (data->data[i].data && data->data[i].destroy)
                data->data[i].destroy (data->data[i].data);
        }
        X_LOCK (dataset_ht);

        x_free (data);
    }
}
static inline DataSet*
data_set_lookup_L       (xcptr          location)
{
    DataSet *dataset = x_private_get (&dataset_cached);

    if (dataset && dataset->location == location)
        return dataset;

    dataset = x_hash_table_lookup (dataset_ht, location);
    if (dataset)
        x_private_set (&dataset_cached, dataset);

    return dataset;
}
static void
data_set_destroy_L      (DataSet       *dataset)
{
    register xcptr dataset_location;

    dataset_location = dataset->location;
    while (dataset) {
        if (DATA_LIST_GET_PTR(&dataset->datalist) == NULL) {
            if (dataset == x_private_get (&dataset_cached))
                x_private_set (&dataset_cached, NULL);

            x_hash_table_remove (dataset_ht, (xptr)dataset_location);
            x_slice_free (DataSet, dataset);
            break;
        }

        data_list_clear (&dataset->datalist);
        dataset = data_set_lookup_L (dataset_location);
    }
}

static inline xptr  
data_list_set_internal  (xDataList      **datalist,
                         xQuark         key_id,
                         xptr           new_data,
                         xFreeFunc      free_func,
                         DataSet        *dataset)
{
    xDataList *d, *old_d;
    DataElt old, *data, *data_last, *data_end;

    DATA_LIST_LOCK (datalist);

    d = DATA_LIST_GET_PTR (datalist);

    if (new_data == NULL){ /* remove */
        if (d) {
            data = d->data;
            data_last = data + d->len - 1;
            while (data <= data_last) {
                if (data->key == key_id) {
                    old = *data;
                    if (data != data_last)
                        *data = *data_last;
                    d->len--;

                    /* We don't bother to shrink, but if all data are now gone
                     * we at least free the memory
                     */
                    if (d->len == 0)
                    {
                        DATA_LIST_SET_PTR (datalist, NULL);
                        x_free (d);
                        /* datalist may be situated in dataset, so must not be
                         * unlocked after we free it
                         */
                        DATA_LIST_UNLOCK (datalist);

                        /* the dataset destruction *must* be done
                         * prior to invocation of the data destroy function
                         */
                        if (dataset)
                            data_set_destroy_L(dataset);
                    }
                    else
                    {
                        DATA_LIST_UNLOCK (datalist);
                    }

                    /* We found and removed an old value
                     * the GData struct *must* already be unlinked
                     * when invoking the destroy function.
                     * we use (new_data==NULL && new_destroy_func!=NULL) as
                     * a special hint combination to "steal"
                     * data without destroy notification
                     */
                    if (old.destroy && !free_func)
                    {
                        if (dataset)
                            X_UNLOCK (dataset_ht);
                        old.destroy (old.data);
                        if (dataset)
                            X_LOCK (dataset_ht);
                        old.data = NULL;
                    }

                    return old.data;
                }
                data++;
            }
        }
    }
    else {
        old.data = NULL;
        if (d) {
            data = d->data;
            data_end = data + d->len;
            while (data < data_end) {
                if (data->key == key_id) {
                    if (!data->destroy) {
                        data->data = new_data;
                        data->destroy = free_func;
                        DATA_LIST_UNLOCK (datalist);
                    }
                    else {
                        old = *data;
                        data->data = new_data;
                        data->destroy = free_func;

                        DATA_LIST_UNLOCK (datalist);

                        /* We found and replaced an old value
                         * the GData struct *must* already be unlinked
                         * when invoking the destroy function.
                         */
                        if (dataset)
                            X_UNLOCK (dataset_ht);
                        old.destroy (old.data);
                        if (dataset)
                            X_LOCK (dataset_ht);
                    }
                    return NULL;
                }
                data++;
            }
        }

        /* The key was not found, insert it */
        old_d = d;
        if (d == NULL) {
            d = x_malloc (sizeof (xDataList));
            d->len = 0;
            d->alloc = 1;
        }
        else if (d->len == d->alloc) {
            d->alloc = d->alloc * 2;
            d = x_realloc (d, sizeof (xDataList) + (d->alloc - 1) * sizeof (DataElt));
        }
        if (old_d != d)
            DATA_LIST_SET_PTR (datalist, d);

        d->data[d->len].key = key_id;
        d->data[d->len].data = new_data;
        d->data[d->len].destroy = free_func;
        d->len++;
    }
    DATA_LIST_UNLOCK (datalist);

    return NULL;
}

void
x_data_list_init         (xDataList      **datalist)
{
    x_return_if_fail (datalist != NULL);

    x_atomic_ptr_set (datalist, NULL);
}    

void
x_data_list_clear        (xDataList      **datalist)
{
    xDataList   *data;
    xsize       i;

    x_return_if_fail (datalist != NULL);

    DATA_LIST_LOCK (datalist);

    data = DATA_LIST_GET_PTR (datalist);
    DATA_LIST_SET_PTR (datalist, NULL);

    DATA_LIST_UNLOCK (datalist);

    if (data) {
        for (i = 0; i < data->len; i++) {
            if (data->data[i].data && data->data[i].destroy)
                data->data[i].destroy (data->data[i].data);
        }

        x_free (data);
    }
}  

void
x_data_list_id_set_data_full (xDataList      **datalist,
                              xQuark         key_id,
                              xptr           data,
                              xFreeFunc      free_func)
{
    x_return_if_fail (datalist != NULL);
    if (!data)
        x_return_if_fail (free_func == NULL);
    if (!key_id) {
        if (data)
            x_return_if_fail (key_id > 0);
        else
            return;
    }

    data_list_set_internal (datalist, key_id, data, free_func, NULL);
}  

xptr
x_data_list_id_remove_no_notify  (xDataList  **datalist,
                                  xQuark    key_id)
{
    xptr ret_data = NULL;

    x_return_val_if_fail (datalist != NULL, NULL);

    if (key_id)
        ret_data = data_list_set_internal (datalist, key_id, NULL,
                                           (xFreeFunc) 42, NULL);

    return ret_data;
}  

void
x_data_list_foreach      (xDataList      **datalist,
                          xVisitFunc     func,
                          xptr           user_data)
{
    xDataList   *d;
    xQuark      *keys;
    xsize i, j, len;

    x_return_if_fail (datalist != NULL);
    x_return_if_fail (func != NULL);

    d = DATA_LIST_GET_PTR (datalist);
    if (d == NULL) 
        return;

    /* We make a copy of the keys so that we can handle it changing
       in the callback */
    len = d->len;
    keys = x_new (xQuark, len);
    for (i = 0; i < len; i++)
        keys[i] = d->data[i].key;

    for (i = 0; i < len; i++) {
        /* A previous callback might have removed a later item, so always check that
           it still exists before calling */
        d = DATA_LIST_GET_PTR (datalist);

        if (d == NULL)
            break;
        for (j = 0; j < d->len; j++) {
            if (d->data[j].key == keys[i]) {
                func ((xptr)d->data[i].key, d->data[i].data, user_data);
                break;
            }
        }
    }
    x_free (keys);
}  
xptr
x_data_list_id_dup_data  (xDataList      **datalist,
                          xQuark         key_id,
                          xDupFunc       dup_func,
                          xptr           user_data)
{
    xptr val = NULL;
    xptr retval = NULL;
    xDataList *d;
    DataElt *data, *data_end;

    x_return_val_if_fail (datalist != NULL, NULL);
    x_return_val_if_fail (key_id != 0, NULL);

    DATA_LIST_LOCK (datalist);

    d = DATA_LIST_GET_PTR (datalist);
    if (d) {
        data = d->data;
        data_end = data + d->len;
        while (data < data_end) {
            if (data->key == key_id) {
                val = data->data;
                break;
            }
            data++;
        }
    }

    if (dup_func)
        retval = dup_func (val, user_data);
    else
        retval = val;

    DATA_LIST_UNLOCK (datalist);

    return retval;
}
void
x_data_list_set_flags    (xDataList      **datalist,
                          xuint          flags)
{
    x_return_if_fail (datalist != NULL);
    x_return_if_fail ((flags & ~X_DATA_LIST_FLAGS_MASK) == 0);

    x_atomic_ptr_or (datalist, (xsize)flags);
}  

void
x_data_list_unset_flags  (xDataList      **datalist,
                          xuint          flags)
{
    x_return_if_fail (datalist != NULL);
    x_return_if_fail ((flags & ~X_DATA_LIST_FLAGS_MASK) == 0);

    x_atomic_ptr_and (datalist, (xsize)~flags);
}    

xuint
x_data_list_get_flags    (xDataList      **datalist)
{
    x_return_val_if_fail (datalist != NULL, 0);
    return ((xuint)(xsize)x_atomic_ptr_get (datalist) & X_DATA_LIST_FLAGS_MASK);
}

void
x_data_set_destroy       (xcptr          dataset_location)
{
    x_return_if_fail (dataset_location != NULL);

    X_LOCK (dataset_ht);
    if (dataset_ht){
        register DataSet *dataset;

        dataset = data_set_lookup_L (dataset_location);
        if (dataset)
            data_set_destroy_L(dataset);
    }
    X_UNLOCK (dataset_ht);
}

xptr
x_data_set_id_get_data   (xcptr          dataset_location,
                          xQuark         key_id)
{
    xptr retval = NULL;

    x_return_val_if_fail (dataset_location != NULL, NULL);

    X_LOCK (dataset_ht);
    if (key_id && dataset_ht){
        DataSet *dataset;

        dataset = data_set_lookup_L (dataset_location);
        if (dataset)
            retval = x_data_list_id_get_data (&dataset->datalist, key_id);
    }
    X_UNLOCK (dataset_ht);

    return retval;
}

void
x_data_set_id_set_data_full  (xcptr          location,
                              xQuark         key_id,
                              xptr           data,
                              xFreeFunc      free_func)
{
    register DataSet    *dataset;

    x_return_if_fail (location != NULL);
    if (!data)
        x_return_if_fail (free_func == NULL);
    if (!key_id) {
        if (data)
            x_return_if_fail (key_id > 0);
        else
            return;
    }

    X_LOCK (dataset_ht);
    if (!dataset_ht) {
        dataset_ht = x_hash_table_new (x_direct_hash, NULL);
    }

    dataset = data_set_lookup_L (location);
    if (!dataset) {
        dataset = x_slice_new (DataSet);
        dataset->location = location;
        x_data_list_init (&dataset->datalist);
        x_hash_table_insert (dataset_ht, 
                             (xptr) dataset->location,
                             dataset);
    }

    data_list_set_internal (&dataset->datalist, key_id, data, free_func, dataset);
    X_UNLOCK (dataset_ht);
}  

xptr
x_data_set_id_remove_no_notify   (xcptr      dataset_location,
                                  xQuark     key_id)
{
    xptr ret_data = NULL;

    x_return_val_if_fail (dataset_location != NULL, NULL);

    X_LOCK (dataset_ht);
    if (key_id && dataset_ht)
    {
        DataSet *dataset;

        dataset = data_set_lookup_L (dataset_location);
        if (dataset)
            ret_data = data_list_set_internal (&dataset->datalist, key_id, NULL, (xFreeFunc) 42, dataset);
    } 
    X_UNLOCK (dataset_ht);

    return ret_data;
}

void
x_data_set_foreach       (xcptr          location,
                          xVisitFunc     func,
                          xptr           user_data)
{
    register DataSet    *dataset;

    x_return_if_fail (location != NULL);
    x_return_if_fail (func != NULL);

    X_LOCK (dataset_ht);
    if (dataset_ht)
        dataset = data_set_lookup_L (location);
    else
        dataset = NULL;
    X_UNLOCK (dataset_ht);
    if (dataset)
        x_data_list_foreach (&dataset->datalist, func, user_data);
}
