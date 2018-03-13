#include "config.h"
#include "gskeleton-prv.h"
#include "ganimation.h"
#include "gnode.h"
#include "ganim-prv.h"
#include <string.h>
#define S_SKELETON      "skeleton"
#define S_BONES         "bones"
#define S_BONE          "bone"
#define S_HIERARCHY     "hierarchy"

#define S_ANIMATION     "animation"
#define S_TRACK         "track"
#define S_KEYFRAME      "keyframe"
static xptr
bone_on_enter (xPtrArray *bones, xcstr name, xcstr group, xQuark parent_name)
{
    xsize id;
    id = atoi(name);
    if (id >= bones->len)
        x_ptr_array_resize(bones, id+1);
    if (!bones->data[id]) {
        bones->data[id] = x_object_new (G_TYPE_NODE, NULL);
        x_object_sink (bones->data[id]);
    }
    return bones->data[id];
}
static xptr
bones_on_enter (gSkeleton *skeleton, xcstr name, xcstr group, xQuark parent_name)
{
    struct SkeletonPrivate *priv;

    priv = SKELETON_PRIVATE (skeleton);
    if (!priv->bones)
        priv->bones = x_ptr_array_new_full(atoi(name), x_object_unref);
    return priv->bones;
}
static void
hierarchy_on_visit(gSkeleton *skeleton, xcstr key, xcstr val)
{
    if (!strcmp(key, "parent")) {
        xsize handle, phandle;
        gNode *node, *pnode;
        struct SkeletonPrivate *priv;

        priv = SKELETON_PRIVATE (skeleton);

        sscanf(val, "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", &handle, &phandle);
        x_return_if_fail (handle < priv->bones->len);
        x_return_if_fail (phandle < priv->bones->len);

        node = priv->bones->data[handle];
        pnode = priv->bones->data[phandle];
        x_return_if_fail (node != NULL);
        x_return_if_fail (pnode != NULL);

        g_node_attach (node, pnode);
    }
}
static xptr
animation_on_enter (gSkeleton *skeleton, xcstr name, xcstr group, xQuark parent_name)
{
    return g_skeleton_add_animation (skeleton, name, NULL);
}
static xptr
track_on_enter (gAnimation *animation, xcstr name, xcstr group, xQuark parent_name)
{
    struct NodeTrack *track;
    //xuint16 handle;

    if (!animation->node_track)
        animation->node_track = x_array_new (sizeof(struct NodeTrack));
    track = x_array_append1 (animation->node_track, NULL);
    //track->node = g_skeleton_get_bone (skeleton, handle);
    return track;
}
static xptr
keyframe_on_enter(struct NodeTrack *track, xcstr name, xcstr group, xQuark parent_name)
{
    return NULL;
}
static void
keyframe_on_visit (xArray *bones, xcstr key, xcstr val)
{

}
static xptr
skeleton_on_enter (gSkeleton *skeleton, xcstr name, xcstr group, xQuark parent_name)
{
    if (!skeleton)
        skeleton = g_skeleton_get (name);
    if (!skeleton) 
        return x_object_new (G_TYPE_SKELETON, "name", name, "group", group, NULL);
    return skeleton;
}
static void
bone_script_put(gNode* node, xsize i, xFile *file)
{
    x_script_begin (file, S_BONE, "%"X_SIZE_FORMAT, i);
    x_script_end (file);
}
static void
bones_script_put(xPtrArray *bones, xFile *file)
{
    xsize i;
    x_script_begin (file, S_BONES, "%"X_SIZE_FORMAT, bones->len);

    for (i=0; i<bones->len; ++i) {
        gNode *bone = bones->data[i];
        if (bone)
            bone_script_put (bone, i, file);
    }
    x_script_end (file);
}
static void
hierarchy_script_put(xPtrArray *bones, xFile *file)
{
    xsize i;
    x_script_begin (file, S_HIERARCHY, "%"X_SIZE_FORMAT, bones->len);
    for (i=0; i<bones->len; ++i) {
        xsize j;
        gNode *bone = bones->data[i];
        if (bone && bone->parent) {
            for (j=0;j<bones->len;++j) {
                if (bone->parent == bones->data[j]){
                    x_script_write(file, "parent", "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", i,j);
                    break;
                }
            }
        }
    }
    x_script_end(file);
}
void
_g_skeleton_script_register (void)
{
    xScript *skeleton;

    x_set_script_priority ("skeletonx", 199);

    skeleton = x_script_set (NULL, x_quark_from_static(S_SKELETON),
                             skeleton_on_enter, x_object_set_str, NULL); {
        xScript *bones, *animation;
        bones = x_script_set (skeleton, x_quark_from_static(S_BONES),
                              bones_on_enter, NULL,NULL); {
            x_script_set(bones, x_quark_from_static(S_BONE),
                         bone_on_enter, x_object_set_str, NULL);
        }

        x_script_set (skeleton, x_quark_from_static(S_HIERARCHY),
                      NULL, hierarchy_on_visit, NULL); 
        animation = x_script_set (skeleton, x_quark_from_static(S_ANIMATION),
                                   animation_on_enter, x_object_set_str, NULL); {
            xScript *track;
            track = x_script_set(animation, x_quark_from_static(S_TRACK),
                                 track_on_enter, NULL, NULL); {
                x_script_set(track, x_quark_from_static(S_KEYFRAME),
                             keyframe_on_enter, keyframe_on_visit, NULL);
            }
        }

    }
}
void
_g_skeleton_script_put  (gSkeleton      *skeleton,
                         xFile          *file)
{
    struct SkeletonPrivate *priv = SKELETON_PRIVATE (skeleton);
    x_script_begin (file, S_SKELETON, "%s", priv->name);

    bones_script_put (priv->bones, file);
    hierarchy_script_put(priv->bones, file);

    x_script_end (file);
}
