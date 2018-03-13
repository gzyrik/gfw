#ifndef __G_SIMPLE_H__
#define __G_SIMPLE_H__
#include "gmovable.h"
#include "grender.h"
X_BEGIN_DECLS

#define G_TYPE_SIMPLE              (g_simple_type())
#define G_SIMPLE(object)           (X_INSTANCE_CAST((object), G_TYPE_SIMPLE, gSimple))
#define G_SIMPLE_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_SIMPLE, gSimpleClass))
#define G_IS_SIMPLE(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_SIMPLE))
#define G_SIMPLE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_SIMPLE, gSimpleClass))

#define G_TYPE_RECTANGLE              (g_rectangle_type())
#define G_RECTANGLE(object)           (X_INSTANCE_CAST((object), G_TYPE_RECTANGLE, gRectangle))
#define G_RECTANGLE_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_RECTANGLE, gRectangleClass))
#define G_IS_RECTANGLE(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_RECTANGLE))
#define G_RECTANGLE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_RECTANGLE, gRectangleClass))

#define G_TYPE_BOUNDINGBOX              (g_boundingbox_type())
#define G_BOUNDINGBOX(object)           (X_INSTANCE_CAST((object), G_TYPE_BOUNDINGBOX, gBoundingBox))
#define G_BOUNDINGBOX_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_BOUNDINGBOX, gBoundingBoxClass))
#define G_IS_BOUNDINGBOX(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_BOUNDINGBOX))
#define G_BOUNDINGBOX_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_BOUNDINGBOX, gBoundingBoxClass))

struct _gSimple
{
    gMovable            parent;
    gPrimitiveType      primitive;
    gVertexData         *vdata;
    gIndexData          *idata;
    gMaterial           *material;
};

struct _gSimpleClass
{
    gMovableClass       parent;
};


struct _gRectangle
{
    gSimple             parent;
};

struct _gRectangleClass
{
    gSimpleClass        parent;

};

struct _gBoundingBox
{
    gSimple             parent;
    xbool               world_space;
};

struct _gBoundingBoxClass
{
    gSimpleClass        parent;
};

xType       g_simple_type           ();

xType       g_rectangle_type        ();

xType       g_boundingbox_type      ();

void        g_rectangle_set         (gRectangle     *rect,
                                     const grect    *coord);

void        g_boundingbox_set       (gBoundingBox   *bbox,
                                     const gaabb    *aabb);
X_END_DECLS
#endif /* __G_SIMPLE_H__ */
