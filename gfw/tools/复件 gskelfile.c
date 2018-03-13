#include "config.h"
#include "gskelfile.h"
#include "gskeleton.h"
#include "gnode.h"
#include "ganim-prv.h"
#include <string.h>

xbool _g_skeleton_ogre = 0;
#define FLIP_SKELETON_HEADER       0x0010
#define HEAD_COUNT          6
static xuint16
read_head               (xCache         *cache,
                         xuint32        *count)
{
    xuint16 id;

    if (1 != x_cache_get (cache, &id, sizeof(xuint16), 1))
        return 0;
    if (1 != x_cache_get (cache, count, sizeof(xuint32), 1))
        return 0;
    *count += cache->count - HEAD_COUNT;
    return id;
}
static xuint32
write_head              (xFile          *file,
                         xuint16        id,
                         xuint32        count)
{
    x_file_put (file, &id, sizeof(xuint16), 1);
    x_file_put (file, &count, sizeof(xuint32), 1);
    return HEAD_COUNT;
}

#define BEGIN_READ_CHECK(id, cache, count, max_count)                   \
    xbool __no_handle = FALSE;                                          \
if (!_g_skeleton_ogre && count > max_count){                            \
    x_error ("file %s: line %d: "                                       \
             "skeleton chunk[x%x] end(%d) over limit(%d)",              \
             __FILE__, __LINE__, id, count, max_count);                 \
}

#define DEFAULT_READ_CHECK() __no_handle = TRUE;break

#define END_READ_CHECK(id, cache, count, max_count)                     \
    if (__no_handle) {/* no handle this id */                           \
        if (!_g_skeleton_ogre) {                                        \
            x_warning("file %s: line %d: unknown skeleton chunk[x%x]",  \
                      __FILE__, __LINE__, id);                          \
            x_cache_skip (cache, count - cache->count);                 \
        } else {                                                        \
            x_cache_back (cache, HEAD_COUNT);                           \
            break;                                                      \
        }                                                               \
    } else if (max_count == cache->count) {                             \
        break;                                                          \
    } else if (count != cache->count) {                                 \
        if (!_g_skeleton_ogre) {                                        \
            if (count > cache->count) {                                 \
                x_warning("file %s: line %d: "                          \
                          "skeleton chunk[x%x] not end(%d) at bound(%d)",\
                          __FILE__, __LINE__, id, cache->count, count); \
                x_cache_skip (cache, count - cache->count);             \
            } else if (count < cache->count) {                          \
                x_error ("file %s: line %d: "                           \
                         "skeleton chunk[x%x] end(%d) over bound(%d)",  \
                         __FILE__, __LINE__, id, cache->count, count);  \
            }                                                           \
        }                                                               \
        else {                                                          \
            x_critical ("file %s: line %d: "                            \
                        "skeleton chunk[x%x] not end(%d) at bound(%d)", \
                        __FILE__, __LINE__, id, cache->count, count);   \
            cache->count = count;                                       \
        }                                                               \
    }

#define BEGIN_WRITE_CHECK(file, id, count, max_count)                   \
    xsize __fcount = x_file_tell (file);                                \
