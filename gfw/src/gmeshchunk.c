#include "config.h"
#include "gmesh-prv.h"
#include "grender.h"
#include "gmaterial.h"
#include "ganim-prv.h"
#include "gbuffer.h"
#include <string.h>
enum {
    M_HEADER                        = 0x1000,
    M_MESH                          = 0x3000,

    M_SUBMESH                       = 0x4000,
    M_SUBMESH_PRIMITIVE             = 0x4010,
    M_SUBMESH_TEXTURE_ALIAS         = 0x4200,

    M_GEOMETRY                      = 0x5000,
    M_GEOMETRY_VERTEX_DECLARATION   = 0x5100,
    M_GEOMETRY_VERTEX_ELEMENT       = 0x5110,
    M_GEOMETRY_VERTEX_BUFFER        = 0x5200,

    M_BONES_ASSIGNMENT              = 0x7000,
    M_MESH_LOD                      = 0x8000,
    M_MESH_BOUNDS                   = 0x9000,

    M_SUBMESH_NAME_TABLE            = 0xA000,
    M_SUBMESH_NAME_TABLE_ELEMENT    = 0xA100,

    M_EDGE_LISTS                    = 0xB000,
    M_EDGE_LIST_LOD                 = 0xB100,
    M_EDGE_GROUP                    = 0xB110,

