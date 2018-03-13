#ifndef __G_ANIM_PRV_H__
#define __G_ANIM_PRV_H__
#include "ganimation.h"
X_BEGIN_DECLS

struct TimePos
{
    greal               time;       /* 动画时刻,即在整个运行过程中的时间点 */
    xsize               keyframe;   /* 全局关键帧序号,整个运行过程中所用的关键帧序列中的次序 */
};
struct FrameTrack
{
    xArray              *keytimes;  /* 关键帧时刻表 */ 
    xArray              *keyframes; /* 关键帧列表 */
    xArray              *frames_map; /* 全局关键帧序号 -> 本地关键序号 */
};
struct NodeTrack
{
    struct FrameTrack   frames;
    gNode               *node;
    xuint               lerp_flags;
};
struct VertexTrack
{
    struct FrameTrack   frames;
    gVertexData         *vdata;
    xbool               software;
    xbool               morph;
};

struct XFormFrame
{
    gxform              xform;
};
struct MorphFrame
{
    gVertexBuffer       *buffer;
};
struct PoseFrame
{
    xArray              *pose_ref;
};
struct PoseRef
{
    xsize               index;
    greal               influence;
};

struct AnimationPrivate
{
    xArray          *node_track;
    xArray          *vertex_track;
    xArray          *numeric_track;
    greal           length;     /* 动画时间长度 */
    xArray          *times;     /* 关键帧时刻表 */
    xuint           lerp_flags; /* 动画插值方式 */
    xbool           dirty;
};
X_END_DECLS
#endif /* __G_ANIM_PRV_H__ */