if (count > max_count) {                                                \
    x_error ("file %s: line %d: "                                       \
             "skeleton chunk[%s] end(%d) over bound(%d)",               \
             __FILE__, __LINE__, #id, count, max_count);                \
}                                                                       \
max_count = count;

#define END_WRITE_CHECK(file, id, count, max_count)                     \
    __fcount = x_file_tell (file) - __fcount;                           \
x_assert(!max_count);                                                   \
if (__fcount != count) {                                                \
    x_error ("file %s: line %d: "                                       \
             "skeleton chunk[%s] not end(%d) at bound(%d)",             \
             __FILE__, __LINE__, #id, __fcount, count);                 \
}

static void
read_bone               (xCache         *cache,
                         xuint32        max_count,
                         gSkeleton      *skeleton)
{
    xcstr name;
    xuint16 handle;
    gNode *node;
    greal tmp[4];
    gxform xform;

    name = x_cache_gets (cache);
    x_cache_get (cache, &handle, sizeof(xuint16), 1);
    x_ptr_array_resize (skeleton->bones, handle + 1);
    node = skeleton->bones->data[handle];
    if (!node) {
        node = x_object_new (G_TYPE_NODE, "name", name, NULL);
        skeleton->bones->data[handle] = node;
    }
    x_cache_get (cache, tmp, sizeof(greal), 3);
    tmp[3] = 0;
    xform.offset = g_vec_load (tmp);

    x_cache_get (cache, tmp, sizeof(greal), 4);
    xform.rotate = g_vec_load (tmp);

    if (cache->count + sizeof(greal)*3 <= max_count) {
        x_cache_get (cache, tmp, sizeof(greal), 3);
        tmp[3] = 0;
        xform.scale = g_vec_load (tmp);
    }
    else {
        xform.scale = G_VEC_1;
    }
    g_node_place(node, &xform);
}

static void
read_bone_parent        (xCache         *cache,
                         xuint32        max_count,
                         gSkeleton      *skeleton)
{
    gNode *node, *pnode;
    xuint16 handle, phandle;

    x_cache_get (cache, &handle, sizeof(xuint16), 1);
    x_cache_get (cache, &phandle, sizeof(xuint16), 1);

    x_return_if_fail (handle < skeleton->bones->len);
    x_return_if_fail (phandle < skeleton->bones->len);

    node = skeleton->bones->data[handle];
    pnode = skeleton->bones->data[phandle];
    x_return_if_fail (node != NULL);
    x_return_if_fail (pnode != NULL);

    g_node_set_parent (node, pnode);
}
static void
read_keyframe           (xCache         *cache,
                         xuint32        max_count,
                         struct NodeTrack *track)
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

    if (cache->count + sizeof(greal)*3 <= max_count) {
        x_cache_get (cache, tmp, sizeof(greal), 3);
        tmp[3] = 1;
        keyframe->xform.scale = g_vec_load (tmp);
    }
    else {
        keyframe->xform.scale = G_VEC_1;
    }
}

static void
read_track              (xCache         *cache,
                         xuint32        max_count,
                         gAnimation     *anim,
                         gSkeleton      *skeleton)
{
    struct NodeTrack *track;
    xuint16 handle;
    xuint16 id;
    xuint32 count;

    x_cache_get (cache, &handle, sizeof(xuint16), 1);
    if (!anim->node_track)
        anim->node_track = x_array_new (sizeof(struct NodeTrack));
    track = x_array_append1 (anim->node_track, NULL);
    track->node = skeleton->bones->data[handle];

    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, max_count);
        switch (id) {
        case SKELETON_ANIMATION_TRACK_KEYFRAME:
            read_keyframe (cache, count, track);
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, max_count);
    }
}
static void
read_animation          (xCache         *cache,
                         xuint32        max_count,
                         gSkeleton      *skeleton)
{
    xuint16 id;
    xuint32 count;
    xQuark name;
    greal length;
    gAnimation *anim;

    anim = x_malloc0(sizeof(gAnimation));
    name = x_quark_from(x_cache_gets (cache));
    x_hash_table_insert (skeleton->animations,X_UINT_TO_PTR(name),anim);

    x_cache_get (cache, &length, sizeof(greal), 1);
    anim->length = length;

    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, max_count);
        switch (id) {
        case SKELETON_ANIMATION_TRACK:
            read_track (cache, count, anim, skeleton);
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, max_count);
    }
}