    M_POSES                         = 0xC000,
    M_ANIMATIONS                    = 0xD000,
    M_TABLE_EXTREMES                = 0xE000,
};
static void
flip_vbuf               (xbyte          *base,
                         xsize          source,
                         xsize          vsize,
                         xsize          count,
                         xArray         *elements)
{
    xsize i, j;
    for (j=0; j<count; ++j, base += vsize) {
        for (i=0; i< elements->len; ++i) {
            gVertexElem *elem = &x_array_index (elements, gVertexElem, i);
            if (elem->source != source)
                continue;
            switch (elem->type) {
            case G_VERTEX_FLOAT1:
            case G_VERTEX_FLOAT2:
            case G_VERTEX_FLOAT3:
            case G_VERTEX_FLOAT4:
                x_flip_endian (base + elem->offset, 4, elem->type-G_VERTEX_FLOAT1+1);
                break;
            case G_VERTEX_A8R8G8B8:
                x_flip_endian (base + elem->offset, 4, 4);
                break;
            case G_VERTEX_SHORT1:
            case G_VERTEX_SHORT2:
            case G_VERTEX_SHORT3:
            case G_VERTEX_SHORT4:
                x_flip_endian (base + elem->offset, 2, elem->type-G_VERTEX_SHORT1+1);
                break;
            default:
                x_error_if_reached();
            }
        }
    }
}
static xptr
velem_chunk_get (gVertexData *vdata, xCache *cache, xcstr group)
{
    gVertexElem velem;
    memset(&velem, 0, sizeof(velem));
    velem.source = x_cache_geti16 (cache);
    velem.type = x_cache_geti16 (cache);
    velem.semantic = x_cache_geti16 (cache);
    velem.offset  = x_cache_geti16 (cache);
    velem.index  = x_cache_geti16 (cache);

    g_vertex_data_add (vdata, &velem);
    return NULL;
}
static void
velem_chunk_put (xFile *file, gVertexElem *elem)
{
    const xsize tag = x_chunk_begin (file, M_GEOMETRY_VERTEX_ELEMENT);

    x_file_puti16 (file, elem->source);
    x_file_puti16 (file, elem->type);
    x_file_puti16 (file, elem->semantic);
    x_file_puti16 (file, elem->offset);
    x_file_puti16 (file, elem->index);

    x_chunk_end (file, tag);
}
static xptr
vdecl_chunk_get (gVertexData *vdata, xCache *cache, xcstr group)
{
    return vdata;
}
static void
vdecl_chunk_put (xFile *file, xArray *elements)
{
    xsize i;
    const xsize tag = x_chunk_begin (file, M_GEOMETRY_VERTEX_DECLARATION);

    for (i=0;i<elements->len;++i) {
        gVertexElem *elem = &x_array_index (elements, gVertexElem, i);
        velem_chunk_put (file, elem);
    }

    x_chunk_end (file, tag); 
}
static xptr
vbuf_chunk_get (gVertexData *vdata, xCache *cache, xcstr group)
{
    gBuffer *vbuf;
    xptr p;
    xuint16 bindIndex, vertexSize;

    bindIndex = x_cache_geti16 (cache);
    vertexSize = x_cache_geti16 (cache);

    vbuf = g_vertex_buffer_new (vertexSize, vdata->count, NULL);
    p = g_buffer_lock (vbuf);
    g_vertex_data_bind (vdata, bindIndex, G_VERTEX_BUFFER (vbuf));
    x_object_unref (vbuf);

    x_cache_get (cache, p, 1, vdata->count * vertexSize);
    if (x_cache_get_flip_endian(cache))
        flip_vbuf (p, bindIndex, vertexSize, vdata->count, vdata->elements);
    g_buffer_unlock (vbuf);
    return NULL;
}
static void
vbuf_chunk_put (xFile *file, gVertexData *vdata, xuint16 bindIndex, gVertexBuffer *vbuf)
{
    xptr p;
    const xsize v_bytes = vdata->count * vbuf->vertex_size;
    const xsize tag = x_chunk_begin (file, M_GEOMETRY_VERTEX_BUFFER);

    x_file_puti16 (file, bindIndex);
    x_file_puti16 (file, vbuf->vertex_size);

    p = g_buffer_lock ((gBuffer*)vbuf);
    if (file->flip_endian) {
        xbyte *tmp = x_malloc (v_bytes);
        memcpy (tmp, p, v_bytes);
        flip_vbuf (tmp, bindIndex, vbuf->vertex_size, vdata->count, vdata->elements);
        x_file_put (file, tmp, 1, v_bytes);
        x_free (tmp);
    }
    else {
        x_file_put (file, p, 1, v_bytes);
    }
    g_buffer_unlock ((gBuffer*)vbuf);

    x_chunk_end (file, tag);
}
static xptr
geom_chunk_get (xptr object, xCache *cache, xcstr group)
{
    gVertexData **pvdata;

    if (G_IS_MESH (object))
        pvdata = &(MESH_PRIVATE(object)->vdata);
    else
        pvdata = &(((struct SubMesh*)object)->vdata);

    if (*pvdata == NULL)
        *pvdata = x_new0 (gVertexData, 1);

    (*pvdata)->count = x_cache_geti32 (cache);

    return *pvdata;
}
static void
geom_chunk_put (xFile *file, gVertexData *vdata)
{
    xsize i;
    const xsize tag = x_chunk_begin (file, M_GEOMETRY);

    x_file_puti32 (file, vdata->count);
    vdecl_chunk_put (file, vdata->elements);
    for (i=0; i<vdata->binding->len;++i) {
        gBuffer *vbuf = x_ptr_array_index(vdata->binding, gBuffer, i);
        vbuf_chunk_put (file, vdata, i, G_VERTEX_BUFFER(vbuf));
    }

    x_chunk_end (file, tag);
}
static xptr
bassign_chunk_get (xptr object, xCache *cache, xcstr group)
{
    xArray *bones;
    xsize i, j, k, vcount;

    vcount = x_cache_geti32 (cache);

    if (G_IS_MESH (object)) {
        struct MeshPrivate *priv;
        priv = MESH_PRIVATE(object);
        priv->bone_assign_dirty = TRUE;
        if (!priv->bone_assign) 
            priv->bone_assign = x_array_new_full (sizeof(gBoneAssign), vcount, NULL);
        bones =  priv->bone_assign;
    }
    else {
        struct SubMesh *submesh;
        submesh = (struct SubMesh*)object;
        if (!submesh->bone_assign) 
            submesh->bone_assign = x_array_new_full (sizeof(gBoneAssign), vcount, NULL);
        submesh->bone_assign_dirty = TRUE;
        bones =  submesh->bone_assign;
    }

    x_array_resize (bones, vcount);
    for (i=0; i<vcount; ++i) {
        gBoneAssign *assign;
        xuint32 vindex = x_cache_geti32 (cache);
        xuint16 bindex = x_cache_geti16 (cache);
        xfloat32 weight = x_cache_getf32 (cache);

        assign = x_array_at (bones, vindex);
        /* replace the min weight */
        for (j=0; j<assign->count;++j) {
            if (weight > assign->weight[j])
                break;
        }
        if (j < G_MAX_N_WEIGHTS) {
            if (assign->count < G_MAX_N_WEIGHTS)
                assign->count++;

            for (k=assign->count-1; k>j; --k) {
                assign->bindex[k] = assign->bindex[k-1];
                assign->weight[k] = assign->weight[k-1];
            }
            assign->bindex[j] = bindex;
            assign->weight[j] = weight;  
        }
    }

    return NULL;
}
static void
bassign_chunk_put (xFile *file, xArray *bones)
{
    xsize i, j;
    gBoneAssign *assign;
    const xsize tag = x_chunk_begin (file, M_BONES_ASSIGNMENT);

    x_file_puti32 (file, bones->len);
    for (i=0; i<bones->len;++i) {
        assign = &x_array_index (bones, gBoneAssign, i);
        for (j=0;j<assign->count;++j) {
            x_file_puti32 (file, i);
            x_file_puti16 (file, assign->bindex[j]);
            x_file_putf32 (file, assign->weight[j]);
        }
    }

    x_chunk_end (file, tag);
}
static xptr
submesh_chunk_get (gMesh *mesh, xCache *cache, xcstr group)
{
    struct SubMesh *submesh;
    xbool idx_bit;
    xuint32 idx_count;
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);
    submesh = x_array_append1 (priv->submesh, NULL);

    submesh->primitive = x_cache_geti16 (cache);
    submesh->material = x_strdup (x_cache_gets (cache));
    submesh->shared_vdata = x_cache_getc (cache);
    idx_count = x_cache_geti32 (cache);
    idx_bit = x_cache_getc (cache);

    if (idx_count > 0) {
        xsize index_size =  (idx_bit ? 4 : 2);//TODO
        gBuffer *ibuf = g_index_buffer_new(index_size, idx_count, NULL);
        xptr p = g_buffer_lock (ibuf);
        x_cache_get (cache, p, index_size, idx_count);
        g_buffer_unlock (ibuf);
        if (submesh->idata)
            g_index_data_delete (submesh->idata);
        submesh->idata = g_index_data_new (idx_count);
        g_index_data_bind (submesh->idata, G_INDEX_BUFFER (ibuf));
        x_object_unref (ibuf);
    }
    return submesh;
}
static void
submesh_chunk_put (xFile *file, struct SubMesh *submesh)
{
    xbool idx_bit = FALSE;
    xuint32 idx_count = 0;
    const xsize tag = x_chunk_begin (file, M_SUBMESH);

    if (submesh->idata) {
        idx_count = submesh->idata->count;
        if (submesh->idata->buffer)
            idx_bit = submesh->idata->buffer->index_size==sizeof(xuint32);
    }
    x_file_puti16 (file, submesh->primitive);
    x_file_puts (file, submesh->material, TRUE);
    x_file_putc (file, submesh->shared_vdata);
    x_file_puti32 (file, idx_count);
    x_file_putc (file, idx_bit);

    if (idx_count >0) {
        xptr p = g_buffer_lock (G_BUFFER(submesh->idata->buffer));
        if (idx_bit)
            x_file_put (file, p, sizeof(xuint32), submesh->idata->count);
        else
            x_file_put (file, p, sizeof(xuint16), submesh->idata->count);
        g_buffer_unlock (G_BUFFER(submesh->idata->buffer));
    }
    if (!submesh->shared_vdata)
        geom_chunk_put (file, submesh->vdata);

    if (submesh->bone_assign)
        bassign_chunk_put (file, submesh->bone_assign);

    x_chunk_end (file, tag);
}
static xptr
mesh_chunk_get (gMesh *mesh, xCache *cache, xcstr group)
{
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);
    
    if (priv->skeleton)
        x_free (priv->skeleton);
    priv->skeleton = x_strdup (x_cache_gets(cache));
    priv->audo_edge = FALSE;
    return mesh;
}
static xptr
bounds_chunk_get (gMesh *mesh, xCache *cache, xcstr group)
{
    xfloat32 min[3],max[3], radius;
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);
    x_cache_get (cache, min, sizeof(xfloat32), 3);
    x_cache_get (cache, max, sizeof(xfloat32), 3);
    radius = x_cache_getf32 (cache);

    priv->aabb.extent = 1;
    priv->aabb.minimum = g_vec(min[0], min[1], min[2], 0);
    priv->aabb.maximum = g_vec(max[0], max[1], max[2], 0);
    radius *= radius;
    priv->radius_sq = g_vec (radius, radius, radius, radius);
    return NULL;
}
static void
bounds_chunk_put (xFile *file, gMesh *mesh)
{
    xfloat32 min[4],max[4], radius[4];
    const xsize tag = x_chunk_begin (file, M_MESH_BOUNDS);
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);
    g_vec_store (priv->aabb.minimum, min);
    g_vec_store (priv->aabb.maximum, max);
    g_vec_store (priv->radius_sq, radius);
    x_file_put (file, min, sizeof(xfloat32), 3);
    x_file_put (file, max, sizeof(xfloat32), 3);
    x_file_putf32 (file, sqrtf(radius[0]));

    x_chunk_end (file, tag);
}
static xptr
edge_lists (gMesh *mesh, xCache *cache, xcstr group)
{
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);
    priv->edge_built = TRUE;
    return mesh;
}
static void
edge_group_free (gEdgeGroup *group)
{
    x_array_unref (group->edges);
    /* free by submesh
     * g_vertex_data_delete (group->vdata);
     */
}
static void
edge_data_free          (gEdgeData      *edge_data)
{
    x_array_unref (edge_data->triangles);
    x_align_free (edge_data->normals);
    x_free (edge_data->faceings);
    x_array_unref (edge_data->groups);
    x_free (edge_data);
}
static void
mesh_lod_usage_free     (gMeshLodUsage  *usage)
{
    if (usage->edge_data) {
        edge_data_free (usage->edge_data);
        usage->edge_data = NULL;
    }
}
static xptr
edge_list_lod (gMesh *mesh, xCache *cache, xcstr group)
{
    xbyte closed;
    xuint32 triangles;
    xuint32 groups;
    xuint32 tmp[3];
    xuint32 i;
    xsize lodIndex;
    xbool isManual;
    gMeshLodUsage * usage;
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);

    lodIndex = x_cache_geti16 (cache);
    isManual = x_cache_getc (cache);

    if (!priv->lod_usage)
        priv->lod_usage = x_array_new_full (sizeof(gMeshLodUsage), 0, mesh_lod_usage_free);

    if (priv->lod_usage->len < lodIndex+1)
        x_array_resize (priv->lod_usage, lodIndex+1);

    usage = &x_array_index (priv->lod_usage, gMeshLodUsage, lodIndex);

    x_cache_get (cache, &closed, sizeof(xbyte), 1);
    x_cache_get (cache, &triangles, sizeof(xuint32), 1);
    x_cache_get (cache, &groups, sizeof(xuint32), 1);

    usage->edge_data = x_malloc0(sizeof (gEdgeData));
    usage->edge_data->closed = closed;
    usage->edge_data->triangles = x_array_new_full (sizeof (gTriangle), triangles, NULL);
    usage->edge_data->normals = x_align_malloc(sizeof(gvec) * triangles, G_VEC_ALIGN);
    usage->edge_data->faceings = x_malloc0 (sizeof (xbyte) * triangles);
    usage->edge_data->groups = x_array_new_full (sizeof (gEdgeGroup), groups, edge_group_free);

    for (i = 0; i < triangles; ++i) {
        gTriangle *tri;
        greal normal[4];
        tri = x_array_append1 (usage->edge_data->triangles, NULL);
        x_cache_get (cache, tmp, sizeof(xuint32), 1);
        tri->index_set = tmp[0];

        x_cache_get (cache, tmp, sizeof(xuint32), 1);
        tri->vertex_set = tmp[0];

        x_cache_get (cache, tmp, sizeof(xuint32), 3);
        tri->vert_index[0] = tmp[0];
        tri->vert_index[1] = tmp[1];
        tri->vert_index[2] = tmp[2];

        x_cache_get (cache, tmp, sizeof(xuint32), 3);
        tri->shared_vindex[0] = tmp[0];
        tri->shared_vindex[1] = tmp[1];
        tri->shared_vindex[2] = tmp[2];

        x_cache_get (cache, normal, sizeof(greal), 4);
        usage->edge_data->normals[i] = g_vec_load(normal);
    }

    return usage->edge_data;
}
static xptr
edge_group (gEdgeData *edge_data, xCache *cache, xcstr res_group)
{
    xuint32 e;
    xuint32 numEdges;
    xuint32 tmp[3];
    gEdgeGroup *group;

    group = x_array_append1 (edge_data->groups, NULL);
    x_cache_get (cache, tmp, sizeof(xuint32), 1);
    group->vdata_idx = tmp[0];

    x_cache_get (cache, tmp, sizeof(xuint32), 1);
    group->tri_start = tmp[0];

    x_cache_get (cache, tmp, sizeof(xuint32), 1);
    group->tri_count= tmp[0];

    x_cache_get (cache, &numEdges, sizeof(xuint32), 1);
    group->edges = x_array_new_full (sizeof (gEdge), numEdges, NULL);

    for (e = 0; e < numEdges; ++e) {
        xbyte degenerate;
        gEdge* edge;
        edge = x_array_append1 (group->edges, NULL);

        x_cache_get (cache, tmp, sizeof(xuint32), 2);
        edge->tri_index[0] = tmp[0];
        edge->tri_index[1] = tmp[1];

        x_cache_get (cache, tmp, sizeof(xuint32), 2);
        edge->vert_index[0] = tmp[0];
        edge->vert_index[1] = tmp[1];

        x_cache_get (cache, tmp, sizeof(xuint32), 2);
        edge->shared_vindex[0] = tmp[0];
        edge->shared_vindex[1] = tmp[1];

        x_cache_get (cache, &degenerate, sizeof(xbyte), 1);
        edge->degenerate = degenerate;
    }
    return NULL;
}
void
_g_mesh_chunk_put (gMesh *mesh, xFile *file)
{
    xsize i;
    struct MeshPrivate *priv;
    const xsize tag = x_chunk_begin (file, M_MESH);

    priv = MESH_PRIVATE(mesh);
    x_file_puts (file, priv->skeleton, TRUE);
    if (priv->vdata && priv->vdata->count > 0)
        geom_chunk_put (file, priv->vdata);

    for (i=0; i<priv->submesh->len;++i) {
        struct SubMesh *submesh;
        submesh = &x_array_index (priv->submesh, struct SubMesh, i);
        submesh_chunk_put (file, submesh);
    }

    if (priv->bone_assign)
        bassign_chunk_put (file, priv->bone_assign);

    bounds_chunk_put (file, mesh);

    x_chunk_end (file, tag);
}

