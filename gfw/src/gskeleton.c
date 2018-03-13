#include "config.h"
#include "gskeleton.h"
#include "ganimation.h"
#include "gnode.h"
#include "ganim-prv.h"
#include "gskeleton-prv.h"
#include "gskeleton.h"

X_DEFINE_TYPE (gSkeleton, g_skeleton, X_TYPE_RESOURCE);
static xHashTable       *_skeletons;

/* skeleton property */
enum {
    PROP_0,
    PROP_NAME,
    N_PROPERTIES
};

static void
set_property            (xObject            *skeleton,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    struct SkeletonPrivate *priv;
    priv = SKELETON_PRIVATE (skeleton);

    switch(property_id) {
    case PROP_NAME:
        if (priv->name) {
            x_hash_table_remove (_skeletons, priv->name);
            x_free (priv->name);
        }
        priv->name = x_value_dup_str (value);
        if (priv->name && priv->name[0])
            x_hash_table_insert (_skeletons, priv->name, skeleton);
        break;
    }
}
static void
get_property            (xObject            *skeleton,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    struct SkeletonPrivate *priv;
    priv = SKELETON_PRIVATE (skeleton);

    switch(property_id) {
    case PROP_NAME:
        x_value_set_str (value, priv->name);
        break;
    }
}
static void
skeleton_prepare        (xResource      *resource,
                         xbool          backThread)
{
    struct SkeletonPrivate *priv;
    priv = SKELETON_PRIVATE (resource);

    if (priv->name && x_str_suffix(priv->name,".skeleton")) {
        xFile *file;

        file = x_repository_open (resource->group, priv->name, NULL);
        if (file) {
            xsize i;

            x_chunk_import_file (file, resource->group, resource);
            for (i=0;i<priv->bones->len; ++i) {
                gNode *node;
                node = x_ptr_array_index (priv->bones, gNode, i);
                g_node_save (node);
            }
        }
        else
            x_warning("can't open skeleton file `%s'", priv->name);
    }
}

