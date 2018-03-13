#include <gfw.h>
#include <xfw.h>
#include <string.h>
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
static xbool _g_mesh_ogre = 1;
#define FLIP_M_HEADER       0x0010
#define HEAD_COUNT          6
static xuint16
read_head               (xCache         *cache,
                         xuint32        *count)
{
    xuint16 id;

    if (1 != x_cache_get (cache, &id, sizeof(xuint16), 1))
        return 0;
    if (1 != x_cache_get (cache, count, sizeof(xuint32), 1))
        return 0;
    return id;
}

#define BEGIN_READ_CHECK(id, cache, count, max_count)                   \
    xbool __no_handle = FALSE;                                          \
if (!_g_mesh_ogre && count > max_count){                                \
    x_error ("file %s: line %d: mesh chunk[x%x] end(%d) over limit(%d)",\
             __FILE__, __LINE__, id, count, max_count);                 \
}

#define DEFAULT_READ_CHECK() __no_handle = TRUE;break

#define END_READ_CHECK(id, cache, count, max_count)                     \
    if (__no_handle) {/* no handle this id */                           \
        if (!_g_mesh_ogre) {                                            \
            x_warning("file %s: line %d: unknown mesh chunk[x%x]",      \
                      __FILE__, __LINE__, id);                          \
            x_cache_skip (cache, count - cache->count);                 \
        } else {                                                        \
            x_cache_back (cache, HEAD_COUNT);                           \
            break;                                                      \
        }                                                               \
    } else if (max_count == cache->count) {                             \
        break;                                                          \
    } else if (count != cache->count) {                                 \
        if (!_g_mesh_ogre) {                                            \
            if (count > cache->count) {                                 \
                x_warning("file %s: line %d: "                          \
                          "mesh chunk[x%x] not end(%d) at bound(%d)",   \
                          __FILE__, __LINE__, id, cache->count, count); \
                x_cache_skip (cache, count - cache->count);             \
            } else if (count < cache->count) {                          \
                x_error ("file %s: line %d: "                           \
                         "mesh chunk[x%x] end(%d) over bound(%d)",      \
                         __FILE__, __LINE__, id, cache->count, count);  \
            }                                                           \
        }                                                               \
        else x_critical ("file %s: line %d: "                           \
                         "mesh chunk[x%x] not end(%d) at bound(%d)",    \
                         __FILE__, __LINE__, id, cache->count, count);  \
    }

#define BEGIN_WRITE_CHECK(file, id, count, max_count)                   \
    xsize __fcount = x_file_tell (file);                                \
