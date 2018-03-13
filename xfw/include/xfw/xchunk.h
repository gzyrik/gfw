#ifndef __X_CHUNK_H__
#define __X_CHUNK_H__
#include "xtype.h"
X_BEGIN_DECLS

typedef xptr (*xChunkEnter)(xptr parent, xCache *cache, xcstr group);
typedef xptr (*xChunkLeave)(xptr object, xcstr group);


void        x_chunk_import          (xCache         *cache,
                                     xcstr          group,
                                     xptr           object);

void        x_chunk_import_file     (xFile          *file,
                                     xcstr          group,
                                     xptr           object);

void        x_chunk_import_str      (xcstr          str,
                                     xcstr          group,
                                     xptr           object);

xChunk*     x_chunk_set             (xChunk         *parent,
                                     xuint16        id,
                                     xChunkEnter    on_enter,
                                     xChunkLeave    on_leave);

xChunk*     x_chunk_get             (xChunk         *parent,
                                     xuint16        id);

void        x_chunk_link            (xChunk         *parent,
                                     xuint16        id,
                                     xChunk         *child);

xsize       x_chunk_begin           (xFile          *file,
                                     xuint16        id);

void        x_chunk_end             (xFile          *file,
                                     const xsize    mark);

#endif /* __X_CHUNK_H__ */
