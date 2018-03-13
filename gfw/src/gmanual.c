#include "config.h"
#include "gmanual.h"
#include "gbuffer.h"
#include "grender.h"
#include "grenderqueue.h"
#include "grenderable.h"
#include "gmaterial.h"
#include "gnode.h"
#include <string.h>
struct section {
    gPrimitiveType  primitive;
    gMaterial       *material;
    gaabb           aabb;
    gVertexData     vdata;
    gIndexData      idata;
};
static gTechnique*
manual_technique        (gRenderable    *renderable,
                         xptr           user_data)
{
    struct section *section;
    gManual *manual = (gManual*)renderable;
    xsize i = X_PTR_TO_SIZE (user_data);

    section = &x_array_index(manual->sections, struct section, i);
    return g_material_technique(section->material);
}

static xbool
manual_proj_matrix      (gRenderable    *renderable,
                         gmat4          *mat,
                         xptr           user_data)
{
    gManual *manual = (gManual*)renderable;
    return 0;
}
static xbool
manual_view_matrix      (gRenderable    *renderable,
                         gmat4          *mat,
                         xptr           user_data)
{
    gManual *manual = (gManual*)renderable;
    return FALSE;
}
static xbool
manual_cast_shadow      (gRenderable    *renderable,
                         xptr           user_data)
{
    gManual *manual = (gManual*)renderable;
    return FALSE;
}
static void
manual_render_op        (gRenderable    *renderable,
                         gRenderOp      *op,
                         xptr           user_data)
{
    gManual *manual = (gManual*)renderable;
    xuint i = X_PTR_TO_SIZE (user_data);
    struct section *section;

    section = &x_array_index(manual->sections, struct section, i);
    op->primitive = section->primitive;
    op->vdata = &section->vdata;
    if (section->idata.count > 0)
        op->idata = &section->idata;
    op->renderable = renderable;
}
static void
renderable_init         (gRenderableIFace *iface)
{
    iface->technique  = manual_technique;
    iface->view_matrix = manual_view_matrix;
    iface->proj_matrix = manual_proj_matrix;
    iface->cast_shadow = manual_cast_shadow;
    iface->render_op   = manual_render_op;
}
X_DEFINE_TYPE_EXTENDED (gManual, g_manual, G_TYPE_MOVABLE,0,
                        X_IMPLEMENT_IFACE (G_TYPE_RENDERABLE, renderable_init));

static void
manual_enqueue          (gMovable       *movable,
                         gMovableContext*context)
{
    xsize i;
    gManual *manual = (gManual*)movable;

    for(i=0;i<manual->sections->len; ++i)
        g_render_queue_add_data (context->queue, G_RENDERABLE(manual), context->node, X_SIZE_TO_PTR(i));
}
static void
g_manual_init           (gManual        *manual)
{
    manual->est_nindex = 50;
    manual->est_nvertex = 50;
    manual->est_vsize = sizeof(float) * 12;
}

static void
g_manual_class_init     (gManualClass   *klass)
{
    gMovableClass *mclass;

    mclass = G_MOVABLE_CLASS (klass);
    mclass->enqueue = manual_enqueue;
}

static void
resize_tmp_vbuf         (gManual        *manual,
                         xsize          n_vertex)
{
    xsize new_size;
    if (manual->first_v)
        new_size = manual->est_vsize * n_vertex;
    else
        new_size = manual->decl_size * n_vertex;
    if (new_size > manual->tmp_vbuf_size) {
        if (!manual->tmp_vbuf)
            new_size = manual->est_vsize * manual->est_nvertex;
        else
            new_size = MAX (new_size, manual->tmp_vbuf_size*2);
        manual->tmp_vbuf = x_realloc (manual->tmp_vbuf, new_size);
        manual->tmp_vbuf_size = new_size;
    }
}
void
resize_tmp_ibuf         (gManual        *manual,
                         xsize          n_index)
{
    xsize new_size = n_index *sizeof (xuint32);
    if (new_size > manual->tmp_ibuf_size) {
        if (!manual->tmp_ibuf)
            new_size = manual->est_nindex * sizeof (xuint32);
        else
            new_size = MAX (new_size, manual->tmp_ibuf_size*2);
        manual->tmp_ibuf = x_realloc (manual->tmp_ibuf, new_size);
        manual->tmp_ibuf_size = new_size;
    }
}
void
free_tmp_buf            (gManual        *manual)
{
    x_free (manual->tmp_vbuf);
    manual->tmp_vbuf = NULL;
    manual->tmp_vbuf_size = 0;

    x_free (manual->tmp_ibuf);
    manual->tmp_ibuf = NULL;
    manual->tmp_ibuf_size = 0;
}
static void
copy_tmp_vbuf_to_buffer (gManual        *manual)
{
    xsize i;
    xbyte *base;
    struct section *current = manual->current;
    gVertexData *vdata = &current->vdata;

    /* first vertex, autoorganise decl */
    resize_tmp_vbuf (manual, vdata->count + 1);
    base  = manual->tmp_vbuf + (manual->decl_size * vdata->count);
    for (i=0; i< vdata->elements->len; ++i) {
        xptr p;
        gVertexElem *elem = &x_array_index (vdata->elements, gVertexElem, i);
        if (elem->source != 0)
            continue;
        p = base + elem->offset;
        switch (elem->semantic){
        case G_VERTEX_POSITION:
            x_assert (elem->type == G_VERTEX_FLOAT3);
            memcpy(p, &manual->tmp_v.pos, sizeof(greal)*3);
            break;
        case G_VERTEX_NORMAL:
            x_assert (elem->type == G_VERTEX_FLOAT3);
            memcpy(p, &manual->tmp_v.normal, sizeof(greal)*3);
            break;
        case G_VERTEX_TEXCOORD:
            x_assert (elem->type >= G_VERTEX_FLOAT1
                      && elem->type <= G_VERTEX_FLOAT3);
            memcpy(p, &manual->tmp_v.texcoord[elem->index],
                   sizeof(greal)*(elem->type-G_VERTEX_FLOAT1+1));
            break;
        case G_VERTEX_DIFFUSE:
            x_assert (elem->type == G_VERTEX_A8R8G8B8);
            *(xuint32*)p = g_vec_color(manual->tmp_v.color);
            break;
        default:
            x_error_if_reached();
        }
    }
    vdata->count++;
}
void
g_manual_estimate       (gManual        *manual,
                         xsize          vertex_size,
                         xsize          n_vertex,
                         xsize          n_index)
{
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current == NULL);

    manual->est_vsize = vertex_size;
    manual->est_nvertex = n_vertex;

    resize_tmp_vbuf (manual, n_vertex);

    manual->est_nindex = n_index;
    resize_tmp_ibuf (manual, n_index);
}
static void
manual_finalize         (xObject        *object)
{
    gManual * manual = (gManual*)object;
    free_tmp_buf (manual);
    x_array_unref (manual->sections);
}

