#include "config.h"
#include "gquery.h"
#include "gnode.h"
#include "gscene.h"
#include "gmovable.h"
#include "gscenenode.h"

X_DEFINE_TYPE (gQuery, g_query, X_TYPE_OBJECT);
X_DEFINE_TYPE (gRegionQuery, g_region_query, G_TYPE_QUERY);
X_DEFINE_TYPE (gAabbQuery, g_aabb_query, G_TYPE_REGION_QUERY);
X_DEFINE_TYPE (gSphereQuery, g_sphere_query, G_TYPE_REGION_QUERY);
X_DEFINE_TYPE (gRayQuery, g_ray_query, G_TYPE_QUERY);
X_DEFINE_TYPE (gCollideQuery, g_collide_query, G_TYPE_QUERY);

void
g_query_collect         (gQuery         *query)
{
    gQueryClass *klass;
    x_return_if_fail (G_IS_QUERY (query));

    klass = G_QUERY_GET_CLASS (query);
    klass->collect (query);
}

void
g_query_execute         (gQuery         *query,
                         xCallback      callback,
                         xptr           user_data)
{
    gQueryClass *klass;
    x_return_if_fail (G_IS_QUERY (query));

    klass = G_QUERY_GET_CLASS (query);
    klass->execute (query, callback, user_data);
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
static xbool
rquery_for_node         (gSceneNode     *snode,
                         xptr           objects[])
{
    gRayQuery *rquery = objects[0];

    if (!g_ray_collide_aabb (&rquery->ray, g_scene_node_wbbox (snode), NULL))
        return TRUE;

    objects[1] = snode;
    g_scene_node_foreach (snode, rquery_for_movable, objects);
    g_node_foreach (G_NODE(snode), rquery_for_node, objects);
    return TRUE;
}

static void
collect_collisions      (gMovable       *movable,
                         gRayQuery      *rquery,
                         greal          *dist)
{
    xsize i;
    for (i=0;i<rquery->distance->len;++i) {
        greal d = x_array_index (rquery->distance, greal, i);
        if (*dist < d) break;
    }
    x_array_insert1 (rquery->distance, i, *dist);
    x_ptr_array_insert1 (rquery->result, i, movable);
}
static void
ray_query_collect       (gQuery         *query)
{
    gQueryClass *klass;
    gRayQuery *rquery = (gRayQuery*)query;

    rquery->distance->len = 0;
    rquery->result->len = 0;
    klass = G_QUERY_GET_CLASS (query);
    klass->execute (query, collect_collisions, query);
}
static void
ray_query_execute       (gQuery         *query,
                         xCallback      callback,
                         xptr           user_data)
{
    xptr objects[4]={
        query, g_scene_root_node (query->scene),
        callback, user_data
    };
    rquery_for_node (objects[1], objects);
}
static void
g_query_init            (gQuery         *query)
{
    query->query_mask = -1;
    query->type_mask = -1;
}
static void
g_query_class_init      (gQueryClass    *klass)
{
}
static void
g_ray_query_init        (gRayQuery      *query)
{
    query->distance = x_array_new (sizeof(greal));
    query->result = x_ptr_array_new ();
}
static void
g_ray_query_class_init  (gRayQueryClass *klass)
{
    gQueryClass *qclass;

    qclass = G_QUERY_CLASS (klass);
    qclass->execute = ray_query_execute;
    qclass->collect = ray_query_collect;
}
static void
g_region_query_init     (gRegionQuery     *query)
{
}
static void
g_region_query_class_init (gRegionQueryClass *klass)
{
}
static void
g_aabb_query_init       (gAabbQuery     *query)
{
}
static void
g_aabb_query_class_init (gAabbQueryClass *klass)
{
}
static void
g_sphere_query_init     (gSphereQuery     *query)
{
}
static void
g_sphere_query_class_init (gSphereQueryClass *klass)
{
}
static void
g_collide_query_init     (gCollideQuery     *query)
{
}
static void
g_collide_query_class_init (gCollideQueryClass *klass)
{
}
