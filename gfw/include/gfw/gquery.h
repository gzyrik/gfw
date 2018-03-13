#ifndef __G_QUERY_H__
#define __G_QUERY_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_QUERY                    (g_query_type())
#define G_QUERY(object)                 (X_INSTANCE_CAST((object), G_TYPE_QUERY, gQuery))
#define G_QUERY_CLASS(klass)            (X_CLASS_CAST((klass), G_TYPE_QUERY, gQueryClass))
#define G_IS_QUERY(object)              (X_INSTANCE_IS_TYPE((object), G_TYPE_QUERY))
#define G_QUERY_GET_CLASS(object)       (X_INSTANCE_GET_CLASS((object), G_TYPE_QUERY, gQueryClass))

#define G_TYPE_REGION_QUERY             (g_region_query_type())
#define G_REGION_QUERY(object)          (X_INSTANCE_CAST((object), G_TYPE_REGION_QUERY, gRegionQuery))
#define G_REGION_QUERY_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_REGION_QUERY, gRegionQueryClass))
#define G_IS_REGION_QUERY(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_REGION_QUERY))
#define G_REGION_QUERY_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_REGION_QUERY, gRegionQueryClass))

#define G_TYPE_RAY_QUERY                (g_ray_query_type())
#define G_RAY_QUERY(object)             (X_INSTANCE_CAST((object), G_TYPE_RAY_QUERY, gRayQuery))
#define G_RAY_QUERY_CLASS(klass)        (X_CLASS_CAST((klass), G_TYPE_RAY_QUERY, gRayQueryClass))
#define G_IS_RAY_QUERY(object)          (X_INSTANCE_IS_TYPE((object), G_TYPE_RAY_QUERY))
#define G_RAY_QUERY_GET_CLASS(object)   (X_INSTANCE_GET_CLASS((object), G_TYPE_RAY_QUERY, gRayQueryClass))

#define G_TYPE_SPHERE_QUERY             (g_sphere_query_type())
#define G_SPHERE_QUERY(object)          (X_INSTANCE_CAST((object), G_TYPE_SPHERE_QUERY, gSphereQuery))
#define G_SPHERE_QUERY_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_SPHERE_QUERY, gSphereQueryClass))
#define G_IS_SPHERE_QUERY(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_SPHERE_QUERY))
#define G_SPHERE_QUERY_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_SPHERE_QUERY, gSphereQueryClass))

#define G_TYPE_COLLIDE_QUERY            (g_collide_query_type())
#define G_COLLIDE_QUERY(object)         (X_INSTANCE_CAST((object), G_TYPE_COLLIDE_QUERY, gCollideQuery))
#define G_COLLIDE_QUERY_CLASS(klass)    (X_CLASS_CAST((klass), G_TYPE_COLLIDE_QUERY, gCollideQueryClass))
#define G_IS_COLLIDE_QUERY(object)      (X_INSTANCE_IS_TYPE((object), G_TYPE_COLLIDE_QUERY))
#define G_COLLIDE_QUERY_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_COLLIDE_QUERY, gCollideQueryClass))




void        g_query_collect         (gQuery         *query);

void        g_query_execute         (gQuery         *query,
                                     xCallback      callback,
                                     xptr           user_data);

struct _gQuery
{
    xObject             parent;
    gScene              *scene;
    xuint               query_mask;
    xuint               type_mask;
};
struct _gQueryClass
{
    xObjectClass        parent;
    void (*execute) (gQuery *query, xCallback callback, xptr user_data);
    void (*collect) (gQuery *query);
};
struct _gRegionQuery
{
    gQuery              parent;
    xPtrArray           *result; /* gMovable* set */
};
struct _gRegionQueryClass
{
    gQueryClass         parent;
};
struct _gSphereQuery
{
    gRegionQuery        parent;
    gsphere             sphere;
};
struct _gSphereQueryClass
{
    gRegionQueryClass   parent;
};
struct _gAabbQuery
{
    gRegionQuery        parent;
    gaabb               aabb;
};
struct _gAabbQueryClass
{
    gRegionQueryClass   parent;
};
struct _gRayQuery
{
    gQuery              parent;
    gray                ray;
    xArray              *distance; /* sorted greal */
    xPtrArray           *result; /* gMovable* set sorted by distance */
};
struct _gRayQueryClass
{
    gQueryClass         parent;
};
struct _gCollideQuery
{
    gQuery              parent;
    xHashTable          *result; /* gMovable* -> gMovable* set */
};
struct _gCollideQueryClass
{
    gQueryClass         parent;
};

xType       g_query_type            (void);

xType       g_region_query_type     (void);

xType       g_sphere_query_type     (void);

xType       g_aabb_query_type       (void);

xType       g_ray_query_type        (void);

xType       g_collide_query_type     (void);

X_END_DECLS
#endif /* __G_QUERY_H__ */