static void
read_anim_link          (xCache         *cache,
                         xuint32        max_count,
                         gSkeleton      *skeleton)
{
}
void
g_skeleton_read         (gSkeleton      *skeleton,
                         xCache         *cache)
{
    xuint16 id;
    xuint32 count;

    x_return_if_fail (cache != NULL);
    x_return_if_fail (G_IS_SKELETON (skeleton));
    x_return_if_fail (X_RESOURCE(skeleton)->state != X_RESOURCE_LOADED);

    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, X_MAXUINT32);
        switch (id) {
        case SKELETON_BONE:
            read_bone (cache, count, skeleton);
            break;
        case SKELETON_BONE_PARENT:
            read_bone_parent (cache, count, skeleton);
            break;
        case SKELETON_ANIMATION:
            read_animation (cache, count, skeleton);
            break;
        case FLIP_SKELETON_HEADER:
            cache->flip_endian = TRUE;
            x_flip_endian(&count, sizeof(xuint32), 1);
        case SKELETON_HEADER:
            /* skip header */
            if (!_g_skeleton_ogre) {
                x_cache_skip(cache,count - cache->count);
            }
            else {
                x_cache_back (cache, sizeof(xuint32));
                x_cache_gets(cache);
                count = cache->count;
            }
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, X_MAXUINT32);
    }
}
static xint
get_bone_handle         (gSkeleton      *skeleton,
                         gNode          *bone)
{
    xsize i;
    for (i=0; i <skeleton->bones->len; ++i) {
        if (skeleton->bones->data[i] == bone)
            return i;
    }
    return -1;
}
static xuint32
calc_comment            (xcstr          comment)
{
    xuint32 count = HEAD_COUNT;
    xsize slen = 1;
    if (comment)
        slen += strlen (comment);
    count += slen;
    return count;
}
static xuint32
write_comment           (xFile          *file,
                         xcstr          comment,
                         xuint32        max_count)
{
    const xuint32 count = calc_comment (comment);

    BEGIN_WRITE_CHECK (file, SKELETON_HEADER, count, max_count);

    max_count -= write_head (file, SKELETON_HEADER, count);
    max_count -= x_file_puts (file, comment, TRUE);

    END_WRITE_CHECK (file, SKELETON_HEADER, count, max_count);
    return count;
}
static xuint32
calc_bone_size          (gNode          *bone)
{
    xuint32 count = HEAD_COUNT;
    /* bone name */
    if (bone->name) count += strlen (bone->name);
    count += 1;
    /* bone handle */
    count += sizeof(xuint16);
    /* bone offset */
    count += sizeof(greal)*3;
    /* bone rotate */
    count += sizeof(greal)*4;
    /* bone scale */
    if (!g_vec_aeq_s (bone->local.scale, G_VEC_1))
        count += sizeof(greal)*3;
    return count;
}
static xuint32
write_bone              (xFile          *file,
                         xuint16        handle,
                         gNode          *bone,
                         xuint32        max_count)
{
    greal tmp[4];
    const xuint32 count = calc_bone_size (bone);

    BEGIN_WRITE_CHECK (file, SKELETON_BONE, count, max_count);

    max_count -= write_head (file, SKELETON_BONE, count);
    max_count -= x_file_puts (file, bone->name, TRUE);
    max_count -= x_file_puti16 (file, handle);
    g_vec_store (bone->local.offset, tmp);
    max_count -= x_file_put (file, tmp, sizeof(greal), 3);
    g_vec_store (bone->local.rotate, tmp);
    max_count -= x_file_put (file, tmp, sizeof(greal), 4);
    if (!g_vec_aeq_s (bone->local.scale, G_VEC_1)) {
        g_vec_store (bone->local.scale, tmp);
        max_count -= x_file_put (file, tmp, sizeof(greal), 3);
    }

    END_WRITE_CHECK (file, SKELETON_BONE, count, max_count);
    return count;
}
static xuint32
calc_bone_parent_size   ()
{
    xuint32 count = HEAD_COUNT;
    count += sizeof(xuint16);
    count += sizeof(xuint16);
    return count;
}
static xuint32
write_bone_parent       (xFile          *file,
                         xuint16        handle,
                         xuint16        phandle,
                         xuint32        max_count)
{
    const xuint32 count = calc_bone_parent_size ();

    BEGIN_WRITE_CHECK (file, SKELETON_BONE_PARENT, count, max_count);

    max_count -= write_head (file, SKELETON_BONE_PARENT, count);
    max_count -= x_file_puti16 (file, handle);
    max_count -= x_file_puti16 (file, phandle);

    END_WRITE_CHECK (file, SKELETON_BONE_PARENT, count, max_count);
    return count;
}
static xuint32
calc_keyframe_size      (struct XFormFrame *keyframe)
{
    xuint32 count = HEAD_COUNT;
    /* keyframe time */
    count += sizeof(greal);
    /* keyframe rotate */
    count += sizeof(greal)*4;
    /* keyframe offset */
    count += sizeof(greal)*3;
    /* keyframe scale */
    if (!g_vec_aeq_s (keyframe->xform.scale, G_VEC_1))
        count += sizeof(greal)*3;
    return count;
}
static xuint32
write_keyframe          (xFile          *file,
                         greal          time,
                         struct XFormFrame *keyframe,
                         xuint32        max_count)
{
    greal tmp[4];
    const xuint32 count = calc_keyframe_size (keyframe);

    BEGIN_WRITE_CHECK (file, SKELETON_ANIMATION_TRACK_KEYFRAME, count, max_count);

    max_count -= write_head (file, SKELETON_ANIMATION_TRACK_KEYFRAME, count);
    max_count -= x_file_putf32 (file, time);
    g_vec_store (keyframe->xform.rotate, tmp);
    max_count -= x_file_put (file, tmp, sizeof(greal), 4);
    g_vec_store (keyframe->xform.offset, tmp);
    max_count -= x_file_put (file, tmp, sizeof(greal), 3);
    if (!g_vec_aeq_s (keyframe->xform.scale, G_VEC_1)) {
        g_vec_store (keyframe->xform.scale, tmp);
        max_count -= x_file_put (file, tmp, sizeof(greal), 3);
    }

    END_WRITE_CHECK (file, SKELETON_ANIMATION_TRACK_KEYFRAME, count, max_count);

    return count;
}
static xuint32
calc_track_size         (struct FrameTrack *track)
{
    xsize i;
    xuint32 count = HEAD_COUNT;
    /* node_track node handle */
    count += sizeof(xuint16); 
    for (i=0; i<track->keyframes->len; ++i) {
        struct XFormFrame *keyframe;
        keyframe = &x_array_index (track->keyframes, struct XFormFrame, i);
        count += calc_keyframe_size (keyframe);
    }
    return count;
}
static xuint32
write_track             (xFile          *file,
                         xuint16        handle,
                         struct FrameTrack *track,
                         xuint32        max_count)
{
    xsize i;
    const xuint32 count = calc_track_size (track);

    BEGIN_WRITE_CHECK (file, SKELETON_ANIMATION_TRACK, count, max_count);

    max_count -= write_head (file, SKELETON_ANIMATION_TRACK, count);
    max_count -= x_file_puti16 (file, handle);
    for (i=0;i<track->keyframes->len;++i) {
        struct XFormFrame *keyframe;
        keyframe = &x_array_index (track->keyframes, struct XFormFrame, i);
        max_count -= write_keyframe (file, x_array_index(track->keytimes, greal, i),
                                     keyframe, max_count);
    }

    END_WRITE_CHECK (file, SKELETON_ANIMATION_TRACK, count, max_count);

    return count;
}
static xuint32
calc_animation_size     (gSkeleton      *skeleton,
                         xQuark         name,
                         gAnimation     *anim)
{
    xsize i;
    xuint32 count = HEAD_COUNT;
    /* name */
    count += strlen (x_quark_to (name)) + 1;
    /* time length */
    count += sizeof (greal);
    /* node track */
    for (i=0;i<anim->node_track->len;++i) {
        struct NodeTrack *track;
        xint handle;
        track = &x_array_index (anim->node_track, struct NodeTrack, i);
        handle = get_bone_handle (skeleton, track->node);
        if (handle >= 0)
            count += calc_track_size (&track->frames);
    }
    return count;
}
struct Cxt
{
    gSkeleton   *skeleton;
    xuint32     max_count;
    xFile       *file;
};
static xuint32
write_animation         (xQuark         name,
                         gAnimation     *anim,
                         struct Cxt     *cxt)
{
    xsize i;
    xFile *file = cxt->file;
    gSkeleton *skeleton= cxt->skeleton;
    xuint max_count = cxt->max_count;
    const xuint32 count = calc_animation_size (skeleton, name, anim);

