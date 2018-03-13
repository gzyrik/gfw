#include "config.h"
#include "xquark.h"
#include "xhash.h"
#include "xatomic.h"
#include "xthread.h"
#include "xmem.h"
#include "xtest.h"
#include "xmsg.h"
#include "xmem.h"
#include "xstr.h"
#include <string.h>

#define QUARK_BLOCK_SIZE         2048
#define QUARK_STRING_BLOCK_SIZE (4096 - sizeof (xsize))

X_LOCK_DEFINE_STATIC (quark_global);
static xHashTable *quark_ht = NULL;
static xstr *quarks = NULL;
static xint quark_seq_id = 0;
static xchar*quark_block = NULL;
static xsize quark_block_offset = 0;

static inline xQuark
quark_new               (xstr           str)
{
    xQuark quark;
    xstr *quarks_new;

    if (quark_seq_id % QUARK_BLOCK_SIZE == 0) {
        quarks_new = x_new (xstr, quark_seq_id + QUARK_BLOCK_SIZE);
        if (quark_seq_id != 0)
            memcpy (quarks_new, quarks, sizeof (xstr) * quark_seq_id);
        memset (quarks_new + quark_seq_id, 0, sizeof (xstr) * QUARK_BLOCK_SIZE);
        /* This leaks the old quarks array. Its unfortunate, but it allows
         * us to do lockless lookup of the arrays, and there shouldn't be that
         * many quarks in an app
         */
        x_atomic_ptr_set (&quarks, quarks_new);
    }
    if (!quark_ht) {
        x_assert (quark_seq_id == 0);
        quark_ht = x_hash_table_new (x_str_hash, x_str_cmp);
        quarks[quark_seq_id] = NULL;
        x_atomic_int_inc (&quark_seq_id);
    }

    quark = quark_seq_id;
    x_atomic_ptr_set (&quarks[quark], str);
    x_hash_table_insert (quark_ht, str, X_SIZE_TO_PTR (quark));
    x_atomic_int_inc (&quark_seq_id);

    return quark;
}

static xstr
quark_strdup            (xcstr          str)
{
    xchar *copy;
    xsize len;

    len = strlen (str) + 1;

    /* For strings longer than half the block size, fall back
       to strdup so that we fill our blocks at least 50%. */
    if (len > QUARK_STRING_BLOCK_SIZE / 2)
        return x_strdup (str);

    if (quark_block == NULL ||
        QUARK_STRING_BLOCK_SIZE - quark_block_offset < len)
    {
        quark_block = x_malloc (QUARK_STRING_BLOCK_SIZE);
        quark_block_offset = 0;
    }

    copy = quark_block + quark_block_offset;
    memcpy (copy, str, len);
    quark_block_offset += len;

    return copy;
}

static inline xQuark
quark_from_str          (xcstr          str,
                         xbool          dup)
{
    xQuark quark = 0;

    if (quark_ht)
        quark = X_PTR_TO_SIZE (x_hash_table_lookup (quark_ht, str));

    if (!quark)
    {
        quark = quark_new (dup ? quark_strdup (str) : (xstr)str);
        //TRACE(GLIB_QUARK_NEW(string, quark));
    }

    return quark;
}

xQuark
x_quark_try             (xcstr          str)
{
    xQuark quark;

    X_LOCK (quark_global);
    if (quark_ht && str)
        quark = X_PTR_TO_SIZE (x_hash_table_lookup (quark_ht, str));
    else
        quark = 0;
    X_UNLOCK (quark_global);

    return quark;

}

xQuark
x_quark_from_static     (xcstr          str)
{
    xQuark quark;

    x_return_val_if_fail (str, 0);

    X_LOCK (quark_global);
    quark = quark_from_str (str, FALSE);
    X_UNLOCK (quark_global);

    return quark;
}

xQuark
x_quark_from            (xcstr          str)
{
    xQuark quark;

    x_return_val_if_fail (str, 0);

    X_LOCK (quark_global);
    quark = quark_from_str (str, TRUE);
    X_UNLOCK (quark_global);

    return quark;
}

xistr
x_quark_str             (xQuark         quark)
{
    xcstr *strings;
    xuint seq_id;

    x_return_val_if_fail (quark, NULL);

    strings = x_atomic_ptr_get (&quarks);
    seq_id = x_atomic_int_get (&quark_seq_id);

    if (quark < seq_id)
        return strings[quark];

    return NULL;
}

xcstr
x_intern_str_try        (xcstr          str)
{
    xQuark quark;
    xcstr result;

    x_return_val_if_fail (str, NULL);
    x_return_val_if_fail (quark_ht, NULL);

    X_LOCK (quark_global);
    quark = X_PTR_TO_SIZE (x_hash_table_lookup (quark_ht, str));
    result = quarks[quark];
    X_UNLOCK (quark_global);

    return result;
}

xcstr
x_intern_str            (xcstr          str)
{
    xQuark quark;
    xcstr result;

    x_return_val_if_fail (str, NULL);

    X_LOCK (quark_global);
    quark = quark_from_str (str, TRUE);
    result = quarks[quark];
    X_UNLOCK (quark_global);

    return result;
}

xcstr
x_intern_static_str     (xcstr          str)
{
    xQuark quark;
    xcstr result;

    x_return_val_if_fail (str, NULL);

    X_LOCK (quark_global);
    quark = quark_from_str (str, FALSE);
    result = quarks[quark];
    X_UNLOCK (quark_global);

    return result;
}

/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
