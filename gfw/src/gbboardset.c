#include "config.h"
#include "gbboardset.h"
#include "gbuffer.h"
#include "grenderqueue.h"
#include "grender.h"
#include "grenderable.h"
#include "gcamera.h"
#include "gnode.h"
#include "gprender.h"
#include "gpsystem.h"
#include <string.h>

typedef enum {
    G_BBR_VERTEX,
    G_BBR_TEXCOORD
} gBBoardRotateType;

typedef enum {
    G_BBO_TOP_LEFT,
    G_BBO_TOP_CENTER,
    G_BBO_TOP_RIGHT,
    G_BBO_CENTER_LEFT,
    G_BBO_CENTER,
    G_BBO_CENTER_RIGHT,
    G_BBO_BOTTOM_LEFT,
    G_BBO_BOTTOM_CENTER,
    G_BBO_BOTTOM_RIGHT
} gBBoardOrigin;

typedef enum {
    /* 始终面向摄像机,且垂直于摄像机 */
    G_BBT_POINT,
    /* 共享的朝向(默认Y轴),只能面向镜头旋转 */
    G_BBT_ORIENTED_COMMON,
    /* 独立的朝向,但只能面向镜头旋转 */
    G_BBT_ORIENTED_SELF,
    /* 共享的垂直方向(默认Z轴),可不再面向镜头 */
    G_BBT_PERPENDICULAR_COMMON,
    /* 独立的垂直方向(默认Z轴),可不再面向镜头 */
    G_BBT_PERPENDICULAR_SELF,
} gBBoardType;

struct _gBBoardSet
{
    gSimple             parent;
    gBBoardType         type;
    gBBoardOrigin       origin;
    gBBoardRotateType   rotate_type;
    xArray              *bboards;
    xsize               pool_size;
    xArray              *texcoord;
    gvec                common_z;
    gvec                common_y;
    greal               common_width;
    greal               common_height;
    xuint               camera_stamp;
    xbool               pointing; /* point rendering */
    xbool               common_size;
    xbool               accurate_face;
    xbool               sorting;
    xbool               culling;
    xbool               world_space;
};
struct _gBBoardSetClass
{
    gSimpleClass    parent;
};
enum {
    PROP_0,
    PROP_POOL_SIZE,
    PROP_COMMON_WIDTH,
    PROP_COMMON_HEIGHT,
    PROP_COMMON_SIZE,
    PROP_ACCURATE_FACE,
    PROP_COMMON_Z,
    PROP_COMMON_Y,
    N_PROPERTIES
};
struct bb_compiler {
    gBuffer         *vbuf;
    xbyte           *pbuf;
    gvec            cam_pos;
    gvec            cam_x;
    gvec            cam_y;
    gquat           cam_quat;
    gvec            cam_dir;
    grect           axes_origin;
    xsize           n_visibles;
    gvec            bb_voffset[4];
    gaabb           aabb;
    gCamera         *camera;
    gBBoardSet      *bboardset;
};
static void
prender_init            (gPRenderIFace     *iface);
X_DEFINE_TYPE_EXTENDED (gBBoardSet, g_bboardset, G_TYPE_SIMPLE, 0,
                        X_IMPLEMENT_IFACE (G_TYPE_PRENDER, prender_init));

static void
bboard_free             (gBBoard        *bboard)
{
    x_slice_free (gBBoard, bboard);
}