void
_g_mesh_chunk_register (void)
{
    xChunk *mesh, *geom, *submesh, *edges, *bassign;

    mesh = x_chunk_set (NULL, M_MESH, mesh_chunk_get, NULL);
    x_chunk_set (mesh, M_MESH_BOUNDS, bounds_chunk_get, NULL); 

    bassign = x_chunk_set (mesh, M_BONES_ASSIGNMENT, bassign_chunk_get, NULL); 
    geom = x_chunk_set (mesh, M_GEOMETRY, geom_chunk_get, NULL); {
        xChunk *vdecl;
        x_chunk_set (geom, M_GEOMETRY_VERTEX_BUFFER, vbuf_chunk_get, NULL);
        vdecl = x_chunk_set (geom, M_GEOMETRY_VERTEX_DECLARATION, vdecl_chunk_get, NULL); {
            x_chunk_set (vdecl, M_GEOMETRY_VERTEX_ELEMENT, velem_chunk_get, NULL);
        }
    }
    edges = x_chunk_set (mesh, M_EDGE_LISTS, edge_lists, NULL); {
        xChunk *edge_lod;
        edge_lod = x_chunk_set (edges, M_EDGE_LIST_LOD, edge_list_lod, NULL); {
            x_chunk_set (edge_lod, M_EDGE_GROUP, edge_group, NULL);
        }
    }
    submesh = x_chunk_set (mesh, M_SUBMESH, submesh_chunk_get, NULL); {
        x_chunk_link (submesh, M_GEOMETRY, geom);
        x_chunk_link (submesh, M_BONES_ASSIGNMENT , bassign);
    }
}
