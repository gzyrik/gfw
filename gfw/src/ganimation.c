#include "config.h"
#include "ganim-prv.h"
#include "gnode.h"
#include "gskeleton.h"
static xint
time_cmp                (greal              *v1,
                         greal              *v2)
{
    if (g_real_aeq (*v1, *v2))
        return 0;
    return *v1 > *v2;
}
static void
collect_frame_times     (struct FrameTrack  *track,
                         xArray             *times)
{
    xsize i;
    for (i=0;i<track->keyframes->len;++i) {
        greal time = x_array_index (track->keytimes, greal, i);
        x_array_insert_sorted (times, &time, &time,
                               (xCmpFunc)time_cmp, FALSE);
    }
}

static void
build_frame_map         (struct FrameTrack  *track,
                         xArray             *times)
{
    xsize i,j;

    if (!track->frames_map)
        track->frames_map = x_array_new_full (sizeof(xsize),times->len, NULL);

    x_array_resize (track->frames_map,times->len);
    for (j=i=0; j<times->len; ++j) {
        x_array_index (track->frames_map, xsize, j) = i;

        for (;i<track->keyframes->len;++i) {
            greal time = x_array_index (track->keytimes, greal, i); 
            if (time > x_array_index (times, greal, j))
                break;
        }
    }
}
static greal 
get_key_frame           (gAnimation         *animation,
                         struct FrameTrack  *track,
                         struct TimePos     time_pos,
                         xsize              *current,
                         xsize              *next)
{
    xsize i;
    greal t1, t2;
    greal time;
    /* wrap time */
    time = time_pos.time;
    while ( time > animation->length)
        time -= animation->length;
    if (time_pos.keyframe != -1) {
        i = x_array_index (track->frames_map, xsize, time_pos.keyframe);
    }
    else {
        for (i=0;i<track->frames_map->len;++i) {
            if (time < x_array_index (track->keytimes, greal, i))
                break;
        }
    }
    /* There is no keyframe after this time, wrap back to first */
    if (i == track->keyframes->len) {
        *next = 0;
        *current = track->keyframes->len-1;
    }
    else {
        *next = i;
        if (i > 0)
            *current = i-1;
        else if (time_pos.time > animation->length)
            *current =  track->keyframes->len -1;
        else
            *current = i;
    }
    if (*current == *next)
        return 0;
    t1 = x_array_index (track->keytimes, greal, *current);
    t2 = x_array_index (track->keytimes, greal, *next);
    if (t1 > t2)
        t2 += animation->length;
    return (time - t1)/(t2-t1);
}
static gxform
get_xform_frame         (gAnimation         *animation,
                         struct FrameTrack  *track,
                         struct TimePos     time_pos)
{
    greal t;
    xsize cur, next;
    t = get_key_frame (animation, track, time_pos, &cur, &next);
    if (cur == next) {
        struct XFormFrame *kf1;
        kf1 = &x_array_index (track->keyframes, struct XFormFrame, cur);
        return kf1->xform;
    }
    else {
        struct XFormFrame *xf1, *xf2;
        xf1 = &x_array_index (track->keyframes, struct XFormFrame, cur);
        xf2 = &x_array_index (track->keyframes, struct XFormFrame, next);
        return g_xform_lerp (t, &xf1->xform, &xf2->xform, animation->lerp_flags);
    }
}
static void
apply_node_track        (gAnimation         *animation,
                         struct NodeTrack   *track,
                         gNode              *node,
                         struct TimePos     time_pos,
                         greal              weight,
                         greal              scale)
{
    gxform xform;

    if (!node)
        node = track->node;
    if (!node
        || g_real_aeq0 (weight)
        || !track->frames.keyframes)
        return;

    xform = get_xform_frame (animation, &track->frames, time_pos);
    if (!g_real_aeq (weight, 1)) {
        if (animation->lerp_flags & G_LERP_SPHERICAL)
            xform.rotate = g_quat_slerp (weight, G_QUAT_EYE, xform.rotate,
                                         track->lerp_flags & G_LERP_SHORT_ROTATE);
        else
            xform.rotate = g_quat_nlerp (weight, G_QUAT_EYE, xform.rotate,
                                         track->lerp_flags & G_LERP_SHORT_ROTATE);
    }
    if (!g_real_aeq (scale, 1)
        && !g_vec_aeq_s (xform.scale, G_VEC_1)) {
        xform.scale = g_vec_lerp (g_vec(scale,scale,scale,scale), G_VEC_1, xform.scale);
    }
    xform.offset = g_vec_scale (xform.offset, weight * scale);
    g_node_affine (node, &xform);
}
static void
build_keyframe_time_list    (gAnimation     *animation)
{
    xsize i;
    struct FrameTrack *track;
    if (!animation->times)
        animation->times = x_array_new (sizeof(greal));
    animation->times->len = 0;

    if (animation->node_track) {
        for (i=0;i<animation->node_track->len;++i) {
            track = &x_array_index (animation->node_track,
                                    struct NodeTrack, i).frames;
            collect_frame_times (track, animation->times);
        }
    }
    if (animation->node_track) {
        for (i=0;i<animation->node_track->len;++i) {
            track = &x_array_index (animation->node_track,
                                    struct NodeTrack, i).frames;
            build_frame_map (track, animation->times);
        }
    }
}
static struct TimePos
get_time_pos                (gAnimation     *animation,
                             greal          time)
{
    xsize i;
    struct TimePos pos;
    if (animation->dirty || !animation->times)
        build_keyframe_time_list (animation);
    /* wrap time */
    while ( time > animation->length)
        time -= animation->length;
    /* nearest keyframe */
    for (i=0;i<animation->times->len;++i) {
        if (time <= x_array_index (animation->times, greal, i))
            break;
    }
    pos.time = time;
    pos.keyframe = i;
    return pos;
}
void
g_animaton_apply            (gAnimation     *animation,
                             greal          time,
                             greal          weight,
                             greal          scale)
{
}