static void
bbset_new_buf           (gBBoardSet     *bboardset)
{
    gVertexData *vdata;
    gSimple *simple;
    gVertexElem velem;

    simple = (gSimple*)bboardset;
    if (bboardset->pointing &&  bboardset->type != G_BBT_POINT) {
        x_error ("has point rendering enabled but is using a type "
                 "other than G_BBT_POINT, this may not give you the"
                 "results you expect.");
    }
    vdata = g_vertex_data_new (bboardset->pool_size);
    vdata->start = 0;
    if (!bboardset->pointing)
        vdata->count *= 4;

    memset(&velem, 0, sizeof(velem));
    velem.semantic = G_VERTEX_POSITION;
    velem.type = G_VERTEX_FLOAT3;
    g_vertex_data_add (vdata, &velem);

    memset(&velem, 0, sizeof(velem));
    velem.semantic = G_VERTEX_DIFFUSE;
    velem.type = G_VERTEX_A8R8G8B8;
    g_vertex_data_add (vdata, &velem);

    if (!bboardset->pointing) {
        xuint16 *pindex;
        gIndexData *idata;
        gBuffer *ibuf;
        xsize i;
        xuint16 offset, vindex;

        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_TEXCOORD;
        velem.type = G_VERTEX_FLOAT2;
        g_vertex_data_add (vdata, &velem);

        idata = g_index_data_new(bboardset->pool_size*6);
        ibuf = g_index_buffer_new (sizeof(xuint16), idata->count, NULL);
        g_index_data_bind (idata, G_INDEX_BUFFER(ibuf));
        x_object_unref (ibuf);

        pindex = g_buffer_lock (ibuf);
        for (i=0;i<bboardset->pool_size; ++i) {
            offset = i*6;
            vindex = i*4;
            pindex[offset+0] = vindex+0;
            pindex[offset+1] = vindex+2;
            pindex[offset+2] = vindex+1;
            pindex[offset+3] = vindex+1;
            pindex[offset+4] = vindex+2;
            pindex[offset+5] = vindex+3;
        }
        g_buffer_unlock (ibuf);
        if (simple->idata)
            g_index_data_delete (simple->idata);
        simple->idata = idata;
    }
    g_vertex_data_alloc(vdata);
    g_vertex_data_delete (simple->vdata);
    simple->vdata = vdata;
}

