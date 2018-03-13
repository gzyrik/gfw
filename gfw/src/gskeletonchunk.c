#include "config.h"
#include "gskeleton-prv.h"
#include "ganimation.h"
#include "gnode.h"
#include "ganim-prv.h"
enum {
    M_SKELETON                       = 0x1100,
    SKELETON_HEADER                  = 0x1000,
    SKELETON_BONE                    = 0x2000,
    SKELETON_BONE_PARENT             = 0x3000,

    SKELETON_ANIMATION              = 0x4000,
    SKELETON_ANIMATION_TRACK        = 0x4100,
    SKELETON_ANIMATION_TRACK_KEYFRAME= 0x4110,

    SKELETON_ANIMATION_LINK         = 0x5000,
};

static xptr
skeleton_get (gSkeleton *skeleton, xCache *cache, xcstr group)
{
    return skeleton;
}
static xptr
bone_get (gSkeleton *skeleton, xCache *cache, xcstr group)
{
    xcstr name;
    xuint16 handle;
    gNode *node;
    greal tmp[4];
    gxform xform;
    struct SkeletonPrivate *priv;

    priv = SKELETON_PRIVATE (skeleton);
    name = x_cache_gets (cache);
    handle = x_cache_geti16 (cache);
    x_ptr_array_resize (priv->bones, handle + 1);
    node = priv->bones->data[handle];
    if (!node) {
        node = x_object_new (G_TYPE_NODE, "name", name, NULL);
        priv->bones->data[handle] = node;
    }
    x_cache_get (cache, tmp, sizeof(greal), 3);
    tmp[3] = 0;
    xform.offset = g_vec_load (tmp);

    x_cache_get (cache, tmp, sizeof(greal), 4);
    xform.rotate = g_vec_load (tmp);

    x_cache_get (cache, tmp, sizeof(greal), 3);
    tmp[3] = 0;
    xform.scale = g_vec_load (tmp);

    g_node_place(node, &xform);
    return NULL;
}
static void
bone_put              (xFile          *file,
                       xuint16        handle,
                       gNode          *bone)
{
    greal tmp[4];
    const gxform* xform;
    xstr name;
    const xsize tag = x_chunk_begin (file, SKELETON_BONE);

    x_object_get(bone, "name", &name, NULL);
    x_file_puts (file, name, TRUE);
    x_free(name);

    x_file_puti16 (file, handle);

    xform = g_node_xform (bone);
    g_vec_store (xform->offset, tmp);
    x_file_put (file, tmp, sizeof(greal), 3);

    g_vec_store (xform->rotate, tmp);
    x_file_put (file, tmp, sizeof(greal), 4);

    g_vec_store (xform->scale, tmp);
    x_file_put (file, tmp, sizeof(greal), 3);

    x_chunk_end (file, tag);
}
static xptr
bone_parent_get (gSkeleton *skeleton, xCache *cache, xcstr group)
{
    gNode *node, *pnode;
    xuint16 handle, phandle;
    xPtrArray* bones;

    bones = g_skeleton_bones(skeleton);
    x_cache_get (cache, &handle, sizeof(xuint16), 1);
    x_cache_get (cache, &phandle, sizeof(xuint16), 1);

    x_return_val_if_fail (handle < bones->len, NULL);
    x_return_val_if_fail (phandle < bones->len, NULL);

    node = bones->data[handle];
    pnode = bones->data[phandle];

    x_return_val_if_fail (node != NULL, NULL);
    x_return_val_if_fail (pnode != NULL, NULL);

    g_node_attach (node, pnode);

    return NULL;
}
static void
bone_parent_put (xFile *file, xuint16 handle,xuint16 phandle)
{
    const xsize tag = x_chunk_begin (file, SKELETON_BONE_PARENT);
    x_file_puti16(file, handle);
    x_file_puti16(file, phandle);
    x_chunk_end (file, tag);
}
static xptr
animation_get(gSkeleton *skeleton, xCache *cache, xcstr group)
{
    xcstr name;
    greal length;
    gAnimation *anim;

    name = x_cache_gets (cache);
    anim = g_skeleton_add_animation (skeleton, name, NULL);
    x_cache_get (cache, &length, sizeof(greal), 1);
    x_object_set(anim,"length", length, NULL);
    return anim;
}
static xptr
track_get(gAnimation *anim, xCache *cache, xcstr group)
{
    struct NodeTrack *track;
    xuint16 handle;

    x_cache_get (cache, &handle, sizeof(xuint16), 1);
    if (!anim->node_track)
        anim->node_track = x_array_new (sizeof(struct NodeTrack));
    track = x_array_append1 (anim->node_track, NULL);
    //track->node = g_skeleton_get_bone (skeleton, handle);
    return track;
}
static xptr
keyframe_get(struct NodeTrack *track, xCache *cache, xcstr group)
{
    greal tmp[4];
    struct XFormFrame *keyframe;

    if (!track->frames.keytimes)
        track->frames.keytimes = x_array_new(sizeof(greal));
    if (!track->frames.keyframes)
        track->frames.keyframes = x_array_new(sizeof(struct XFormFrame));

    x_cache_get (cache, tmp, sizeof(greal), 1);
    x_array_append1 (track->frames.keytimes, tmp);
    keyframe = x_array_append1 (track->frames.keyframes, NULL);

    x_cache_get (cache, tmp, sizeof(greal), 4);
    keyframe->xform.rotate = g_vec_load (tmp);

    x_cache_get (cache, tmp, sizeof(greal), 3);
    tmp[3] = 0;
    keyframe->xform.offset = g_vec_load (tmp);

    if (x_cache_offset(cache) <= sizeof(xfloat)*3) {
        x_cache_get (cache, tmp, sizeof(xfloat), 3);
        tmp[3] = 1;
        keyframe->xform.scale = g_vec_load (tmp);
    }
    else {
        keyframe->xform.scale = G_VEC_1;
    }
    return NULL;
}
void
_g_skeleton_chunk_register (void)
{
    xChunk *skeleton, *animation, *track;

    skeleton = x_chunk_set (NULL, M_SKELETON, skeleton_get, NULL);
    x_chunk_set (skeleton, SKELETON_BONE, bone_get, NULL); 
    x_chunk_set (skeleton, SKELETON_BONE_PARENT, bone_parent_get, NULL); 
    animation = x_chunk_set (skeleton, SKELETON_ANIMATION, animation_get, NULL); 
    track = x_chunk_set (animation, SKELETON_ANIMATION_TRACK, track_get, NULL); 
    x_chunk_set (track, SKELETON_ANIMATION_TRACK_KEYFRAME, keyframe_get, NULL); 
}
void
_g_skeleton_chunk_put   (gSkeleton      *skeleton,
                         xFile          *file)
{
    xsize i;
    xPtrArray *bones;
    struct SkeletonPrivate *priv;

    const xsize tag = x_chunk_begin (file, M_SKELETON);
    priv = SKELETON_PRIVATE (skeleton);
    bones = priv->bones;
    for (i=0; i<bones->len; ++i) {
        gNode *bone = bones->data[i];
        if (bone)
            bone_put (file, i, bone);
    }
    for (i=0; i<bones->len; ++i) {
        xsize j;
        gNode *bone = bones->data[i];
        if (bone && bone->parent) {
            for (j=0;j<bones->len;++j) {
                if (bone->parent == bones->data[j]){
                    bone_parent_put (file, i, j);
                    break;
                }
            }
        }
    }
    x_chunk_end (file, tag);
}
