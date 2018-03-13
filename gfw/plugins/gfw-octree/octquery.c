#include "gfw-octree.h"
X_DEFINE_DYNAMIC_TYPE (octRayQuery, oct_ray_query, G_TYPE_RAY_QUERY);
xType
_oct_ray_query_register (xTypeModule    *module)
{
    return oct_ray_query_register (module);
}
static xbool
rquery_for_movable      (gMovable       *movable,
                         xptr           objects[])
{
    greal dist;
    gRayQuery *rquery = objects[0];
    gSceneNode *snode = objects[1];
    xCallback callback = objects[2];
    xptr user_data = objects[3];

    if (!g_movable_match(movable,
                         rquery->parent.query_mask,
                         rquery->parent.type_mask))
        return TRUE;

    if (g_ray_collide_aabb (&rquery->ray,
                            g_movable_wbbox(movable, snode),
                            &dist))
        callback(movable, user_data, &dist);
    return TRUE;
}
static void
ray_travel_collisions   (gRayQuery      *rquery,
                         struct octree  *octree,
                         xbool          inside,
                         gSceneNode     *exclude,
                         xCallback      callback,
                         xptr           user_data)
{
    xsize i;
    gSceneNode *snode;
    gBoxSide on_side;

    if (!inside) {
        gaabb aabb;
        gvec halfsize;

        aabb = octree->aabb;
        halfsize = g_aabb_half (&aabb);
        aabb.minimum = g_vec_sub (aabb.minimum, halfsize);
        aabb.maximum = g_vec_add (aabb.maximum, halfsize);

        on_side = g_ray_collide_aabb (&rquery->ray, &aabb, NULL);
        if (on_side < G_BOX_PARTIAL )
            return;
        inside = (on_side == G_BOX_INSIDE);
    }
    for (i=0;i<octree->nodes->len;++i) {
        snode = x_ptr_array_index(octree->nodes, gSceneNode, i);
        if (snode == exclude)
            continue;
        if (!inside)
            on_side = g_ray_collide_aabb (&rquery->ray, g_scene_node_wbbox (snode), NULL);
        else
            on_side = G_BOX_INSIDE;
        if (on_side >= G_BOX_PARTIAL) {
            xptr objects[4]={rquery, snode,callback, user_data};
            g_scene_node_foreach (snode, rquery_for_movable, objects);
        }
    }
    /* ตÝน้ */
    for (i=0;i<8;++i) {
        if (octree->children[i]) {
            ray_travel_collisions (rquery, octree->children[i], inside,
                                   exclude, callback, user_data);
        }
    }
}
static void
ray_query_execute       (gQuery         *query,
                         xCallback      callback,
                         xptr           user_data)
{
    gRayQuery *rquery = (gRayQuery*)query;
    octScene *oscene= (octScene*)query->scene;

    ray_travel_collisions (rquery, oscene->octree,
                           FALSE, NULL,
                           callback, user_data);
}
static void
oct_ray_query_init      (octRayQuery    *rquery)
{

}
static void
oct_ray_query_class_init (octRayQueryClass *klass)
{
    gQueryClass *qclass;

    qclass = G_QUERY_CLASS (klass);
    qclass->execute = ray_query_execute;
}
static void
oct_ray_query_class_finalize (octRayQueryClass *klass)
{

}
