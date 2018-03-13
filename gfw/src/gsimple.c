#include "config.h"
#include "gsimple.h"
#include "gmaterial.h"
#include "grender.h"
#include "grenderable.h"
#include "grenderqueue.h"
#include "gbuffer.h"
#include "gnode.h"
#include "gpsystem.h"
#include <string.h>

static gTechnique*
simple_technique        (gRenderable    *renderable,
                         xptr           user_data)
{
    gSimple *simple = (gSimple*)renderable;
    return g_material_technique(simple->material);
}

static void
simple_render_op        (gRenderable    *renderable,
                         gRenderOp      *op,
                         xptr           user_data)
{
    gSimple *simple = (gSimple*)renderable;
    op->primitive = simple->primitive;
    op->vdata = simple->vdata;
    op->idata = simple->idata;
    op->renderable = renderable;
}
static void
renderable_init         (gRenderableIFace *iface)
{
    iface->technique  = simple_technique;
    iface->render_op   = simple_render_op;
}
X_DEFINE_TYPE_EXTENDED (gSimple, g_simple, G_TYPE_MOVABLE, X_TYPE_ABSTRACT,
                        X_IMPLEMENT_IFACE (G_TYPE_RENDERABLE, renderable_init));

enum {
    PROP_0,
    PROP_MATERIAL,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gSimple *simple = (gSimple*)object;