static void
gen_bboard_origin       (gBBoardSet     *bboardset,
                         struct bb_compiler *bbc)
{
    greal left, right, top, bottom;
    switch(bboardset->origin) {
    case G_BBO_TOP_LEFT:
        left = 0.0f;
        right = 1.0f;
        top = 0.0f;
        bottom = -1.0f;
        break;
    case G_BBO_TOP_CENTER:
        left = -0.5f;
        right = 0.5f;
        top = 0.0f;
        bottom = -1.0f;
        break;
    case G_BBO_TOP_RIGHT:
        left = -1.0f;
        right = 0.0f;
        top = 0.0f;
        bottom = -1.0f;
        break;
    case G_BBO_CENTER_LEFT:
        left = 0.0f;
        right = 1.0f;
        top = 0.5f;
        bottom = -0.5f;
        break;
    default:
    case G_BBO_CENTER:
        left = -0.5f;
        right = 0.5f;
        top = 0.5f;
        bottom = -0.5f;
        break;
    case G_BBO_CENTER_RIGHT:
        left = -1.0f;
        right = 0.0f;
        top = 0.5f;
        bottom = -0.5f;
        break;
    case G_BBO_BOTTOM_LEFT:
        left = 0.0f;
        right = 1.0f;
        top = 1.0f;
        bottom = 0.0f;
        break;
    case G_BBO_BOTTOM_CENTER:
        left = -0.5f;
        right = 0.5f;
        top = 1.0f;
        bottom = 0.0f;
        break;
    case G_BBO_BOTTOM_RIGHT:
        left = -1.0f;
        right = 0.0f;
        top = 1.0f;
        bottom = 0.0f;
        break;
    }
    bbc->axes_origin.left = left;
    bbc->axes_origin.right = right;
    bbc->axes_origin.top = top;
    bbc->axes_origin.bottom = bottom;
}
static void
gen_bboard_axes         (gBBoardSet     *bboardset,
                         struct bb_compiler *bbc,
                         gBBoard        *bb)
{
    if (bboardset->accurate_face
        && (bboardset->type == G_BBT_POINT
            || bboardset->type == G_BBT_ORIENTED_COMMON
            || bboardset->type == G_BBT_ORIENTED_SELF)) {
        bbc->cam_dir = g_vec3_norm (g_vec_sub (bb->position, bbc->cam_pos));
    }
    switch (bboardset->type) {
    case G_BBT_POINT:
        if (bboardset->accurate_face) {
            bbc->cam_y = g_vec3_rotate (G_VEC_Y, bbc->cam_quat);
            bbc->cam_x = g_vec3_norm (g_vec3_cross (bbc->cam_dir, bbc->cam_y));
            bbc->cam_y = g_vec3_cross (bbc->cam_x, bbc->cam_dir);
        }
        else {
            bbc->cam_y = g_vec3_rotate (G_VEC_Y, bbc->cam_quat);
            bbc->cam_x = g_vec3_rotate (G_VEC_X, bbc->cam_quat);
        }
        break;
    case G_BBT_ORIENTED_COMMON:
        bbc->cam_y = bboardset->common_y;
        bbc->cam_x = g_vec3_norm (g_vec3_cross (bbc->cam_dir, bbc->cam_y));
        break;
    case G_BBT_ORIENTED_SELF:
        bbc->cam_y = bb->direction;
        bbc->cam_x = g_vec3_norm (g_vec3_cross (bbc->cam_dir, bbc->cam_y));
        break;
    case G_BBT_PERPENDICULAR_COMMON:
        bbc->cam_x = g_vec3_cross (bboardset->common_z, bboardset->common_y);
        bbc->cam_y = g_vec3_cross (bboardset->common_y, bbc->cam_x);
        break;
    case G_BBT_PERPENDICULAR_SELF:
        bbc->cam_x = g_vec3_norm (g_vec3_cross (bboardset->common_z,
                                                bb->direction));
        bbc->cam_y = g_vec3_cross (bb->direction, bbc->cam_x);
        break;
    }
}
static void
gen_bboard_voffset      (gBBoardSet     *bboardset,
                         struct bb_compiler *bbc,
                         greal          width,
                         greal          height,
                         gvec           *voffset)
{
    gvec left, right, top, bottom;

    left = g_vec_scale (bbc->cam_x, bbc->axes_origin.left * width);
    right = g_vec_scale (bbc->cam_x, bbc->axes_origin.right * width);
    top = g_vec_scale (bbc->cam_y, bbc->axes_origin.top * height);
    bottom = g_vec_scale (bbc->cam_y, bbc->axes_origin.bottom* height);

    voffset[0] = g_vec_add (left, top);
    voffset[1] = g_vec_add (right, top);
    voffset[2] = g_vec_add (left, bottom);
    voffset[3] = g_vec_add (right, bottom);
}

