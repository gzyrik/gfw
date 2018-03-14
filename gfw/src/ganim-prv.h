#ifndef __G_ANIM_PRV_H__
#define __G_ANIM_PRV_H__
#include "ganimation.h"
X_BEGIN_DECLS

struct TimePos
{
    greal               time;       /* ����ʱ��,�����������й����е�ʱ��� */
    xsize               keyframe;   /* ȫ�ֹؼ�֡���,�������й��������õĹؼ�֡�����еĴ��� */
};
struct FrameTrack
{
    xArray              *keytimes;  /* �ؼ�֡ʱ�̱� */ 
    xArray              *keyframes; /* �ؼ�֡�б� */
    xArray              *frames_map; /* ȫ�ֹؼ�֡��� -> ���عؼ���� */
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
    greal           length;     /* ����ʱ�䳤�� */
    xArray          *times;     /* �ؼ�֡ʱ�̱� */
    xuint           lerp_flags; /* ������ֵ��ʽ */
    xbool           dirty;
};
X_END_DECLS
#endif /* __G_ANIM_PRV_H__ */
