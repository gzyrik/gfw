#ifndef __GFW_OCTREE_H__
#define __GFW_OCTREE_H__
#define X_LOG_DOMAIN                "gfw-octree"
#include <gfw.h>
X_BEGIN_DECLS

typedef struct _octNode             octNode;
typedef struct _octNodeClass        octNodeClass;
typedef struct _octScene            octScene;
typedef struct _octSceneClass       octSceneClass;
typedef struct _octRayQuery         octRayQuery;
typedef struct _octRayQueryClass    octRayQueryClass;

struct octree
{
    struct octree       *parent;
    struct octree       *children[8];
    gBoundingBox        *boundingbox;
    gaabb               aabb;
    xPtrArray           *nodes;
    xsize               n_nodes;
    xsize               depth;
};
struct _octNode
{
    gSceneNode          parent;
    struct octree       *octree;
};
struct _octNodeClass 
{
    gSceneNodeClass     parent;
};
struct _octScene
{
    gScene              parent;
    gaabb               world_box;
    xbool               octree_visible;
    struct octree       *octree;
};
struct _octSceneClass 
{
    gSceneClass         parent;
};
struct _octRayQuery
{
    gRayQuery           parent;
};
struct _octRayQueryClass
{
    gRayQueryClass      parent;
};

X_INTERN_FUNC
void       octree_delete            (struct octree  *octree);
X_INTERN_FUNC
struct octree* octree_new           (void);
X_INTERN_FUNC
xbool       octree_twice_size       (struct octree  *octree,
                                     const gaabb    *box);
X_INTERN_FUNC
void        octree_attach           (struct octree  *octree,
                                     octNode     *node);
X_INTERN_FUNC
void        octree_detach           (struct octree  *octree,
                                     octNode     *node);
X_INTERN_FUNC
void        octree_set_depth        (struct octree  *octree,
                                     xsize          depth);
X_INTERN_FUNC
void        octree_set_size         (struct octree  *octree,
                                     const gvec     size);
X_INTERN_FUNC
gBoundingBox* octree_boundingbox    (struct octree  *octree);

X_INTERN_FUNC
void        oct_scene_refresh       (octScene    *scene,
                                     octNode     *node);
X_INTERN_FUNC
xType       _oct_node_register      (xTypeModule    *module);
X_INTERN_FUNC
xType       _oct_scene_register     (xTypeModule    *module);
X_INTERN_FUNC
xType       _oct_ray_query_register (xTypeModule    *module);

#endif /* __GFW_OCTREE_H__ */