static void
gen_bboard_vertices     (gBBoardSet     *bboardset,
                         struct bb_compiler *bbc,
                         gvec           *voffset,
                         gBBoard        *bboard)
{
    grect  *tex_rect;
    xfloat texcoord[4][2];
    xuint32 color;
    color = g_vec_color (bboard->color);

    if (bboard->tex_index < 0) 
        tex_rect = &bboard->tex_rect;
    else 
        tex_rect = &x_array_index (bboardset->texcoord,
                                   grect, bboard->tex_index);

    texcoord[0][0] = tex_rect->left;
    texcoord[0][1] = tex_rect->top;
    texcoord[1][0] = tex_rect->right;
    texcoord[1][1] = tex_rect->top;
    texcoord[2][0] = tex_rect->left;
    texcoord[2][1] = tex_rect->bottom;
    texcoord[3][0] = tex_rect->right;
    texcoord[3][1] = tex_rect->bottom;

    if (bboardset->pointing) {
        memcpy (bbc->pbuf, &bboard->position, sizeof(xfloat)*3);
        bbc->pbuf += sizeof(xfloat)*3;
        memcpy (bbc->pbuf, &color, sizeof(xint32));
        bbc->pbuf += sizeof(xint32);
        g_aabb_extent (&bbc->aabb, bboard->position);
    }
    else {
        gquat quat;
        gvec pos;
        xsize i;
        xbool rotate = !g_real_aeq0 (bboard->rotate);
        if (rotate) {
            if (bboardset->rotate_type == G_BBR_VERTEX) {
                pos = g_vec3_norm (g_vec3_cross (g_vec_sub (voffset[3],
                                                            voffset[0]),
                                                 g_vec_sub (voffset[2], 
                                                            voffset[1])));
                quat = g_quat_around (pos, bboard->rotate);
            }
            else {
                const greal cos_rot = g_real_cos (bboard->rotate);
                const greal sin_rot = g_real_sin (bboard->rotate);
                greal width = (tex_rect->right-tex_rect->left)/2;
                greal height = (tex_rect->bottom-tex_rect->top)/2;
                greal mid_u = tex_rect->left+width;
                greal mid_v = tex_rect->top+height;

                greal cos_rot_w = cos_rot * width;
                greal cos_rot_h = cos_rot * height;
                greal sin_rot_w = sin_rot * width;
                greal sin_rot_h = sin_rot * height;

                texcoord[0][0] = mid_u - cos_rot_w + sin_rot_h;
                texcoord[0][1] = mid_v - sin_rot_w - cos_rot_h;
                texcoord[1][0] = mid_u + cos_rot_w + sin_rot_h;
                texcoord[1][1] = mid_v + sin_rot_w - cos_rot_h;
                texcoord[2][0] = mid_u - cos_rot_w - sin_rot_h;
                texcoord[2][1] = mid_v - sin_rot_w + cos_rot_h;
                texcoord[3][0] = mid_u + cos_rot_w - sin_rot_h;
                texcoord[3][1] = mid_v + sin_rot_w + cos_rot_h;
            }
        }
        for (i=0;i<4;++i) {
            if (rotate)
                pos = g_vec_add (g_vec3_rotate (voffset[i], quat),
                                 bboard->position);
            else
                pos = g_vec_add (voffset[i], bboard->position);
            g_aabb_extent (&bbc->aabb, pos);
            memcpy (bbc->pbuf, &pos, sizeof(xfloat)*3);
            bbc->pbuf += sizeof(xfloat)*3;
            memcpy (bbc->pbuf, &color, sizeof(xint32));
            bbc->pbuf += sizeof(xint32);
            memcpy (bbc->pbuf, texcoord[i], sizeof(xfloat)*2);
            bbc->pbuf += sizeof(xfloat)*2;
        }
    }
}
static void
bbset_set_pool_size     (gBBoardSet     *bbset,
                         xsize          pool_size)
{
    gSimple *simple = (gSimple*)bbset;

    bbset->pool_size = pool_size;
    if (simple->vdata) {
        g_vertex_data_delete (simple->vdata);
        simple->vdata = NULL;
    }
    if (simple->idata) {
        g_index_data_delete (simple->idata);
        simple->idata = NULL;
    }
    bbset->camera_stamp = 0;
}
static void
bbset_lock              (gBBoardSet     *bboardset,
                         struct bb_compiler *bbc,
                         xsize          n_bboard)
{
    gSimple *simple = (gSimple*)bboardset;
    xsize bboard_vsize;

    if (!simple->vdata)
        bbset_new_buf (bboardset);

    bbc->vbuf = g_vertex_data_get(simple->vdata, 0);
    bboard_vsize = G_VERTEX_BUFFER(bbc->vbuf)->vertex_size;
    if (!bboardset->pointing) {
        xbool common;
        gen_bboard_origin (bboardset, bbc);
        common = (bboardset->type != G_BBT_ORIENTED_SELF
                  && bboardset->type != G_BBT_PERPENDICULAR_SELF
                  && !(bboardset->accurate_face 
                       &&  bboardset->type != G_BBT_PERPENDICULAR_COMMON));
        if (common) {
            gen_bboard_axes (bboardset, bbc, NULL);
            gen_bboard_voffset (bboardset,bbc,
                                bboardset->common_width,
                                bboardset->common_height,
                                bbc->bb_voffset);
        }
        bboard_vsize *= 4;
    }
    bbc->n_visibles = 0;
    bbc->pbuf = g_buffer_lock_range (bbc->vbuf, 0, n_bboard * bboard_vsize);
}
static void
bbset_unlock            (gBBoardSet     *bboardset,
                         struct bb_compiler *bbc)
{
    gSimple *simple = (gSimple*)bboardset;
    if (bboardset->pointing) {
        simple->primitive = G_RENDER_POINT_LIST;
        simple->vdata->count = bbc->n_visibles;
        if (simple->idata) {
            g_index_data_delete (simple->idata);
            simple->idata = NULL;
        }
    }
    else {
        simple->primitive = G_RENDER_TRIANGLE_LIST;
        simple->vdata->count = bbc->n_visibles * 4;
        simple->idata->count = bbc->n_visibles * 6;
    }
    g_buffer_unlock (bbc->vbuf);
}
static xbool
bbset_see               (gBBoardSet     *bboardset,
                         gCamera        *camera,
                         gBBoard        *bboard)
{
    return FALSE;
}
static xbool
bbset_inject            (gBBoard        *bboard,
                         struct bb_compiler *bbc)
{
    xbool per_bboard;
    gBBoardSet     *bboardset = bbc->bboardset;