void
g_manual_begin          (gManual        *manual,
                         xcstr          material,
                         xint           primitive)
{
    struct section *current;
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current == NULL);

    if (!manual->sections)
        manual->sections = x_array_new (sizeof(struct section));

    current = x_array_append1 (manual->sections, NULL);
    current->primitive = primitive;
    current->material = g_material_attach(material, current);
    current->aabb.extent = 0;

    manual->first_v = TRUE;
    manual->decl_size = 0;
    manual->tex_index = 0;
    manual->cur_updating = FALSE;
    manual->bit32_ibuf = FALSE;
    manual->tmp_v_pending = FALSE;
    manual->current = current;
}
void
g_manual_update         (gManual        *manual,
                         xsize          section_idx)
{
    xsize i;
    gVertexData *vdata;
    struct section *current;

    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current == NULL);
    x_return_if_fail (manual->sections != NULL);
    x_return_if_fail (section_idx < manual->sections->len);

    current = &x_array_index (manual->sections, struct section, section_idx);

    current->vdata.count = 0;
    current->idata.count = 0;
    current->aabb.extent = 0;

    /* aready have the vertex declare size */
    vdata = &current->vdata;
    x_return_if_fail (vdata->elements->len != 0);
    manual->decl_size = 0;
    for (i=0; i<vdata->elements->len; ++i) {
        gVertexElem *elem = &x_array_index (vdata->elements, gVertexElem, i);
        if (elem->source == 0) {
            manual->decl_size += g_vertex_type_size (elem->type);
            if (elem->semantic == G_VERTEX_TEXCOORD) {
                manual->tmp_v.texdim[elem->index] = elem->type-G_VERTEX_FLOAT1+1;
            }
        }
    }
    manual->first_v = TRUE;
    manual->tex_index = 0;
    manual->cur_updating = TRUE;
    manual->bit32_ibuf = FALSE;
    manual->tmp_v_pending = FALSE;
    manual->current = current;
}

void
g_manual_position       (gManual        *manual,
                         gvec           pos)
{
    struct section *current;
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current != NULL);

    current = manual->current;
    if (manual->tmp_v_pending) {
        copy_tmp_vbuf_to_buffer (manual);
        manual->first_v = FALSE;
    }
    if (manual->first_v && !manual->cur_updating) {
        gVertexElem velem;
        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_POSITION;
        velem.type = G_VERTEX_FLOAT3;
        g_vertex_data_add (&current->vdata, &velem);
    }
    manual->tmp_v.pos = pos;
    g_aabb_extent (&current->aabb, pos);
    manual->tex_index = 0;
    manual->tmp_v_pending = TRUE;
}

void
g_manual_normal         (gManual        *manual,
                         gvec           normal)
{
    struct section *current;
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current != NULL);

    current = manual->current;
    if (manual->first_v && !manual->cur_updating) {
        gVertexElem velem;
        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_NORMAL;
        velem.type = G_VERTEX_FLOAT3;
        g_vertex_data_add (&current->vdata, &velem);
    }
    manual->tmp_v.normal = normal;
}