void
g_animation_skeleton        (gAnimation     *animation,
                             gSkeleton      *skeleton,
                             greal          time,
                             greal          weight,
                             greal          scale)
{
    xsize i;
    struct TimePos time_pos;
    xPtrArray *bones;

    time_pos = get_time_pos (animation, time);
    bones = g_skeleton_bones (skeleton);
    for (i=0; i<animation->node_track->len;++i) {
        struct NodeTrack *track;
        track = &x_array_index (animation->node_track, struct NodeTrack, i);
        apply_node_track (animation, track,
                          bones->data[i],
                          time_pos, weight, scale);
    }
}

void
g_animation_entity          (gAnimation     *animation,
                             gEntity        *entity,
                             greal          time,
                             greal          weight,
                             xbool          software)
{
}

static void state_free (gState *state)
{
    x_slice_free (gState, state);
}
gStateSet*
g_states_new                (void)
{
    gStateSet *states;
    states = x_slice_new (gStateSet);
    states->stamp = 0;
    states->table = x_hash_table_new_full (x_direct_hash, x_direct_cmp,
                                           NULL, (xFreeFunc)state_free);
    return states;
}

xint
g_states_unref              (gStateSet      *states)
{
    xint ref_count;

    x_return_val_if_fail (states != NULL, -1);

    ref_count = x_hash_table_unref (states->table);
    if (!ref_count)
        x_slice_free (gStateSet, states);

    return ref_count;
}

gStateSet*
g_states_ref                (gStateSet      *states)
{
    x_return_val_if_fail (states != NULL, NULL);

    if (!x_hash_table_ref (states->table))
        return NULL;
    return states;
}

gState*
g_states_insert             (gStateSet      *states,
                             xQuark         name,
                             greal          length)
{
    gState* ret;

    x_return_val_if_fail (states != NULL, NULL);
    ret = x_hash_table_lookup (states->table, (xptr)name);
    if (!ret) {
        ret = x_slice_new (gState);
        ret->parent = states;
        ret->length = length;
        ret->loop   = FALSE;
        ret->weight = 1;
        ret->enable = FALSE;
        x_hash_table_insert (states->table, (xptr)name, ret);
    }

    return ret;
}


gState*
g_states_get                (gStateSet      *states,
                             xcstr          name)
{
    xQuark quark;

    x_return_val_if_fail (states != NULL, NULL);
    quark = x_quark_try (name);
    if (!quark)
        return NULL;
    return x_hash_table_lookup (states->table, (xptr)quark);
}