    if(bboardset->culling && !bbset_see (bboardset, bbc->camera, bboard))
        return TRUE;
    x_return_val_if_fail (bbc->n_visibles < bboardset->pool_size, FALSE);

    per_bboard = (bboardset->type == G_BBT_ORIENTED_SELF
                  || bboardset->type == G_BBT_PERPENDICULAR_SELF
                  || (bboardset->accurate_face 
                      && bboardset->type != G_BBT_PERPENDICULAR_COMMON));

    if (per_bboard && !bboardset->pointing)
        gen_bboard_axes (bboardset, bbc, bboard);

    if (bboardset->common_size || bboardset->pointing) {
        if (per_bboard && !bboardset->pointing) {
            gen_bboard_voffset (bboardset, bbc,
                                bboardset->common_width,
                                bboardset->common_height,
                                bbc->bb_voffset);
        }
        gen_bboard_vertices (bboardset, bbc, bbc->bb_voffset, bboard);
    }
    else {
        if (per_bboard) {
            gvec bb_voffset[4];
            gen_bboard_voffset (bboardset, bbc,
                                bboard->width, bboard->height,
                                bb_voffset);
            gen_bboard_vertices (bboardset, bbc, bb_voffset, bboard);
        }
        else {
            gen_bboard_vertices (bboardset, bbc, bbc->bb_voffset, bboard);
        }
    }
    bbc->n_visibles++;
    return TRUE;
}
static void
bbset_enqueue           (gBBoardSet     *bboardset,
                         gMovableContext*context,
                         gPSystem       *psystem)
{
    xsize i;
    gFrustum* frustum = G_FRUSTUM(context->camera);

    if (bboardset->camera_stamp != frustum->stamp) {
        struct bb_compiler bbc = {0};
        gParticle *p;
        gxform *cam_xform = g_frustum_xform (frustum);
        gxform *node_xform = g_node_xform (context->node);

        bbc.cam_quat = cam_xform->rotate;
        bbc.cam_pos  = cam_xform->offset;
        bbc.camera = context->camera;
        bbc.bboardset = bboardset;

        bboardset->camera_stamp = frustum->stamp;
        if (!bboardset->world_space) {
            bbc.cam_quat = g_quat_mul (bbc.cam_quat, g_quat_inv (node_xform->rotate));
            bbc.cam_pos = g_vec3_inv_affine (bbc.cam_pos, node_xform);
        }
        bbc.cam_dir  = g_vec3_rotate (G_VEC_NEG_Z, bbc.cam_quat);

        if (bboardset->sorting)
            ;//bboardset_sort (bboardset, camera);


        if (psystem) {
            gBBoard bboard;
            xPtrArray *active_p = psystem->active_p;
            bbset_lock (bboardset, &bbc, active_p->len);
            for (i=0; i<active_p->len;++i) {
                p = x_ptr_array_index (active_p, gParticle, i);
                bboard.position = p->position;
                bboard.direction = p->direction;
                bboard.color = p->color;
                bboard.tex_index = 0;
                bboard.rotate = 0;
                bboard.width = p->width;
                bboard.height = p->height;
                bboard.visiable = TRUE;
                bbset_inject (&bboard, &bbc);
            }
            bbset_unlock (bboardset, &bbc);
            g_movable_resize ((gMovable*)psystem, &bbc.aabb);
            psystem->bounds_duration = 1;
        }
        else {
            bbset_lock (bboardset, &bbc, bboardset->bboards->len);
            x_array_foreach(bboardset->bboards,X_WALKFUNC(bbset_inject), &bbc);
            bbset_unlock (bboardset, &bbc);
            g_movable_resize ((gMovable*)bboardset, &bbc.aabb);
        }
    }
    g_render_queue_add (context->queue, G_RENDERABLE(bboardset), context->node);
}