void
g_manual_texcoord       (gManual        *manual,
                         xsize          dim,
                         gvec           texcoord)
{
    struct section *current;
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current != NULL);

    current = manual->current;
    if (manual->first_v && !manual->cur_updating) {
        gVertexElem velem;

        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_TEXCOORD;
        velem.type  = G_VERTEX_FLOAT1+dim-1;
        velem.index = manual->tex_index;
        g_vertex_data_add (&current->vdata, &velem);
        manual->tmp_v.texdim[manual->tex_index] = dim;
    }
    else {
        x_assert (manual->tmp_v.texdim[manual->tex_index] == dim);
    }
    manual->tmp_v.texcoord[manual->tex_index] = texcoord;
    ++manual->tex_index;
}

void
g_manual_color          (gManual        *manual,
                         gcolor         color)
{
    struct section *current;
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current != NULL);

    current = manual->current;
    if (manual->first_v && !manual->cur_updating) {
        gVertexElem velem;

        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_DIFFUSE;
        velem.type  = G_VERTEX_A8R8G8B8;

        g_vertex_data_add (&current->vdata, &velem);
    }
    manual->tmp_v.color = color;
}

void
g_manual_index          (gManual        *manual,
                         xsize          index)
{
    struct section *current;
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current != NULL);

    current = manual->current;
    if (index > 0xFFFF)
        manual->bit32_ibuf = TRUE;

    resize_tmp_ibuf (manual, ++current->idata.count);
    manual->tmp_ibuf[current->idata.count-1] = index;
}

void
g_manual_triangle       (gManual        *manual,
                         xsize          index0,
                         xsize          index1,
                         xsize          index2)
{
    struct section *current;
    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current != NULL);

    current = manual->current;
    x_return_if_fail (current->primitive == G_RENDER_TRIANGLE_LIST);

    g_manual_index (manual, index0); 
    g_manual_index (manual, index1); 
    g_manual_index (manual, index2); 
}

void
g_manual_quad           (gManual        *manual,
                         xsize          index0,
                         xsize          index1,
                         xsize          index2,
                         xsize          index3)
{
    g_manual_triangle (manual, index0, index1, index2);
    g_manual_triangle (manual, index2, index3, index0);
}

void
g_manual_end            (gManual        *manual)
{
    gVertexData *vdata;
    gIndexData  *idata;
    gVertexBuffer *vbuf;
    gIndexBuffer *ibuf;
    xsize i, index_size;
    gaabb aabb;
    xbool new_ibuf, new_vbuf;
    struct section *current;

    x_return_if_fail (G_IS_MANUAL (manual));
    x_return_if_fail (manual->current != NULL);

    current = manual->current;
    if (manual->tmp_v_pending)
        copy_tmp_vbuf_to_buffer (manual);

    vdata = &current->vdata;
    idata = &current->idata;

    x_return_if_fail (vdata->count > 0);
    new_ibuf = (idata->count > 0);
    new_vbuf = TRUE;

    index_size = (manual->bit32_ibuf ? sizeof(xuint32) : sizeof(xuint16));
    if (manual->cur_updating) {

        vbuf = G_VERTEX_BUFFER(g_vertex_data_get (vdata, 0));
        if (vbuf && vbuf->vertex_count >= vdata->count)
            new_vbuf = FALSE;

        if (new_vbuf) {
            ibuf = G_INDEX_BUFFER(g_index_data_get(idata));
            if (ibuf && ibuf->index_count >= idata->count
                && ibuf->index_size == index_size)
                new_ibuf = FALSE;
        }
    }

    if (new_vbuf) {
        vbuf = G_VERTEX_BUFFER (g_vertex_buffer_new(manual->decl_size, vdata->count, NULL));
        g_vertex_data_bind (vdata, 0, vbuf);
        x_object_unref (vbuf);
    }
    if (new_ibuf) {
        ibuf = G_INDEX_BUFFER (g_index_buffer_new (index_size, idata->count, NULL));
        g_index_data_bind (idata, ibuf);
        x_object_unref (ibuf);
    }
    g_buffer_write (G_BUFFER (vbuf), 0,
                    vdata->count*vbuf->vertex_size,
                    manual->tmp_vbuf);
    if (idata->count > 0) {
        if (index_size == sizeof(xuint32)) {
            g_buffer_write (G_BUFFER (ibuf), 0,
                            idata->count * sizeof(xuint32),
                            manual->tmp_ibuf);
        }
        else {
            xsize i;
            xuint16 *p = g_buffer_lock (G_BUFFER (ibuf));
            xuint32 *pInt32 = manual->tmp_ibuf;
            for (i=0; i< idata->count; ++i)
                *p++ = (xuint16)*pInt32++;
            g_buffer_unlock (G_BUFFER (ibuf));
        }
    }
    manual->current = NULL;
    free_tmp_buf (manual);
    aabb.extent = 0;
    for (i=0;i<manual->sections->len;++i) {
        current = &x_array_index(manual->sections, struct section, i);
        g_aabb_merge (&aabb, &current->aabb);
    }
    g_movable_resize ((gMovable*)manual, &aabb);
}

gMesh*
g_manual_to_mesh        (gManual        *manual,
                         xcstr          mesh,
                         xcstr          group)
{
    return NULL;
}