    switch(property_id) {
    case PROP_MATERIAL:
        g_material_detach(simple->material, simple);
        simple->material = g_material_attach(x_value_get_str (value), simple);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gSimple *simple = (gSimple*)object;

    switch(property_id) {
    case PROP_MATERIAL:
        if (simple->material)
            x_object_get_value(simple->material, "name", value);
        break;
    }
}

static void
simple_enqueue          (gMovable       *movable,
                         gMovableContext*context)
{
    gSimple *simple = (gSimple*)movable;

    g_render_queue_add(context->queue, G_RENDERABLE(simple), context->node);
}
static void
g_simple_init           (gSimple        *simple)
{
    simple->material = g_material_attach ("default", simple);
}
static void
simple_finalize         (xObject        *object)
{
    gSimple *simple = (gSimple*)object;
    g_material_detach(simple->material, simple);
    g_vertex_data_delete (simple->vdata);
    g_index_data_delete (simple->idata);
}
static void
g_simple_class_init     (gSimpleClass   *klass)
{
    gMovableClass *mclass;
    xParam         *param;
    xObjectClass   *oclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = simple_finalize;

    mclass = G_MOVABLE_CLASS (klass);
    mclass->enqueue = simple_enqueue;


    param = x_param_str_new ("material",
                             "Material",
                             "The material name of the simple object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_MATERIAL, param);
}

/* gRectangle */
X_DEFINE_TYPE (gRectangle, g_rectangle, G_TYPE_SIMPLE);
static void
g_rectangle_init        (gRectangle     *rect)
{
    gVertexData *vdata;
    gVertexElem velem;
    gaabb aabb;
    greal v[]={-1,1,0,-1,-1,0,1,1,0,1,-1,0};
    greal t[]={0,0,0,1,1,0,1,1};
    gSimple *simple = (gSimple*)rect;

    simple->primitive = G_RENDER_TRIANGLE_STRIP;
    aabb.extent = -1;
    g_movable_resize (G_MOVABLE(simple), &aabb);

    vdata = x_new0 (gVertexData, 1);
    vdata->count = 4;

    memset(&velem, 0, sizeof(velem));
    velem.semantic = G_VERTEX_POSITION;
    velem.type = G_VERTEX_FLOAT3;
    g_vertex_data_add (vdata, &velem);

    memset(&velem, 0, sizeof(velem));
    velem.source = 1;
    velem.semantic = G_VERTEX_TEXCOORD;
    velem.type = G_VERTEX_FLOAT2;
    g_vertex_data_add (vdata, &velem);

    g_vertex_data_alloc (vdata);

    g_buffer_write (g_vertex_data_get (vdata, 0),
                    0, -1, v);

    g_buffer_write (g_vertex_data_get (vdata, 1),
                    0, -1, t);

    simple->vdata = vdata;
}
static xsize
rectangle_wmatrix       (gRenderable    *renderable,
                         gmat4          *mat,
                         gNode          *node,
                         xptr           user_data)
{
    *mat = g_mat4_eye ();
    return 1;
}
static xbool
rectangle_pmatrix       (gRenderable    *renderable,
                         gmat4          *mat,
                         xptr           user_data)
{
    *mat = g_mat4_eye ();
    return TRUE;
}
static xbool
rectangle_vmatrix       (gRenderable    *renderable,
                         gmat4          *mat,
                         xptr           user_data)
{
    *mat = g_mat4_eye ();
    return TRUE;
}
static void
g_rectangle_class_init  (gRectangleClass*klass)
{
    gRenderableIFace *iface;

    iface = x_iface_peek (klass, G_TYPE_RENDERABLE);
    iface->proj_matrix = rectangle_pmatrix;
    iface->view_matrix = rectangle_vmatrix;
    iface->world_matrix = rectangle_wmatrix;
}

void
g_rectangle_set         (gRectangle     *rect,
                         const grect    *coord)
{
    xfloat *p;
    gBuffer *vbuf;
    gSimple *simple = (gSimple*)rect;

    vbuf = g_vertex_data_get (simple->vdata, 0);
    p = g_buffer_lock (vbuf);

    *p++ = coord->left;
    *p++ = coord->top;
    *p++ = 0;

    *p++ = coord->left;
    *p++ = coord->bottom;
    *p++ = 0;

    *p++ = coord->right;
    *p++ = coord->top;
    *p++ = 0;

    *p++ = coord->right;
    *p++ = coord->bottom;
    *p++ = 0;

    g_buffer_unlock (vbuf);
}
/* gBoundingBox */
X_DEFINE_TYPE (gBoundingBox, g_boundingbox, G_TYPE_SIMPLE);
static void
boundingbox_set         (gVertexData    *vdata,
                         const gaabb    *aabb)
{
    xfloat min[4], max[4];
    xfloat *p;
    gBuffer *vbuf;

    g_vec_store (aabb->minimum, min);
    g_vec_store (aabb->maximum, max);
    vbuf = g_vertex_data_get (vdata, 0);
    p = g_buffer_lock (vbuf);

    /* line 0 */
    *p++ = min[0];
    *p++ = min[1];
    *p++ = min[2];
    *p++ = max[0];
    *p++ = min[1];
    *p++ = min[2];
    /* line 1 */
    *p++ = min[0];
    *p++ = min[1];
    *p++ = min[2];
    *p++ = min[0];
    *p++ = min[1];
    *p++ = max[2];
    /* line 2 */
    *p++ = min[0];
    *p++ = min[1];
    *p++ = min[2];
    *p++ = min[0];
    *p++ = max[1];
    *p++ = min[2];
    /* line 3 */
    *p++ = min[0];
    *p++ = max[1];
    *p++ = min[2];
    *p++ = min[0];
    *p++ = max[1];
    *p++ = max[2];
    /* line 4 */
    *p++ = min[0];
    *p++ = max[1];
    *p++ = min[2];
    *p++ = max[0];
    *p++ = max[1];
    *p++ = min[2];
    /* line 5 */
    *p++ = max[0];
    *p++ = min[1];
    *p++ = min[2];
    *p++ = max[0];
    *p++ = min[1];
    *p++ = max[2];
    /* line 6 */
    *p++ = max[0];
    *p++ = min[1];
    *p++ = min[2];
    *p++ = max[0];
    *p++ = max[1];
    *p++ = min[2];
    /* line 7 */
    *p++ = min[0];
    *p++ = max[1];
    *p++ = max[2];
    *p++ = max[0];
    *p++ = max[1];
    *p++ = max[2];
    /* line 8 */
    *p++ = min[0];
    *p++ = max[1];
    *p++ = max[2];
    *p++ = min[0];
    *p++ = min[1];
    *p++ = max[2];
    /* line 9 */
    *p++ = max[0];
    *p++ = max[1];
    *p++ = min[2];
    *p++ = max[0];
    *p++ = max[1];
    *p++ = max[2];
    /* line 10 */
    *p++ = max[0];
    *p++ = min[1];
    *p++ = max[2];
    *p++ = max[0];
    *p++ = max[1];
    *p++ = max[2];
    /* line 11 */
    *p++ = min[0];
    *p++ = min[1];
    *p++ = max[2];
    *p++ = max[0];
    *p++ = min[1];
    *p++ = max[2];

    g_buffer_unlock (vbuf);
}
static void
g_boundingbox_init      (gBoundingBox     *bbox)
{
    gVertexElem velem;
    gSimple *simple = (gSimple*)bbox;

    simple->primitive = G_RENDER_LINE_LIST;
    simple->vdata = x_new0 (gVertexData, 1);
    simple->vdata->count = 24;

    memset(&velem, 0, sizeof(velem));
    velem.semantic = G_VERTEX_POSITION;
    velem.type = G_VERTEX_FLOAT3;
    g_vertex_data_add (simple->vdata, &velem);

    g_vertex_data_alloc (simple->vdata);

    boundingbox_set (simple->vdata, g_movable_lbbox (G_MOVABLE(simple)));
}
static xsize
boundingbox_wmatrix     (gRenderable    *renderable,
                         gmat4          *mat,
                         gNode          *node,
                         xptr           user_data)
{
    gBoundingBox *bbox = (gBoundingBox*)renderable;
    if (bbox->world_space || !node)
        *mat = g_mat4_eye ();
    else
        *mat = *g_node_mat4 (node);

    return 1;
}
static void
g_boundingbox_class_init(gBoundingBoxClass*klass)
{
    gRenderableIFace *iface;

    iface = x_iface_peek (klass, G_TYPE_RENDERABLE);
    iface->world_matrix = boundingbox_wmatrix;
}

void
g_boundingbox_set       (gBoundingBox   *bbox,
                         const gaabb    *aabb)
{
    gSimple *simple = (gSimple*)bbox;

    g_movable_resize ((gMovable*)simple, aabb);
    boundingbox_set (simple->vdata, aabb);
}


