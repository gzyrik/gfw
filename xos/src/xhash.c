#include "config.h"
#include "xhash.h"
#include "xmem.h"
#include "xmsg.h"
#include "xslice.h"
#include "xatomic.h"
#include <string.h>

#define HASH_TABLE_MIN_SHIFT 3  /* 1 << 3 == 8 buckets */

#define UNUSED_HASH_VALUE 0
#define TOMBSTONE_HASH_VALUE 1
#define HASH_IS_UNUSED(h_) ((h_) == UNUSED_HASH_VALUE)
#define HASH_IS_TOMBSTONE(h_) ((h_) == TOMBSTONE_HASH_VALUE)
#define HASH_IS_REAL(h_) ((h_) >= 2)

struct _xHashTable
{
    xint size;
    xint mod;
    xuint mask;
    xint nnodes;
    xint noccupied; /* nnodes + tombstones */
    xptr *keys;
    xuint*hashes;
    xptr *values;
#ifndef X_DISABLE_ASSERT
    xuint version;
#endif
    xHashFunc   hash;
    xCmpFunc    compare;
    xint        ref_count;
    xFreeFunc   key_destroy;
    xFreeFunc   value_destroy;
};

static const xint prime_mod [] =
{
    1,          /* For 1 << 0 */
    2,
    3,
    7,
    13,
    31,
    61,
    127,
    251,
    509,
    1021,
    2039,
    4093,
    8191,
    16381,
    32749,
    65521,      /* For 1 << 16 */
    131071,
    262139,
    524287,
    1048573,
    2097143,
    4194301,
    8388593,
    16777213,
    33554393,
    67108859,
    134217689,
    268435399,
    536870909,
    1073741789,
    2147483647  /* For 1 << 31 */
};

static inline void
hash_table_set_shift    (xHashTable     *hash_table,
                         xint           shift)
{
    xint i;
    xuint mask = 0;

    hash_table->size = 1 << shift;
    hash_table->mod  = prime_mod [shift];

    for (i = 0; i < shift; i++) {
        mask <<= 1;
        mask |= 1;
    }

    hash_table->mask = mask;
}

static inline xint
hash_table_find_closest_shift (xint n)
{
    xint i;

    for (i = 0; n; i++)
        n >>= 1;

    return i;
}

static inline void
hash_table_set_shift_from_size (xHashTable  *hash_table,
                                xint        size)
{
    xint shift;

    shift = hash_table_find_closest_shift (size);
    shift = MAX (shift, HASH_TABLE_MIN_SHIFT);

    hash_table_set_shift (hash_table, shift);
}

static inline void
hash_table_resize       (xHashTable       *hash_table)
{
    xptr *new_keys;
    xptr *new_values;
    xuint *new_hashes;
    xint old_size;
    xint i;

    old_size = hash_table->size;
    hash_table_set_shift_from_size (hash_table, hash_table->nnodes * 2);

    new_keys = x_new0 (xptr, hash_table->size);
    if (hash_table->keys == hash_table->values)
        new_values = new_keys;
    else
        new_values = x_new0 (xptr, hash_table->size);
    new_hashes = x_new0 (xuint, hash_table->size);

    for (i = 0; i < old_size; i++)
    {
        xuint node_hash = hash_table->hashes[i];
        xuint hash_val;
        xuint step = 0;

        if (!HASH_IS_REAL (node_hash))
            continue;

        hash_val = node_hash % hash_table->mod;

        while (!HASH_IS_UNUSED (new_hashes[hash_val]))
        {
            step++;
            hash_val += step;
            hash_val &= hash_table->mask;
        }

        new_hashes[hash_val] = hash_table->hashes[i];
        new_keys[hash_val] = hash_table->keys[i];
        new_values[hash_val] = hash_table->values[i];
    }

    if (hash_table->keys != hash_table->values)
        x_free (hash_table->values);

    x_free (hash_table->keys);
    x_free (hash_table->hashes);

    hash_table->keys = new_keys;
    hash_table->values = new_values;
    hash_table->hashes = new_hashes;

    hash_table->noccupied = hash_table->nnodes;
}

static inline void
hash_table_maybe_resize (xHashTable       *hash_table)
{
    xint noccupied = hash_table->noccupied;
    xint size = hash_table->size;

    if ((size > hash_table->nnodes * 4 && size > 1 << HASH_TABLE_MIN_SHIFT)
        || (size <= noccupied + (noccupied / 16)))
        hash_table_resize (hash_table);
}