static void
prender_expand_pool     (gPRender       *render,
                         xsize          pool_size)
{
    bbset_set_pool_size ((gBBoardSet*)render, pool_size);
}
static void
prender_partices_moved  (gPRender       *render,
                         xPtrArray      *active_p)
{
    gBBoardSet *bboardset = (gBBoardSet*)render;
    bboardset->camera_stamp = 0;
}
static void
prender_partice_emitted (gPRender       *render,
                         gParticle      *p)
{
    gBBoardSet *bboardset = (gBBoardSet*)render;
    bboardset->camera_stamp = 0;
    p->width = bboardset->common_width;
    p->height = bboardset->common_height;
}
static void
prender_partice_expired (gPRender       *render,
                         gParticle      *p)
{
    gBBoardSet *bboardset = (gBBoardSet*)render;
    bboardset->camera_stamp = 0;
}
static void
prender_enqueue         (gPRender       *render,
                         gMovableContext*context,
                         gPSystem       *psystem)
{
    bbset_enqueue ((gBBoardSet*)render, context, psystem);
}
static void
prender_init            (gPRenderIFace  *iface)
{
    iface->expand_pool = prender_expand_pool;
    iface->partice_emitted = prender_partice_emitted;
    iface->partice_expired = prender_partice_expired;
    iface->partices_moved = prender_partices_moved;
    iface->enqueue = prender_enqueue;
}

static void
movable_enqueue         (gMovable       *movable,
                         gMovableContext*context)
{
    bbset_enqueue ((gBBoardSet*)movable, context, NULL);
}

