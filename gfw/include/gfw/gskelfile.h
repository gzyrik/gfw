#ifndef __G_SKEL_FILE_H__
#define __G_SKEL_FILE_H__
X_BEGIN_DECLS

enum {
    SKELETON_HEADER                  = 0x1000,
    SKELETON_BONE                    = 0x2000,
    SKELETON_BONE_PARENT             = 0x3000,

    SKELETON_ANIMATION              = 0x4000,
    SKELETON_ANIMATION_TRACK        = 0x4100,
    SKELETON_ANIMATION_TRACK_KEYFRAME= 0x4110,

    SKELETON_ANIMATION_LINK         = 0x5000,
};

X_END_DECLS
#endif /* __G_SKEL_FILE_H__ */