static inline xuint
hash_table_lookup_node  (xHashTable     *hash_table,
                         xptr           key,
                         xuint          *hash_return)
{
    xuint node_index;
    xuint node_hash;
    xuint hash_value;
    xuint first_tombstone = 0;
    xbool have_tombstone = FALSE;
    xuint step = 0;

    hash_value = hash_table->hash (key);
    if (X_UNLIKELY (!HASH_IS_REAL (hash_value)))
        hash_value = 2;

    *hash_return = hash_value;

    node_index = hash_value % hash_table->mod;
    node_hash = hash_table->hashes[node_index];

    while (!HASH_IS_UNUSED (node_hash)) {
        /* We first check if our full hash values
         * are equal so we can avoid calling the full-blown
         * key equality function in most cases.
         */
        if (node_hash == hash_value) {
            xptr node_key = hash_table->keys[node_index];

            if (hash_table->compare) {
                if (!hash_table->compare (node_key, key))
                    return node_index;
            } else if (node_key == key) {
                return node_index;
            }
        } else if (HASH_IS_TOMBSTONE (node_hash) && !have_tombstone) {
            first_tombstone = node_index;
            have_tombstone = TRUE;
        }

        step++;
        node_index += step;
        node_index &= hash_table->mask;
        node_hash = hash_table->hashes[node_index];
    }

    if (have_tombstone)
        return first_tombstone;

    return node_index;
}

static inline void
hash_table_insert_node  (xHashTable       *hash_table,
                         xuint          node_index,
                         xuint          key_hash,
                         xptr           key,
                         xptr           value,
                         xbool          keep_new_key,
                         xbool          reusing_key)
{
    xuint old_hash;
    xptr old_key;
    xptr old_value;

    if (X_UNLIKELY (hash_table->keys == hash_table->values && key != value))
        hash_table->values = x_memdup (hash_table->keys,
                                       sizeof (xptr) * hash_table->size);

    old_hash = hash_table->hashes[node_index];
    old_key = hash_table->keys[node_index];
    old_value = hash_table->values[node_index];

    if (HASH_IS_REAL (old_hash)) {
        if (keep_new_key)
            hash_table->keys[node_index] = key;
        hash_table->values[node_index] = value;
    } else {
        hash_table->keys[node_index] = key;
        hash_table->values[node_index] = value;
        hash_table->hashes[node_index] = key_hash;

        hash_table->nnodes++;

        if (HASH_IS_UNUSED (old_hash)) {
            /* We replaced an empty node, and not a tombstone */
            hash_table->noccupied++;
            hash_table_maybe_resize (hash_table);
        }

#ifndef X_DISABLE_ASSERT
        hash_table->version++;
#endif
    }

    if (HASH_IS_REAL (old_hash)) {
        if (hash_table->key_destroy && !reusing_key)
            hash_table->key_destroy (keep_new_key ? old_key : key);
        if (hash_table->value_destroy)
            hash_table->value_destroy (old_value);
    }
}

static void
hash_table_remove_node  (xHashTable     *hash_table,
                         xint           i,
                         xbool          notify)
{
    xptr key;
    xptr value;

    key = hash_table->keys[i];
    value = hash_table->values[i];

    /* Erect tombstone */
    hash_table->hashes[i] = TOMBSTONE_HASH_VALUE;

    /* Be GC friendly */
    hash_table->keys[i] = NULL;
    hash_table->values[i] = NULL;

    hash_table->nnodes--;

    if (notify && hash_table->key_destroy)
        hash_table->key_destroy (key);

    if (notify && hash_table->value_destroy)
        hash_table->value_destroy (value);
}
static void
hash_table_remove_all_nodes (xHashTable     *hash_table,
                             xbool          notify)
{
    int i;
    xptr key;
    xptr value;

    hash_table->nnodes = 0;
    hash_table->noccupied = 0;

    if (!notify ||
        (hash_table->key_destroy == NULL &&
         hash_table->value_destroy == NULL)) {
        memset (hash_table->hashes, 0, hash_table->size * sizeof (xuint));
        memset (hash_table->keys, 0, hash_table->size * sizeof (xptr));
        memset (hash_table->values, 0, hash_table->size * sizeof (xptr));

        return;
    }

    for (i = 0; i < hash_table->size; i++) {
        if (HASH_IS_REAL (hash_table->hashes[i])) {
            key = hash_table->keys[i];
            value = hash_table->values[i];

            hash_table->hashes[i] = UNUSED_HASH_VALUE;
            hash_table->keys[i] = NULL;
            hash_table->values[i] = NULL;

            if (hash_table->key_destroy != NULL)
                hash_table->key_destroy (key);

            if (hash_table->value_destroy != NULL)
                hash_table->value_destroy (value);
        }
        else if (HASH_IS_TOMBSTONE (hash_table->hashes[i])) {
            hash_table->hashes[i] = UNUSED_HASH_VALUE;
        }
    }
}

