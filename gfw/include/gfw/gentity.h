#ifndef __G_ENTITY_H__
#define __G_ENTITY_H__
#include "gmovable.h"
X_BEGIN_DECLS

#define G_TYPE_ENTITY             (g_entity_type())
#define G_ENTITY(object)          (X_INSTANCE_CAST((object), G_TYPE_ENTITY, gEntity))
#define G_ENTITY_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_ENTITY, gEntityClass))
#define G_IS_ENTITY(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_ENTITY))
#define G_ENTITY_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_ENTITY, gEntityClass))


struct _gSubEntity
{
    xbool               visiable;
    gMaterial           *material;
    gVertexData         *hardware_vdata;
    gVertexData         *software_vdata;
    gVertexData         *skeleton_vdata;
};

struct _gEntity
{
    gMovable            parent;
    gMesh               *mesh;
    xArray              *subentity;
    gMaterial           *material;
    gSkeleton           *skeleton;
    xint                animation;
    xbool               hardware_animated;
    gVertexData         *hardware_vdata;
    gVertexData         *software_vdata;
    gVertexData         *skeleton_vdata;
    gStateSet           *states;
    xuint               states_stamp;
};

struct _gEntityClass
{
    gMovableClass           parent;
};

xType       g_entity_type           (void);

#define     g_entity_states(entity) G_ENTITY(entity)->states

X_END_DECLS
#endif /* __G_ENTITY_H__ */