if (count > max_count) {                                                \
    x_error ("file %s: line %d: "                                       \
             "mesh chunk[%s] end(%d) over bound(%d)",                   \
             __FILE__, __LINE__, #id, count, max_count);                \
}                                                                       \
max_count = count;

#define END_WRITE_CHECK(file, id, count, max_count)                     \
    __fcount = x_file_tell (file) - __fcount;                           \
x_assert(!max_count);                                                   \
if (__fcount != count) {                                                \
    x_error ("file %s: line %d: "                                       \
             "mesh chunk[%s] not end(%d) at bound(%d)",                 \
             __FILE__, __LINE__, #id, __fcount, count);                 \
}


static void
read_geometry_velem     (xCache         *cache,
                         gMesh          *mesh,
                         gVertexData    *vdata)
{
    xint16 source, offset, type, semantic, index;
    x_cache_get (cache, &source, sizeof(xint16), 1);
    x_cache_get (cache, &type, sizeof(xint16), 1);
    x_cache_get (cache, &semantic, sizeof(xint16), 1);
    x_cache_get (cache, &offset, sizeof(xint16), 1);
    x_cache_get (cache, &index, sizeof(xint16), 1);

    g_vertex_data_add (vdata, source, offset, type, semantic, index);
}
static void
read_geometry_vdecl     (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh,
                         gVertexData    *vdata)
{
    xuint16 id;
    xuint32 count;

    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK(id, cache, count, max_count);
        switch (id) {
        case M_GEOMETRY_VERTEX_ELEMENT:
            read_geometry_velem (cache, mesh, vdata);
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK(id, cache, count, max_count);
    }
}
static void
read_geometry_vbuf      (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh,
                         gVertexData    *vdata)
{
    xuint16 id;
    xuint32 count;
    xuint16 bindIndex, vertexSize;

    x_cache_read (cache, &bindIndex, sizeof(xuint16), 1);
    x_cache_read (cache, &vertexSize, sizeof(xuint16), 1);

    id=read_head (cache, &count);
    if (id == M_GEOMETRY_VERTEX_BUFFER_DATA) {
        gBuffer *vbuf;
        xptr p;
        vbuf = g_vertex_buffer_new (vertexSize, vdata->count, NULL);
        p = g_buffer_lock (vbuf);
        g_vertex_data_binding (vdata, bindIndex, G_VERTEX_BUFFER (vbuf));
        x_object_unref (vbuf);
        x_cache_read (cache, p, 1, vdata->count * vertexSize);
        if (cache->flip_endian)
            flip_vbuf (p, bindIndex, vertexSize, vdata->count, vdata->elements);
        g_buffer_unlock (vbuf);
    }
    else {
        x_error("no geometry buffer data");
    }
}
static void
read_geometry           (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh,
                         gVertexData    **pvdata)
{
    xuint16 id;
    xuint32 count;
    gVertexData *vdata;

    x_cache_read (cache, &count, sizeof(xuint32),1);
    *pvdata = vdata = x_new0 (gVertexData, 1);
    vdata->start = 0;
    vdata->count = count;
    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK(id, cache, count, max_count);
        switch (id) {
        case M_GEOMETRY_VERTEX_DECLARATION:
            read_geometry_vdecl (cache, count, mesh, vdata);
            break;
        case M_GEOMETRY_VERTEX_BUFFER:
            read_geometry_vbuf (cache, count, mesh, vdata);
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, max_count);
    }
}
static void
read_submesh_primitive  (xCache         *cache,
                         gMesh          *mesh,
                         gSubMesh       *submesh)
{
    xuint16 primitive;
    x_cache_read (cache, &primitive, sizeof(xuint16), 1);
    if (!_g_mesh_ogre)
        submesh->primitive = primitive;
    else
        submesh->primitive = primitive - 1;

}
static void
read_submesh_bone_assign(xCache         *cache,
                         gMesh          *mesh,
                         gSubMesh       *submesh)
{
    xArray *blends;
    struct BoneAssign *assign;
    xuint32 vindex;
    xuint16 bindex;
    greal weight;

    x_cache_read (cache, &vindex, sizeof(xuint32), 1); 
    x_cache_read (cache, &bindex, sizeof(xuint16), 1); 
    x_cache_read (cache, &weight, sizeof(greal), 1); 

    x_assert (submesh->shared_vdata == FALSE);

    if (!submesh->bone_assign) {
        submesh->bone_assign = x_ptr_array_new (submesh->vdata->count);
        submesh->bone_assign->free_func = x_array_unref;
        x_ptr_array_resize (submesh->bone_assign, submesh->vdata->count);
    }
    blends = x_ptr_array_index (submesh->bone_assign, xArray, vindex);
    if (!blends) {
        blends = x_array_new (sizeof(struct BoneAssign), G_MAX_N_BLEND_WEIGHTS);
        submesh->bone_assign->data[vindex] = blends;
    }
    assign = x_array_append1 (blends, NULL);
    assign->bindex = bindex;
    assign->weight = weight;
    submesh->bone_assign_dirty = TRUE;
}
static void
read_submesh            (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xuint16 id;
    xuint32 count;
    xcstr material;
    gSubMesh *submesh;
    xbyte shared_vdata,bit32_ibuf;
    xuint32 index_count;

    material = x_cache_gets (cache);
    submesh = x_array_append1 (mesh->submesh, NULL);
    submesh->material = x_strdup (material);

    x_cache_read (cache, &shared_vdata, sizeof(xbyte), 1);
    x_cache_read (cache, &index_count, sizeof(xuint32), 1);
    x_cache_read (cache, &bit32_ibuf, sizeof(xbyte), 1);

    submesh->shared_vdata = shared_vdata;
    if (index_count > 0) {
        xsize index_size =  (bit32_ibuf ? 4 : 2);
        gBuffer *ibuf = g_index_buffer_new(index_size, index_count, NULL);
        xptr p = g_buffer_lock (ibuf);
        x_cache_read (cache, p, index_size, index_count);
        g_buffer_unlock (ibuf);
        if (submesh->idata)
            g_index_data_delete (submesh->idata);
        submesh->idata = g_index_data_new (index_count);
        g_index_data_binding (submesh->idata, G_INDEX_BUFFER (ibuf));
        x_object_unref (ibuf);
    }
    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, max_count);
        switch (id) {
        case M_GEOMETRY:
            read_geometry(cache, count, mesh, &submesh->vdata);
            break;
        case M_SUBMESH_PRIMITIVE:
            read_submesh_primitive (cache, mesh, submesh);
            break;
        case M_SUBMESH_BONE_ASSIGNMENT:
            read_submesh_bone_assign (cache, mesh, submesh);
            break;
        case M_SUBMESH_TEXTURE_ALIAS:
            //read_submesh_texture_alias (cache, mesh);
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, max_count);
    }
}
static void
read_skeleton_link      (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    x_free (mesh->skeleton);
    mesh->skeleton = x_strdup (x_cache_gets(cache));
}
static void
read_mesh_bone_assign   (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xArray *blends;
    struct BoneAssign *assign;
    xuint32 vindex;
    xuint16 bindex;
    greal weight;

    x_cache_read (cache, &vindex, sizeof(xuint32), 1); 
    x_cache_read (cache, &bindex, sizeof(xuint16), 1); 
    x_cache_read (cache, &weight, sizeof(xfloat32), 1); 

    if (!mesh->bone_assign) {
        mesh->bone_assign = x_ptr_array_new (mesh->vdata->count);
        mesh->bone_assign->free_func = x_array_unref;
        x_ptr_array_resize (mesh->bone_assign, mesh->vdata->count);
    }
    blends = x_ptr_array_index (mesh->bone_assign, xArray, vindex);
    if (!blends) {
        blends = x_array_new (sizeof(struct BoneAssign), G_MAX_N_BLEND_WEIGHTS);
        mesh->bone_assign->data[vindex] = blends;
    }
    assign = x_array_append1 (blends, NULL);
    assign->bindex = bindex;
    assign->weight = weight;
    mesh->bone_assign_dirty = TRUE;
}
static void
read_bounds             (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    greal min[3],max[3], radius;
    x_cache_read (cache, min, sizeof(greal), 3);
    x_cache_read (cache, max, sizeof(greal), 3);
    x_cache_read (cache, &radius, sizeof(greal), 1);
    mesh->aabb.extent = 1;
    mesh->aabb.minimum = g_vec(min[0], min[1], min[2], 0);
    mesh->aabb.maximum = g_vec(max[0], max[1], max[2], 0);
    radius *= radius;
    mesh->radius_sq = g_vec (radius, radius, radius, radius);
}
static void
read_submesh_name_table (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xuint16 id;
    xuint32 count;
    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, max_count);
        switch (id) {
        case M_SUBMESH_NAME_TABLE_ELEMENT:
            {
                xuint16 submesh_index;
                xcstr name;
                x_cache_read (cache, &submesh_index, sizeof(xuint16), 1);
                name = x_cache_gets (cache);
                x_array_index (mesh->submesh,gSubMesh,submesh_index).name = x_strdup (name);
            }
            break;
        default:
            DEFAULT_READ_CHECK();

        }
        END_READ_CHECK (id, cache, count, max_count);
    }
}
static void
read_edge_group         (xCache         *cache,
                         xuint32        max_count,
                         struct EdgeGroup *group)
{
    xuint32 e;
    xuint32 numEdges;
    xuint32 tmp[3];