xHashTable*
x_hash_table_new_full   (xHashFunc      hash,
                         xCmpFunc       key_compare,
                         xFreeFunc      key_destroy,
                         xFreeFunc      value_destroy)
{
    xHashTable *hash_table = x_slice_new (xHashTable);
    hash_table_set_shift (hash_table, HASH_TABLE_MIN_SHIFT);
    hash_table->nnodes       = 0;
    hash_table->noccupied    = 0;
    hash_table->hash         = hash ? hash : X_INT_FUNC(x_direct_hash);
    hash_table->compare      = key_compare;
    hash_table->ref_count          = 1;
#ifndef X_DISABLE_ASSERT
    hash_table->version      = 0;
#endif
    hash_table->key_destroy   = key_destroy;
    hash_table->value_destroy = value_destroy;
    hash_table->keys          = x_new0 (xptr, hash_table->size);
    hash_table->values        = hash_table->keys;
    hash_table->hashes        = x_new0 (xuint, hash_table->size);

    return hash_table;
}

xHashTable* 
x_hash_table_ref        (xHashTable     *hash_table)
{
    x_return_val_if_fail (hash_table != NULL, NULL);
    x_return_val_if_fail (x_atomic_int_get (&hash_table->ref_count) > 0, NULL);

    x_atomic_int_inc (&hash_table->ref_count);

    return hash_table;
}

xint
x_hash_table_unref      (xHashTable     *hash_table)
{
    xint ref_count;
    x_return_val_if_fail (hash_table != NULL, -1);
    x_return_val_if_fail (x_atomic_int_get (&hash_table->ref_count) > 0, -1);

    ref_count = x_atomic_int_dec (&hash_table->ref_count);
    if X_LIKELY(ref_count != 0)
        return ref_count;

    hash_table_remove_all_nodes (hash_table, TRUE);
    if (hash_table->keys != hash_table->values)
        x_free (hash_table->values);
    x_free (hash_table->keys);
    x_free (hash_table->hashes);
    x_slice_free (xHashTable, hash_table);

    return ref_count;
}

xsize
x_hash_table_size       (xHashTable     *hash_table)
{
    x_return_val_if_fail (hash_table != NULL, 0);
    return hash_table->nnodes;
}
void
x_hash_table_insert_full(xHashTable   *hash_table,
                         xptr         key,
                         xptr         value,
                         xbool        keep_new_key,
                         xbool        reusing_key)
{
    xuint key_hash;
    xuint node_index;

    x_return_if_fail (hash_table != NULL);

    node_index = hash_table_lookup_node (hash_table, key, &key_hash);

    hash_table_insert_node (hash_table, node_index,
                            key_hash, key, value,
                            keep_new_key, reusing_key);
}

xbool
x_hash_table_remove_full(xHashTable     *hash_table,
                         xptr           key,
                         xbool          notify)
{
    xuint node_index;
    xuint node_hash;

    x_return_val_if_fail (hash_table != NULL, FALSE);

    node_index = hash_table_lookup_node (hash_table, key, &node_hash);

    if (!HASH_IS_REAL (hash_table->hashes[node_index]))
        return FALSE;

    hash_table_remove_node (hash_table, node_index, notify);
    hash_table_maybe_resize (hash_table);

#ifndef X_DISABLE_ASSERT
    hash_table->version++;
#endif

    return TRUE;
}

xptr
x_hash_table_lookup     (xHashTable     *hash_table,
                         xcptr          key)
{
    xuint node_index;
    xuint node_hash;

    x_return_val_if_fail (hash_table != NULL, NULL);

    node_index = hash_table_lookup_node (hash_table, (xptr)key, &node_hash);

    return HASH_IS_REAL (hash_table->hashes[node_index])
        ? hash_table->values[node_index]
        : NULL;
}

