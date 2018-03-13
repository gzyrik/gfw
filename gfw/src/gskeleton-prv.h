#ifndef __G_SKELETON_PRV_H__
#define __G_SKELETON_PRV_H__
#include "gskeleton.h"
X_BEGIN_DECLS

struct SkeletonPrivate
{
    xstr                name;
    xPtrArray           *bones;
    xHashTable          *animations;
    xHashTable          *clients;
};

#define SKELETON_PRIVATE(o)  ((struct SkeletonPrivate*) (G_SKELETON (o)->priv))

X_INTERN_FUNC void
_g_skeleton_chunk_put   (gSkeleton      *skeleton,
                         xFile          *file);

X_INTERN_FUNC void
_g_skeleton_script_put  (gSkeleton      *skeleton,
                         xFile          *file);

X_END_DECLS
#endif /* __G_SKELETON_PRV_H__ */