static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gBBoardSet *bbset = (gBBoardSet*)object;

    switch(property_id) {
    case PROP_POOL_SIZE:
        bbset_set_pool_size (bbset, x_value_get_uint (value));
        break;
    case PROP_COMMON_WIDTH:
        bbset->common_width = x_value_get_float (value);
        break;
    case PROP_COMMON_HEIGHT:
        bbset->common_height = x_value_get_float (value);
        break;
    case PROP_COMMON_SIZE:
        bbset->common_size = x_value_get_bool (value);
        break;
    case PROP_ACCURATE_FACE:
        bbset->accurate_face = x_value_get_bool (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gBBoardSet *bbset = (gBBoardSet*)object;

    switch(property_id) {
    case PROP_POOL_SIZE:
        x_value_set_uint (value, bbset->pool_size);
        break;
    case PROP_COMMON_WIDTH:
        x_value_set_float (value, bbset->common_width);
        break;
    case PROP_COMMON_HEIGHT:
        x_value_set_float (value, bbset->common_height);
        break;
    case PROP_COMMON_SIZE:
        x_value_set_bool (value, bbset->common_size);
        break;
    case PROP_ACCURATE_FACE:
        x_value_set_bool (value, bbset->accurate_face);
        break;
    }
}
static void
g_bboardset_init        (gBBoardSet     *bboardset)
{
    bboardset->origin = G_BBO_CENTER;
    bboardset->type = G_BBT_POINT;
    bboardset->common_y = G_VEC_Y;
    bboardset->common_z = G_VEC_Z;
    bboardset->texcoord = x_array_new (sizeof(grect));
    bboardset->bboards = x_array_new (sizeof(gBBoard));
    g_bboardset_texgrid (bboardset, 1, 1);
}

static void
g_bboardset_class_init  (gBBoardSetClass*klass)
{
    gMovableClass *mclass;
    xParam         *param;
    xObjectClass   *oclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;

    mclass = G_MOVABLE_CLASS (klass);
    mclass->type_flags = G_QUERY_BBOARD;
    mclass->enqueue = movable_enqueue;

    param = x_param_uint_new ("pool-size",
                              "Pool size",
                              "The size of the pool of billboards",
                              1, X_MAXUINT32, 100,
                              X_PARAM_STATIC_STR
                              | X_PARAM_READWRITE
                              | X_PARAM_CONSTRUCT);
    x_oclass_install_param (oclass, PROP_POOL_SIZE, param);

    param = x_param_float_new ("common-width",
                               "Common width",
                               "The common width of the billboards",
                               1, X_MAXFLOAT, 100,
                               X_PARAM_STATIC_STR
                               | X_PARAM_READWRITE
                               | X_PARAM_CONSTRUCT);
    x_oclass_install_param (oclass, PROP_COMMON_WIDTH, param);

    param = x_param_float_new ("common-height",
                               "Common height",
                               "The common height of the billboards",
                               1, X_MAXFLOAT, 100,
                               X_PARAM_STATIC_STR
                               | X_PARAM_READWRITE
                               | X_PARAM_CONSTRUCT);
    x_oclass_install_param (oclass, PROP_COMMON_HEIGHT, param);

    param = x_param_bool_new ("common-size",
                              "Common size",
                              "Whether the billboards is the same size",
                              TRUE,
                              X_PARAM_STATIC_STR
                              | X_PARAM_READWRITE);
    x_oclass_install_param (oclass, PROP_COMMON_SIZE, param);


    param = x_param_bool_new ("accurate-face",
                              "Accurate face",
                              "Whether the billboards face to the camera accurately",
                              FALSE,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_ACCURATE_FACE,param);
}

void
g_bboardset_texcoord    (gBBoardSet     *bboardset,
                         xsize          index,
                         xsize          count,
                         grect          *texcoord)
{
    if (bboardset->texcoord->len < index + count)
        x_array_resize (bboardset->texcoord, index+count);
    memcpy (&x_array_index (bboardset->texcoord, grect, index),
            texcoord, sizeof(grect)*count);
}

void
g_bboardset_texgrid     (gBBoardSet     *bboardset,
                         xsize          rows,
                         xsize          cols)
{
    xsize i,j,k;
    grect *rt;
    greal top,bottom;

    x_return_if_fail (G_IS_BBOARD_SET (bboardset));
    x_return_if_fail (rows > 0);
    x_return_if_fail (cols > 0);

    x_array_resize (bboardset->texcoord, rows*cols);
    for (i=0,k=0; i<rows; ++i) {
        top = (greal)i/(greal)rows;
        bottom = (greal)(i+1)/(greal)rows;
        for (j=0; j<cols; ++j) {
            rt = &x_array_index (bboardset->texcoord, grect, k);
            rt->left = (greal)j/(greal)cols;
            rt->right= (greal)(j+1)/(greal)cols;
            rt->top = top;
            rt->bottom = bottom;
            ++k;
        }
    }
}

xArray*
g_bboardset_lock        (gBBoardSet     *bboardset)
{
    x_return_val_if_fail (G_IS_BBOARD_SET (bboardset), NULL);

    return bboardset->bboards;
}
static xbool bounding_box(gBBoard *bboard, gaabb *aabb)
{
    g_aabb_extent(aabb, bboard->position);
    return TRUE;
}
void
g_bboardset_unlock      (gBBoardSet     *bboardset)
{
    gaabb aabb;
    x_return_if_fail (G_IS_BBOARD_SET (bboardset));

    bboardset->camera_stamp = 0;
    aabb.extent = 0;
    x_array_foreach (bboardset->bboards, X_WALKFUNC(bounding_box), &aabb);
    g_movable_resize ((gMovable*)bboardset, &aabb);
}