xuint
x_hash_table_foreach_remove_full (xHashTable     *hash_table,
                                  xWalkFunc      func,
                                  xptr         user_data,
                                  xbool        notify)
{
    xuint deleted = 0;
    xint i;
#ifndef X_DISABLE_ASSERT
    xint version = hash_table->version;
#endif

    x_return_val_if_fail (hash_table != NULL, 0);
    x_return_val_if_fail (func != NULL, 0);

    for (i = 0; i < hash_table->size; i++) {
        xuint node_hash = hash_table->hashes[i];
        xptr node_key   = hash_table->keys[i];
        xptr node_value = hash_table->values[i];

        if (HASH_IS_REAL (node_hash) &&
            (* func) (node_key, node_value, user_data))
        {
            hash_table_remove_node (hash_table, i, notify);
            deleted++;
        }

#ifndef X_DISABLE_ASSERT
        x_return_val_if_fail (version == hash_table->version, 0);
#endif
    }

    hash_table_maybe_resize (hash_table);

#ifndef X_DISABLE_ASSERT
    if (deleted > 0)
        hash_table->version++;
#endif

    return deleted;
}
static inline  void
hash_table_walk         (xHashTable     *hash_table,
                         xptr           func,
                         xptr           user_data,
                         int            method,
                         xbool          foreach)
{
    xint i;
    xBoolFunc walk = (xBoolFunc)func;
    xVoidFunc cb = (xVoidFunc)func;

#ifndef G_DISABLE_ASSERT
    xint version;
#endif

    x_return_if_fail (hash_table != NULL);
    x_return_if_fail (func != NULL);

#ifndef G_DISABLE_ASSERT
    version = hash_table->version;
#endif

    for (i = 0; i < hash_table->size; i++) {
        xuint node_hash = hash_table->hashes[i];
        xptr node_key = hash_table->keys[i];
        xptr node_value = hash_table->values[i];

        if (HASH_IS_REAL (node_hash)) {
            if (method == 1){
                if (foreach)
                    cb (node_key, user_data);
                else if (!walk (node_key, user_data))
                    break;
            }
            else if (method == 2) {
                if (foreach)
                    cb (node_value, user_data);
                else if (!walk (node_value, user_data))
                    break;
            }
            else {
                if (foreach)
                    cb (node_key, node_value, user_data);
                else if (!walk (node_key, node_value, user_data))
                    break;
            }
        }

#ifndef G_DISABLE_ASSERT
        x_return_if_fail (version == hash_table->version);
#endif
    }
}
void
x_hash_table_walk_key   (xHashTable     *hash_table,
                         xWalkFunc      func,
                         xptr           user_data)
{
     hash_table_walk (hash_table, func, user_data, 1, FALSE);
}
void
x_hash_table_foreach_key(xHashTable     *hash_table,
                         xVisitFunc     func,
                         xptr           user_data)
{
    hash_table_walk (hash_table, func, user_data, 1, TRUE);
}
void
x_hash_table_walk_val   (xHashTable     *hash_table,
                         xWalkFunc      func,
                         xptr           user_data)
{
    hash_table_walk (hash_table, func, user_data, 2, FALSE);
}
void
x_hash_table_foreach_val(xHashTable     *hash_table,
                         xVisitFunc     func,
                         xptr           user_data)
{
    hash_table_walk (hash_table, func, user_data, 2, TRUE);
}
void
x_hash_table_walk       (xHashTable     *hash_table,
                         xWalkFunc      func,
                         xptr           user_data)
{
    hash_table_walk (hash_table, func, user_data, 3, FALSE);
}

void
x_hash_table_foreach    (xHashTable     *hash_table,
                         xVisitFunc     func,
                         xptr           user_data)
{
    hash_table_walk (hash_table, func, user_data, 3, TRUE);
}

xint
x_direct_hash           (xptr           key)
{
    return (xint)X_PTR_TO_SIZE(key);
}

xint
x_direct_cmp            (xptr           key1,
                         xptr           key2,
                         ...)
{
    return (xint)(X_PTR_TO_SIZE(key1) - X_PTR_TO_SIZE(key2));
}

xint
x_str_hash              (xptr           key)
{
    const signed char *p;
    xuint32 h = 5381;

    for (p = key; p && *p != '\0'; p++)
        h = (h << 5) + h + *p;

    return h;
}

/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
