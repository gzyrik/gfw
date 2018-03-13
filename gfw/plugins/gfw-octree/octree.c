#include "gfw-octree.h"
struct octree*
octree_new              (void)
{
    struct octree* octree;
    octree = x_slice_new0 (struct octree);
    octree->nodes = x_ptr_array_new();
    octree->nodes->free_func = x_object_unref;
    return octree;
}
void
octree_delete           (struct octree  *octree)
{
    xsize i;
    for (i=0; i<8; ++i) {
        if (octree->children[i])
            octree_delete(octree->children[i]);
    }
    x_object_unref (octree->boundingbox);
    x_ptr_array_unref (octree->nodes);
    x_slice_free (struct octree, octree);
}
xbool
octree_twice_size       (struct octree  *octree,
                         const gaabb    *box)
{
    if (box->extent <= 0)
        return FALSE;
    return g_vec_le_s (g_aabb_size (box),
        g_aabb_half (&octree->aabb));
}
static xint
octree_location         (struct octree  *octree,
                         const gaabb    *box,
                         gvec           *pos)
{
    if (octree_twice_size (octree, box)) {
        gvec d;
        d = g_vec_sub (g_aabb_center (box),
                       g_aabb_center (&octree->aabb));
        d = g_vec_gt (d, G_VEC_0);
        if (pos) *pos = g_vec_and (d, G_VEC_1);
        return g_vec_mmsk (d);
    }
    return -1;
}
static xptr
octree_sure_child        (struct octree *octree,
                          xsize         index,
                          const  gvec   pos)
{
    gvec tmp1, tmp2;
    struct octree *child;

    x_assert (octree->depth > 0);
    child = octree->children[index];
    if (child) return child;

    child = x_slice_new0 (struct octree);
    child->nodes = x_ptr_array_new();
    child->nodes->free_func = x_object_unref;
    child->depth = octree->depth - 1;
    child->parent = octree;
    octree->children[index] = child;

    tmp1 = g_vec_mul (g_vec_sub (g_vec_1(2), pos),
                      octree->aabb.minimum);
    tmp2 = g_vec_mul (pos,
                      octree->aabb.maximum);
    child->aabb.minimum = g_vec_scale (g_vec_add (tmp1, tmp2), 0.5f);

    tmp1 = g_vec_mul (g_vec_sub (G_VEC_1, pos),
                      octree->aabb.minimum);
    tmp2 = g_vec_mul (g_vec_add (G_VEC_1, pos),
                      octree->aabb.maximum);
    child->aabb.maximum = g_vec_scale (g_vec_add (tmp1, tmp2), 0.5f);
    child->aabb.extent = 1;
    return child;
}

void
octree_attach           (struct octree  *octree,
                         octNode        *node)
{
    gaabb *box;
    xint index;
    gvec pos;
    struct octree *child;

    box = g_scene_node_wbbox (G_SCENE_NODE(node));
    x_return_if_fail (box->extent == 1);

    if (octree->depth == 0)
        index = -1;
    else
        index = octree_location (octree, box, &pos);

    if (index < 0) {
        node->octree = octree;
        x_object_sink (node);
        x_ptr_array_append1 (octree->nodes, node);
        while (octree) {
            octree->n_nodes++;
            octree = octree->parent;
        }
        return;
    }
    child = octree ->children[index];
    if (!child) 
        child = octree_sure_child (octree, index, pos);

    octree_attach (child, node);
}
void
octree_detach           (struct octree  *octree,
                         octNode        *node)
{
    if (x_ptr_array_remove_data (octree->nodes, node)) {
        while (octree) {
            octree->n_nodes--;
            octree = octree->parent;
        }
    }
}

static void
collect_nodes           (struct octree  *octree,
                         xPtrArray      *nodes)
{
    xsize i;

    if (octree->boundingbox)
        x_object_unref (octree->boundingbox);

    if (octree->nodes) {
        x_ptr_array_append (nodes,
                            octree->nodes->data,
                            octree->nodes->len);
        octree->nodes->len = 0;
        x_ptr_array_unref (octree->nodes);
    }

    for (i=0; i<8;++i) {
        if (octree->children[i]) {
            collect_nodes (octree->children[i], nodes);
            octree->children[i] = NULL;
        }
    }
    x_slice_free (struct octree, octree);
}
void
octree_set_size         (struct octree  *octree,
                         const gvec     size)
{
    xsize i;
    octNode *n;
    xPtrArray *nodes;

    if (g_vec_aeq_s (size, octree->aabb.maximum))
        return;

    octree->aabb.extent = 1;
    octree->aabb.maximum = size;
    octree->aabb.minimum = g_vec_negate (size);
    if (!octree->n_nodes)
        return;

    nodes = x_ptr_array_new_full (octree->n_nodes, x_object_unref);
    for (i=0; i<8;++i) {
        if (octree->children[i]) {
            collect_nodes (octree->children[i], nodes);
            octree->children[i] = NULL;
        }
    }
    for (i=0;i<nodes->len;++i) {
        n = x_ptr_array_index (nodes, octNode, i);
        octree_attach (octree, n);
    }
    x_ptr_array_unref (nodes);
}
void
octree_set_depth        (struct octree  *octree,
                         xsize          depth)
{
    xsize i;
    if (octree->depth == depth)
        return;
    if (!octree->depth) {
        xPtrArray *nodes = octree->nodes;
        octree->depth = depth;
        octree->nodes = x_ptr_array_new_full (nodes->len, NULL);
        for (i=0; i<nodes->len; ++i)
            octree_attach (octree, nodes->data[i]);
        x_ptr_array_unref (nodes);
    }
    else if (!depth) {
        octree->depth = 0;
        for (i=0; i<8;++i) {
            if (octree->children[i]) {
                collect_nodes (octree->children[i], octree->nodes);
                octree->children[i] = NULL;
            }
        }
    }
    else {
        octree->depth = depth;
        for (i=0; i<8;++i) {
            if (octree->children[i])
                octree_set_depth (octree->children[i], depth-1);
        }
    }
}
gBoundingBox*
octree_boundingbox    (struct octree  *octree)
{
    if (!octree->boundingbox) {
        octree->boundingbox = x_object_new (G_TYPE_BOUNDINGBOX, NULL);
        octree->boundingbox->world_space = TRUE;
        g_boundingbox_set (octree->boundingbox, &octree->aabb);
    }
    return octree->boundingbox;
}