    BEGIN_WRITE_CHECK (file, SKELETON_ANIMATION, count, max_count);

    max_count -= write_head (file, SKELETON_ANIMATION, count);
    max_count -= x_file_puts (file, x_quark_to(name), TRUE);
    max_count -= x_file_putf32 (file,anim->length);
    for (i=0;i<anim->node_track->len;++i) {
        struct NodeTrack *track;
        xint handle;
        track = &x_array_index (anim->node_track, struct NodeTrack, i);
        handle = get_bone_handle (skeleton, track->node);
        if (handle >= 0)
            max_count -= write_track (file, handle, &track->frames, count);
    }
    END_WRITE_CHECK (file, SKELETON_ANIMATION, count, max_count);
    return count;
}

void
g_skeleton_write        (gSkeleton      *skeleton,
                         xFile          *file,
                         xcstr          comment)
{
    xsize i;
    struct Cxt cxt;
    x_return_if_fail (X_IS_FILE (file));
    x_return_if_fail (G_IS_SKELETON (skeleton));
    x_return_if_fail (X_RESOURCE(skeleton)->state == X_RESOURCE_LOADED);

    write_comment (file, comment, X_MAXUINT32);

    for (i=0; i<skeleton->bones->len; ++i) {
        gNode *bone = skeleton->bones->data[i];
        if (bone)
            write_bone (file, i, bone, X_MAXUINT32);
    }
    for (i=0; i<skeleton->bones->len; ++i) {
        gNode *bone = skeleton->bones->data[i];
        if (bone && bone->parent) {
            xint phandle;
            phandle = get_bone_handle (skeleton, bone->parent);
            if (phandle >= 0)
                write_bone_parent (file, i, phandle, X_MAXUINT32);
        }
    }
    cxt.file = file;
    cxt.max_count = X_MAXUINT32;
    cxt.skeleton = skeleton;
    x_hash_table_foreach (skeleton->animations,
                          (xHFunc)write_animation, &cxt);
}
