#ifndef __G_ANIM_H__
#define __G_ANIM_H__
#include "gtype.h"
X_BEGIN_DECLS
#define G_TYPE_SKELETON              (g_skeleton_type())
#define G_SKELETON(object)           (X_INSTANCE_CAST((object), G_TYPE_SKELETON, gSkeleton))
#define G_SKELETON_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_SKELETON, gSkeletonClass))
#define G_IS_SKELETON(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_SKELETON))
#define G_SKELETON_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_SKELETON, gSkeletonClass))


void        g_animation_apply           (gAnimation     *animation,
                                         greal          time,
                                         greal          weight,
                                         greal          scale);

void        g_animation_skeleton        (gAnimation     *animation,
                                         gSkeleton      *skeleton,
                                         greal          time,
                                         greal          weight,
                                         greal          scale);

void        g_animation_entity          (gAnimation     *animation,
                                         gEntity        *entity,
                                         greal          time,
                                         greal          weight,
                                         xbool          software);

gStateSet*  g_states_new                (void);

xint        g_states_unref              (gStateSet      *states);

gStateSet*  g_states_ref                (gStateSet      *states);

#define     g_states_dirty(states)      (states)->stamp++

gState*     g_states_insert             (gStateSet      *states,
                                         xQuark         name,
                                         greal          length);

gState*     g_states_get                (gStateSet      *states,
                                         xcstr          name);

typedef enum {
    G_ANIMATION_NONE,
    G_ANIMATION_SKELETON,
    G_ANIMATION_MORPH,
    G_ANIMATION_POSE,
} gAnimationType;

struct _gAnimation
{
    xArray          *node_track;
    xArray          *vertex_track;
    xArray          *numeric_track;
    greal           length;     /* 动画时间长度 */
    xArray          *times;     /* 关键帧时刻表 */
    xuint           lerp_flags; /* 动画插值方式 */
    xbool           dirty;
};

struct _gState
{
    gStateSet       *parent;
    greal           time;
    greal           length;
    greal           weight;
    xbool           loop;
    xbool           enable;
};

struct _gStateSet
{
    xuint           stamp;
    xHashTable      *table;
};



X_END_DECLS
#endif /* __G_ANIM_H__ */

