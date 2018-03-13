#include "config.h"
#include "xchunk.h"
#include "xarchive.h"
#include <string.h>
struct _xChunk
{
    xHashTable      *children;
    xChunkEnter     on_enter;
    xChunkLeave     on_leave;
};

static xChunk _root;
static void
parse_chunk             (xCache         *cache,
                         xcstr          group,
                         xptr           object,
                         xChunk         *chunk)
{
    xuint16 id = x_cache_geti16 (cache);
    xsize size = x_cache_geti32 (cache);

    if (chunk->children) {
        chunk = x_hash_table_lookup (chunk->children, X_SIZE_TO_PTR(id));
        if (chunk) {
            xptr child;

            child = chunk->on_enter(object, cache, group);
            size += x_cache_tell (cache);
            do {
                parse_chunk (cache, group, child, chunk);
            } while (x_cache_tell(cache) < size);
            chunk->on_leave (object, group);
            return;
        }
    }
    x_cache_skip (cache, size);
    x_warning ("no chunk[%x] processor", id);
}
void
x_chunk_import          (xCache         *cache,
                         xcstr          group,
                         xptr           object)
{
    x_return_if_fail (cache != NULL);
    x_return_if_fail (group != NULL);

    parse_chunk (cache, group, object, &_root);
}
void
x_chunk_import_str      (xcstr          str,
                         xcstr          group,
                         xptr           object)
{
    xCache         *cache;
    xsize           slen;

    slen = strlen (str);

    x_return_if_fail (slen >2);

    cache = x_cache_new ((xbyte*)str, slen);

    x_chunk_import (cache, group, object);

    x_cache_delete (cache);
}

void
x_chunk_import_file     (xFile          *file,
                         xcstr          group,
                         xptr           object)
{
    xCache         *cache;

    x_return_if_fail (file != NULL);

    cache = x_file_cache_new (file, 64);

    x_chunk_import (cache, group, object);

    x_cache_delete (cache);
}

xChunk*
x_chunk_set             (xChunk         *parent,
                         xuint16        id,
                         xChunkEnter    on_enter,
                         xChunkLeave    on_leave)
{
    xChunk *chunk;
    x_return_val_if_fail (id != 0, NULL);

    if (!parent)
        parent = &_root;
    if (!parent->children)
        parent->children = x_hash_table_new(x_direct_hash, x_direct_cmp);

    chunk = x_hash_table_lookup (parent->children, X_SIZE_TO_PTR(id));
    if (!chunk) {
        chunk = x_slice_new0 (xChunk);
        chunk->on_enter = on_enter;
        chunk->on_leave = on_leave;
        x_hash_table_insert (parent->children, X_SIZE_TO_PTR(id), chunk);
    }
    return chunk;
}

void
x_chunk_link            (xChunk         *parent,
                         xuint16        id,
                         xChunk         *child)
{
    xChunk *chunk;
    x_return_if_fail (id != 0);
    x_return_if_fail (child != NULL);

    if (!parent)
        parent = &_root;
    if (!parent->children)
        parent->children = x_hash_table_new(x_direct_hash, x_direct_cmp);

    chunk = x_hash_table_lookup (parent->children, X_SIZE_TO_PTR(id));
    x_return_if_fail (chunk == NULL);

    x_hash_table_insert (parent->children, X_SIZE_TO_PTR(id), child);
}

xChunk*
x_chunk_get             (xChunk        *parent,
                         xuint16       id)
{
    x_return_val_if_fail (id != 0, NULL);
    if (!parent)
        parent = &_root;
    if (parent->children)
        return x_hash_table_lookup (parent->children, X_SIZE_TO_PTR(id));

    return NULL;
}

xsize
x_chunk_begin           (xFile          *file,
                         xuint16        id)
{
    x_return_val_if_fail (id != 0, -1);

    x_file_puti16 (file, id);
    x_file_seek (file, 4, X_SEEK_CUR);
    return x_file_tell (file);
}

void
x_chunk_end             (xFile          *file,
                         const xsize    mark)
{
    xsize pos = x_file_tell (file);
    x_file_seek (file, mark-4, X_SEEK_SET);
    x_file_puti32 (file, pos - mark);
    x_file_seek (file, pos, X_SEEK_SET);
}

