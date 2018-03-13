#ifndef __G_SKELETON_H__
#define __G_SKELETON_H__
#include "gtype.h"
X_BEGIN_DECLS


#define G_TYPE_SKELETON              (g_skeleton_type())
#define G_SKELETON(object)           (X_INSTANCE_CAST((object), G_TYPE_SKELETON, gSkeleton))
#define G_SKELETON_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_SKELETON, gSkeletonClass))
#define G_IS_SKELETON(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_SKELETON))
#define G_SKELETON_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_SKELETON, gSkeletonClass))


gSkeleton*  g_skeleten_new          (xcstr          file,
                                     xcstr          group,
                                     xcstr          first_property,
                                     ...);

gSkeleton*  g_skeleton_get          (xcstr          name);

gAnimation* g_skeleton_add_animation(gSkeleton      *skeleton,
                                     xcstr          name,
                                     xcstr          first_property,
                                     ...);

void        g_skeleton_attach       (gSkeleton      *skeleton,
                                     xptr           client);

void        g_skeleton_detach       (gSkeleton      *skeleton,
                                     xptr           client);

void        g_skeleton_states       (gSkeleton      *skeleton,
                                     gStateSet      *states);

void        g_skeleton_pose         (gSkeleton      *skeleton,
                                     xptr           client,
                                     gStateSet      *states);

gmat4*      g_skeleton_mat4         (gSkeleton      *skeleton,
                                     xptr           client);

xPtrArray*  g_skeleton_bones        (gSkeleton      *skeleton);

void        g_skeleton_write        (gSkeleton      *skeleton,
                                     xFile          *file,
                                     xbool          binary);

struct _gSkeleton
{
    xResource           parent;
    xptr                priv;
};

struct _gSkeletonClass
{
    xResourceClass      parent;
};


xType       g_skeleton_type         (void);

X_END_DECLS
#endif /* __G_SKELETON_H__ */