static void
skeleton_load           (xResource      *resource,
                         xbool          backThread)
{
    struct SkeletonPrivate *priv;
    priv = SKELETON_PRIVATE (resource);
}
struct client
{
    gmat4           *bone_mat4;
};
static void
client_free             (struct client  *c)
{
    x_align_free(c->bone_mat4);
    x_slice_free (struct client, c);
}
static void
g_skeleton_init         (gSkeleton      *skeleton)
{
    struct SkeletonPrivate *priv;
    priv = x_instance_private(skeleton, G_TYPE_SKELETON);
    skeleton->priv = priv;

    priv->bones = x_ptr_array_new ();
    priv->clients = x_hash_table_new_full (x_direct_hash, x_direct_cmp, NULL, x_free);
    priv->animations = x_hash_table_new_full (x_direct_hash,x_direct_cmp, NULL, x_free);
}
static void
skeleton_finalize       (xObject        *object)
{

}
static void
g_skeleton_class_init   (gSkeletonClass *klass)
{
    xParam         *param;
    xObjectClass   *oclass;
    xResourceClass *rclass;

    x_class_set_private(klass, sizeof(struct SkeletonPrivate));
    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = skeleton_finalize;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = skeleton_prepare;
    rclass->load = skeleton_load;

    if (!_skeletons)
        _skeletons = x_hash_table_new (x_str_hash, x_str_cmp);

    param = x_param_str_new ("name",
                             "Name",
                             "The name of the skeleton object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_NAME, param);
}
gAnimation*
g_skeleton_add_animation(gSkeleton      *skeleton,
                         xcstr          name,
                         xcstr          first_property,
                         ...)
{
    struct SkeletonPrivate *priv;
    gAnimation     *animation = NULL;

    x_return_val_if_fail (name != NULL, NULL);
    x_return_val_if_fail (G_IS_SKELETON (skeleton), NULL);

    priv = SKELETON_PRIVATE (skeleton);

    x_hash_table_insert(priv->animations, X_SIZE_TO_PTR(x_quark_from(name)), animation);
    return animation;
}
void
g_skeleton_attach       (gSkeleton      *skeleton,
                         xptr           stub)
{
    struct client *client;
    struct SkeletonPrivate *priv;

    x_return_if_fail (G_IS_SKELETON (skeleton));
    x_return_if_fail (stub != NULL);
    priv = SKELETON_PRIVATE (skeleton);

    client = x_hash_table_lookup (priv->clients, stub);
    if (!client) {
        client = x_slice_new0 (struct client);
        x_hash_table_insert (priv->clients, (xptr)stub, client);
    }
}

void
g_skeleton_detach       (gSkeleton      *skeleton,
                         xptr           stub)
{
    struct SkeletonPrivate *priv;

    x_return_if_fail (G_IS_SKELETON (skeleton));
    x_return_if_fail (stub != NULL);
    priv = SKELETON_PRIVATE (skeleton);

    x_hash_table_remove (priv->clients, (xptr)stub);
}
static void
add_state (xptr name, gAnimation *anim, gStateSet *states)
{
    g_states_insert (states, (xQuark)name, anim->length);
}
void
g_skeleton_states       (gSkeleton      *skeleton,
                         gStateSet      *states)
{
    struct SkeletonPrivate *priv;
    x_return_if_fail (G_IS_SKELETON (skeleton));
    x_return_if_fail (states != NULL);
    priv = SKELETON_PRIVATE (skeleton);

    x_hash_table_foreach (priv->animations, add_state, states);
}

static void set_state (xptr name, gState *state, gSkeleton *skeleton)
{
    gAnimation *anim;
    struct SkeletonPrivate *priv;
    if (!state->enable) return;

    priv = SKELETON_PRIVATE (skeleton);
    anim = x_hash_table_lookup (priv->animations, name);
    if (anim)
        g_animation_skeleton (anim, skeleton, state->time, state->weight, 1);
}
static void update_bone(gNode* bone, xptr null)
{
    if (!bone->parent)
        g_node_update(bone, TRUE, FALSE);
}
void
g_skeleton_pose         (gSkeleton      *skeleton,
                         xptr           stub,
                         gStateSet      *states)
{
    xsize i;
    gNode *node;
    struct client *client;
    struct SkeletonPrivate *priv;

    x_return_if_fail (states != NULL);
    x_return_if_fail (stub != NULL);
    x_return_if_fail (G_IS_SKELETON (skeleton));
    priv = SKELETON_PRIVATE (skeleton);

    client = x_hash_table_lookup (priv->clients, stub);
    x_return_if_fail (client != NULL);

    x_ptr_array_foreach(priv->bones, g_node_restore, NULL);
    x_hash_table_foreach (states->table, set_state, skeleton);
    x_ptr_array_foreach(priv->bones, update_bone, NULL);

    if (!client->bone_mat4)
        client->bone_mat4 = x_align_malloc (sizeof(gmat4)*priv->bones->len, G_VEC_ALIGN);
    for (i=0; i<priv->bones->len; ++i) {
        node = x_ptr_array_index (priv->bones, gNode, i);
        client->bone_mat4[i] = g_mat4_xform (g_node_offseted (node));
    }
}
gmat4*
g_skeleton_mat4         (gSkeleton      *skeleton,
                         xptr           stub)
{
    struct client *client;
    struct SkeletonPrivate *priv;

    x_return_val_if_fail (G_IS_SKELETON (skeleton), NULL);
    x_return_val_if_fail (stub != NULL, NULL);
    priv = SKELETON_PRIVATE (skeleton);

    client = x_hash_table_lookup (priv->clients, stub);
    x_return_val_if_fail (client != NULL, NULL);

    return client->bone_mat4;
}
xPtrArray*
g_skeleton_bones        (gSkeleton      *skeleton)
{
    struct SkeletonPrivate *priv;

    x_return_val_if_fail (G_IS_SKELETON (skeleton), NULL);
    priv = SKELETON_PRIVATE (skeleton);

    return priv->bones;
}
gSkeleton*
g_skeleten_new          (xcstr          file,
                         xcstr          group,
                         xcstr          first_property,
                         ...)
{
    va_list argv;
    gSkeleton* skeleton;

    skeleton = g_skeleton_get (file);
    if (skeleton)
        return skeleton;

    va_start(argv, first_property);
    skeleton = x_object_new_valist1 (G_TYPE_SKELETON,
                                     first_property, argv,
                                     "name", file,
                                     "group", group,
                                     NULL);
    va_end(argv);
    return skeleton;
}

gSkeleton*
g_skeleton_get          (xcstr          name)
{
    x_return_val_if_fail (name != NULL, NULL);
    if (!_skeletons)
        return NULL;

    return x_hash_table_lookup (_skeletons, name);
}
void
g_skeleton_write        (gSkeleton      *skeleton,
                         xFile          *file,
                         xbool          binary)
{
    if (binary)
        _g_skeleton_chunk_put (skeleton, file);
    else
        _g_skeleton_script_put (skeleton, file);
}