    x_cache_read (cache, tmp, sizeof(xuint32), 1);
    group->vertex_set = tmp[0];

    x_cache_read (cache, tmp, sizeof(xuint32), 1);
    group->tri_start = tmp[0];

    x_cache_read (cache, tmp, sizeof(xuint32), 1);
    group->tri_count= tmp[0];

    x_cache_read (cache, &numEdges, sizeof(xuint32), 1);
    group->edges = x_array_new (sizeof (struct Edge), numEdges);
    group->edges->len = numEdges;

    for (e = 0; e < numEdges; ++e) {
        xbyte degenerate;
        struct Edge* edge;
        edge = &x_array_index (group->edges, struct Edge, e);

        x_cache_read (cache, tmp, sizeof(xuint32), 2);
        edge->tri_index[0] = tmp[0];
        edge->tri_index[1] = tmp[1];

        x_cache_read (cache, tmp, sizeof(xuint32), 2);
        edge->vert_index[0] = tmp[0];
        edge->vert_index[1] = tmp[1];

        x_cache_read (cache, tmp, sizeof(xuint32), 2);
        edge->shared_vindex[0] = tmp[0];
        edge->shared_vindex[1] = tmp[1];

        x_cache_read (cache, &degenerate, sizeof(xbyte), 1);
        edge->degenerate = degenerate;
    }
}
static void
edge_group_free         (struct EdgeGroup *group)
{
    if (group->edges) {
        x_array_unref (group->edges);
        group->edges = NULL;
    }
    if (group->vdata) {
        /* free by submesh
         * g_vertex_data_delete (group->vdata);
         */
        group->vdata = NULL;
    }
}
static gEdgeData*
read_edge_data          (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xbyte closed;
    xuint32 triangles;
    xuint32 groups;
    xuint32 tmp[3];
    xuint32 i;
    gEdgeData* edge_data;

    x_cache_read (cache, &closed, sizeof(xbyte), 1);
    x_cache_read (cache, &triangles, sizeof(xuint32), 1);
    x_cache_read (cache, &groups, sizeof(xuint32), 1);

    edge_data = x_malloc0(sizeof (gEdgeData));
    edge_data->closed = closed;
    edge_data->triangles = x_array_new (sizeof (struct Triangle), triangles);
    edge_data->normals = x_align_malloc(sizeof(gvec) * triangles, G_VEC_ALIGN);
    edge_data->faceings = x_malloc0 (sizeof (xbyte) * triangles);
    edge_data->groups = x_array_new (sizeof (struct EdgeGroup), groups);
    edge_data->groups->free_func = edge_group_free;

    for (i = 0; i < triangles; ++i) {
        struct Triangle *tri;
        greal normal[4];
        tri = x_array_append1 (edge_data->triangles, NULL);
        x_cache_read (cache, tmp, sizeof(xuint32), 1);
        tri->index_set = tmp[0];

        x_cache_read (cache, tmp, sizeof(xuint32), 1);
        tri->vertex_set = tmp[0];

        x_cache_read (cache, tmp, sizeof(xuint32), 3);
        tri->vert_index[0] = tmp[0];
        tri->vert_index[1] = tmp[1];
        tri->vert_index[2] = tmp[2];

        x_cache_read (cache, tmp, sizeof(xuint32), 3);
        tri->shared_vindex[0] = tmp[0];
        tri->shared_vindex[1] = tmp[1];
        tri->shared_vindex[2] = tmp[2];

        x_cache_read (cache, normal, sizeof(greal), 4);
        edge_data->normals[i] = g_vec_load(normal);
    }
    for (i = 0; i < groups; ++i) {
        struct EdgeGroup *group;
        xuint16 id;
        xuint32 count;

        id = read_head (cache, &count);
        if (id != M_EDGE_GROUP)
            x_error("Missing M_EDGE_GROUP stream");

        group = x_array_append1 (edge_data->groups, NULL);
        read_edge_group (cache, count, group);
        if (count != cache->count) {
            if (!_g_mesh_ogre) {
                if (count > cache->count) 
                    x_cache_skip (cache, count - cache->count); 
                else if (count < cache->count) {
                    x_error ("mesh chunk[x%x] end (%d) over bound(%d)",
                             id, cache->count, count);
                }
            }
            else
            {
                x_critical ("mesh chunk[x%x] not end (%d) at bound(%d)",
                            id, cache->count, count);
            }
        }

        if (mesh->vdata) {
            if (group->vertex_set == 0)
                group->vdata = mesh->vdata;
            else
                group->vdata = x_array_index(mesh->submesh, gSubMesh,
                                             group->vertex_set-1).vdata;
        }
        else {
            group->vdata =  x_array_index(mesh->submesh, gSubMesh,
                                          group->vertex_set).vdata;
        }
    }
    return edge_data;
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

static void
read_edge_list_lod      (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xuint16 lodIndex;
    xbyte  isManual;
    x_cache_read (cache, &lodIndex, sizeof(xuint16), 1);
    x_cache_read (cache, &isManual, sizeof(xbyte), 1);
    if (!isManual) {
        gMeshLodUsage * usage;
        if (!mesh->lod_usage) {
            mesh->lod_usage = x_array_new (sizeof(gMeshLodUsage), 4);
            mesh->lod_usage->free_func = mesh_lod_usage_free;
        }
        if (mesh->lod_usage->len < lodIndex+1u)
            x_array_resize (mesh->lod_usage, lodIndex+1);
        usage = &x_array_index (mesh->lod_usage, gMeshLodUsage, lodIndex);
        usage->edge_data = read_edge_data (cache, max_count, mesh);
    }
}
static void
read_edge_list          (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xuint16 id;
    xuint32 count;
    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, max_count);
        switch (id) {
        case M_EDGE_LIST_LOD:
            read_edge_list_lod (cache, count, mesh);
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, max_count);
    }
    mesh->edge_built = TRUE;
}
static void
read_poses              (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
}
static void
read_animations         (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
}
static void
read_mesh               (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xuint16 id;
    xuint32 count;
    xbyte skeleton;

    mesh->audo_edge = FALSE;
    x_cache_read (cache,  &skeleton, sizeof(xbyte), 1);

    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, max_count);
        switch (id) {
        case M_GEOMETRY:
            read_geometry (cache, count, mesh, &mesh->vdata);
            break;
        case M_SUBMESH:
            read_submesh (cache, count, mesh);
            break;
        case M_MESH_SKELETON_LINK:
            read_skeleton_link (cache, count, mesh);
            break;
        case M_MESH_BONE_ASSIGNMENT:
            read_mesh_bone_assign (cache, count, mesh);
            break;
        case M_MESH_LOD:
            //TODO:readMeshLodInfo(stream, pMesh);
            break;
        case M_MESH_BOUNDS:
            read_bounds (cache, count, mesh);
            break;
        case M_SUBMESH_NAME_TABLE:
            read_submesh_name_table (cache, count, mesh);
            break;
        case M_EDGE_LISTS:
            read_edge_list(cache, count, mesh);
            break;
        case M_POSES:
            read_poses (cache, count, mesh);
            break;
        case M_ANIMATIONS:
            read_animations(cache, count, mesh);
            break;
        case M_TABLE_EXTREMES:
            //readExtremes(stream, pMesh);
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, max_count);
    }
}
void
ogre_mesh_read          (gMesh          *mesh,
                         xCache         *cache)
{
    xuint16 id;
    xuint32 count;

    x_return_if_fail (cache != NULL);
    x_return_if_fail (G_IS_MESH (mesh));
    x_return_if_fail (X_RESOURCE(mesh)->state != X_RESOURCE_LOADED);

    while (!cache->eof && (id=read_head (cache, &count))){
        BEGIN_READ_CHECK (id, cache, count, X_MAXUINT32);
        switch (id) {
        case M_MESH:
            read_mesh (cache, count, mesh);
            break;
        case FLIP_M_HEADER:
            cache->flip_endian = TRUE;
            x_flip_endian(&count, sizeof(xuint32), 1);
        case M_HEADER:
            /* skip header */
            if (!_g_mesh_ogre) {
                x_cache_skip(cache,count - cache->count);
            }
            else {
                x_cache_back (cache, sizeof(xuint32));
                x_cache_gets(cache);
                count = cache->count;
            }
            break;
        default:
            DEFAULT_READ_CHECK();
        }
        END_READ_CHECK (id, cache, count, X_MAXUINT32);
    }
}
