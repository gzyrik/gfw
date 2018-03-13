#include "config.h"
#include "gmeshfile.h"
#include "gmesh.h"
#include "gmesh-prv.h"
#include "grender.h"
#include "gmaterial.h"
#include "ganim-prv.h"
#include "gbuffer.h"
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
xbool _g_mesh_ogre = 0;
#define FLIP_M_HEADER       0x0010
#define HEAD_COUNT          6
static xuint16
read_head               (xCache         *cache,
                         xuint32        *count)
{
    xuint16 id;

    if (1 != x_cache_read (cache, &id, sizeof(xuint16), 1))
        return 0;
    if (1 != x_cache_read (cache, count, sizeof(xuint32), 1))
        return 0;
    *count += cache->count - HEAD_COUNT;
    return id;
}
static xuint32
write_head              (xFile          *file,
                         xuint16        id,
                         xuint32        count)
{
    x_file_puti16 (file, id);
    x_file_puti32 (file, count);
    return HEAD_COUNT;
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
    x_cache_read (cache, &source, sizeof(xint16), 1);
    x_cache_read (cache, &type, sizeof(xint16), 1);
    x_cache_read (cache, &semantic, sizeof(xint16), 1);
    x_cache_read (cache, &offset, sizeof(xint16), 1);
    x_cache_read (cache, &index, sizeof(xint16), 1);

    g_vertex_data_add (vdata, source, offset, type, semantic, index);
}
static xuint32
calc_geometry_velem      ()
{
    xuint32 count = HEAD_COUNT;
    return count + sizeof(xint16) * 5;
}
static xuint32
write_geometry_velem     (xFile         *file,
                          xuint32       max_count,
                          gVertexElem   *elem)
{
    const xuint32 count = calc_geometry_velem();
    BEGIN_WRITE_CHECK(file, M_GEOMETRY_VERTEX_ELEMENT, count, max_count);

    max_count -= write_head (file, M_GEOMETRY_VERTEX_ELEMENT, count);
    max_count -= x_file_puti16 (file, elem->source);
    max_count -=  x_file_puti16 (file, elem->type);
    max_count -=  x_file_puti16 (file, elem->semantic);
    max_count -=  x_file_puti16 (file, elem->offset);
    max_count -=  x_file_puti16 (file, elem->index);

    END_WRITE_CHECK(file, M_GEOMETRY_VERTEX_DECLARATION, count, max_count);
    return count;
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
static xuint32
calc_geometry_vdecl     (xArray         *elements)
{
    xuint32 count = HEAD_COUNT;
    return count +  elements->len * calc_geometry_velem();
}
static xuint32
write_geometry_vdecl    (xFile          *file,
                         xuint32        max_count,
                         xArray         *elements)
{
    xsize i;
    const xuint32 count = calc_geometry_vdecl (elements);

    BEGIN_WRITE_CHECK(file, M_GEOMETRY_VERTEX_DECLARATION, count, max_count);
    max_count -= write_head (file, M_GEOMETRY_VERTEX_DECLARATION, count);
    for (i=0;i<elements->len;++i) {
        gVertexElem *elem = &x_array_index (elements, gVertexElem, i);
        max_count -= write_geometry_velem (file, max_count, elem);
    }
    END_WRITE_CHECK(file, M_GEOMETRY_VERTEX_DECLARATION, count, max_count);
    return count;
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
static xuint32
calc_geometry_vbuf     (gVertexData    *vdata,
                        xuint16        bindIndex,
                        gVertexBuffer  *vbuf)
{
    xsize count = HEAD_COUNT;
    /* bind_index, vertex_size */
    count += sizeof(xuint16) + sizeof(xuint16);
    /* M_GEOMETRY_VERTEX_BUFFER_DATA */
    count += HEAD_COUNT;
    count += vbuf->vertex_size * vdata->count;
    return count;
}
static xuint32
write_geometry_vbuf     (xFile          *file,
                         xuint32        max_count,
                         gVertexData    *vdata,
                         xuint16        bindIndex,
                         gBuffer        *vbuf)
{
    const xsize v_count = vdata->count;
    const xsize v_size = G_VERTEX_BUFFER(vbuf)->vertex_size;
    const xsize v_bytes = v_count*v_size;
    const xuint32 buf_count=HEAD_COUNT*2 + sizeof(xuint16)*2 + v_bytes;
    const xuint32 dat_count=HEAD_COUNT + v_bytes;
    xptr p;

    max_count -= write_head (file, M_GEOMETRY_VERTEX_BUFFER, buf_count);
    max_count -= x_file_puti16 (file, bindIndex);
    max_count -= x_file_puti16 (file, v_size);

    max_count -= write_head (file, M_GEOMETRY_VERTEX_BUFFER_DATA, dat_count);
    p = g_buffer_lock (G_BUFFER(vbuf));
    if (file->flip_endian) {
        xbyte *tmp = x_malloc (v_bytes);
        memcpy (tmp, p, v_bytes);
        flip_vbuf (tmp, bindIndex, v_size, vdata->count, vdata->elements);
        max_count -= x_file_put (file, tmp, 1, v_bytes);
        x_free (tmp);
    }
    else {
        max_count -= x_file_put (file, p, 1, v_bytes);
    }
    g_buffer_unlock (vbuf);
    return buf_count;
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
static xuint32
calc_geometry           (gVertexData    *vdata)
{
    xsize i;
    xuint32 count = HEAD_COUNT; /* M_GEOMETRY */
    /* num vertices */
    count += sizeof(xuint32);
    count += calc_geometry_vdecl (vdata->elements); 
    for (i=0; i<vdata->binding->len;++i) {
        gVertexBuffer *vbuf = x_ptr_array_index(vdata->binding, gVertexBuffer, i);
        /* M_GEOMETRY_VERTEX_BUFFER */
        count += calc_geometry_vbuf (vdata,i, vbuf); 
    }
    return count;
}
static xuint32
write_geometry          (xFile          *file,
                         xuint32        max_count,
                         gVertexData    *vdata)
{
    xuint32 i;
    const xuint32 count = calc_geometry (vdata);

    BEGIN_WRITE_CHECK(file, M_GEOMETRY, count, max_count);
    /* header */
    max_count -= write_head (file, M_GEOMETRY, count);
    /* vertex count */
    max_count -= x_file_puti32 (file, vdata->count);
    max_count -= write_geometry_vdecl (file, max_count, vdata->elements);

    for (i=0; i<vdata->binding->len;++i) {
        gBuffer *vbuf = x_ptr_array_index(vdata->binding, gBuffer, i);
        max_count -= write_geometry_vbuf (file, max_count, vdata, i, vbuf);
    }

    END_WRITE_CHECK(file, M_GEOMETRY, count, max_count);

    return count;
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
static xuint32
calc_submesh_primitive  (gSubMesh       *submesh)
{
    return HEAD_COUNT + sizeof(xuint16);
}
static xuint32
write_submesh_primitive (xFile          *file,
                         xuint32        max_count,
                         gSubMesh       *submesh)
{
    const xuint32 count = calc_submesh_primitive (submesh);
    BEGIN_WRITE_CHECK (file, M_SUBMESH_PRIMITIVE, count, max_count);

    max_count -= write_head (file, M_SUBMESH_PRIMITIVE, count);
    max_count -= x_file_puti16 (file, submesh->primitive);

    END_WRITE_CHECK (file, M_SUBMESH_PRIMITIVE, count, max_count);
    return count;
}
static void
calc_texture_alias      (xptr           key,
                         xptr           value,
                         xuint32        *count)
{
    *count += HEAD_COUNT;
    *count += strlen(key) + 1 + strlen(value) + 1;
}
static void
write_texture_alias     (xptr           key,
                         xptr           value,
                         xFile          *file)
{
    xsize count = HEAD_COUNT;
    count += strlen(key) + 1 + strlen(value) + 1;
    write_head (file, M_SUBMESH_TEXTURE_ALIAS, count);
    x_file_puts (file, key, TRUE);
    x_file_puts (file, value, TRUE);
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
static xuint32
calc_bone_assign        (struct BoneAssign *assign)
{
    xuint32 count = HEAD_COUNT;
    /* vert index */
    count += sizeof (xuint32);
    /* bone index */
    count += sizeof (xuint16);
    /* weight */
    count += sizeof (greal);
    return count;
}
static xuint32
write_submesh_bone_assign (xFile        *file,
                           xuint32      max_count,
                           xuint16      vindex,
                           struct BoneAssign *assign)
{
    const xuint32 count = calc_bone_assign (assign);
    BEGIN_WRITE_CHECK (file, M_SUBMESH_BONE_ASSIGNMENT, count, max_count);
    max_count -= write_head (file, M_SUBMESH_BONE_ASSIGNMENT, count);

    max_count -= x_file_puti32 (file, vindex);
    max_count -= x_file_puti16 (file, assign->bindex);
    max_count -= x_file_putf32 (file, assign->weight);

    END_WRITE_CHECK (file, M_SUBMESH_BONE_ASSIGNMENT, count, max_count);
    return count;
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
static xuint32
calc_submesh            (gSubMesh       *submesh)
{
    xsize i;
    xuint32 count = HEAD_COUNT;
    xbool idx_32bit = (submesh->idata->buffer->index_size == sizeof(xuint32));

    /* material name */
    count += strlen(submesh->material)+1;
    /* use shared_vdata */
    count += sizeof(xbyte);
    /* idata->count */
    count += sizeof(xuint32);
    /* idx_32bit */
    count += sizeof(xbyte);
    /* idata */
    if (submesh->idata->buffer && submesh->idata->count > 0)
        count += submesh->idata->buffer->index_size * submesh->idata->count;
    /* geometry */
    if (!submesh->shared_vdata)
        count += calc_geometry (submesh->vdata);

    if (submesh->tex_aliases && x_hash_table_size (submesh->tex_aliases) > 0)
        x_hash_table_foreach (submesh->tex_aliases, calc_texture_alias, &count);

    count += calc_submesh_primitive (submesh);

    if (submesh->bone_assign && submesh->bone_assign->len > 0) {
        xsize j;
        xArray *blends;
        for (i=0;i<submesh->bone_assign->len;++i) {
            struct BoneAssign *assign;
            blends = x_ptr_array_index (submesh->bone_assign, xArray, i);
            if (blends) {
                for (j=0;j<blends->len; ++j) {
                    assign = &x_array_index (blends, struct BoneAssign, j);
                    count += calc_bone_assign (assign);
                }
            }
        }
    }
    return count;
}
static xuint32 
write_submesh           (xFile          *file,
                         xuint32        max_count,
                         gSubMesh       *submesh)
{
    const xuint32 count = calc_submesh (submesh);
    xbool idx_32bit = FALSE;

    BEGIN_WRITE_CHECK (file, M_SUBMESH, count, max_count);
    max_count -= write_head (file, M_SUBMESH, count);
    /* material name */
    max_count -= x_file_puts (file, submesh->material, TRUE);
    /* use shared_vdata */
    max_count -= x_file_putc (file, submesh->shared_vdata);
    /* idata->count */
    max_count -= x_file_puti32 (file, submesh->idata->count);
    /* idx_32bit */
    if (submesh->idata->buffer)
        idx_32bit = submesh->idata->buffer->index_size==sizeof(xuint32);
    max_count -= x_file_putc (file, idx_32bit);

    if (submesh->idata->count >0 && submesh->idata->buffer) {
        /* idata */
        xptr p = g_buffer_lock (G_BUFFER(submesh->idata->buffer));
        if (idx_32bit)
            max_count -= x_file_put (file, p, sizeof(xuint32), submesh->idata->count);
        else
            max_count -= x_file_put (file, p, sizeof(xuint16), submesh->idata->count);
        g_buffer_unlock (G_BUFFER(submesh->idata->buffer));
    }
    if (!submesh->shared_vdata) {
        /* geometry */
        max_count -= write_geometry (file, max_count, submesh->vdata);
    }
    if (submesh->tex_aliases)
        x_hash_table_foreach (submesh->tex_aliases, write_texture_alias, file);

    max_count -= write_submesh_primitive (file, max_count, submesh);

    if (submesh->bone_assign && submesh->bone_assign->len > 0) {
        xsize i, j;
        xArray *blends;
        for (i=0;i<submesh->bone_assign->len;++i) {
            struct BoneAssign *assign;
            blends = x_ptr_array_index (submesh->bone_assign, xArray, i);
            if (blends) {
                for (j=0;j<blends->len; ++j) {
                    assign = &x_array_index (blends, struct BoneAssign, j);
                    max_count += write_submesh_bone_assign (file, max_count, i, assign);
                }
            }
        }
    }

    END_WRITE_CHECK (file, M_SUBMESH, count, max_count);
    return count;
}

static void
read_skeleton_link      (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    x_free (mesh->skeleton);
    mesh->skeleton = x_strdup (x_cache_gets(cache));
}
static xuint32
calc_skeleton_link      (gMesh          *mesh)
{
    xuint32 count = HEAD_COUNT;
    count += strlen (mesh->skeleton) + 1;
    return count;
}
static xuint32
write_skeleton_link     (xFile          *file,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xuint32 count = calc_skeleton_link (mesh);

    BEGIN_WRITE_CHECK (file, M_MESH_SKELETON_LINK, count, max_count);

    max_count -= write_head (file, M_MESH_SKELETON_LINK, count);
    max_count -= x_file_puts (file, mesh->skeleton, TRUE);

    END_WRITE_CHECK (file, M_MESH_SKELETON_LINK, count, max_count);
    return count;
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

static xuint32
write_mesh_bone_assign  (xFile          *file,
                         xuint32        max_count,
                         xuint16        vindex,
                         struct BoneAssign *assign)
{
    const xuint32 count = calc_bone_assign (assign);
    BEGIN_WRITE_CHECK (file, M_MESH_BONE_ASSIGNMENT, count, max_count);
    max_count -= write_head (file, M_MESH_BONE_ASSIGNMENT, count);

    max_count -= x_file_puti32 (file, vindex);
    max_count -= x_file_puti16 (file, assign->bindex);
    max_count -= x_file_putf32  (file, assign->weight);

    END_WRITE_CHECK (file, M_MESH_BONE_ASSIGNMENT, count, max_count);
    return count;
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
static xuint32
calc_bounds             (gMesh          *mesh)
{
    xuint32 count = HEAD_COUNT;
    count += sizeof(greal)*3 + sizeof(greal)*3 + sizeof(greal);
    return count;
}
static xuint32
write_bounds            (xFile          *file,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    greal min[4],max[4], radius[4];
    const xuint32 count = calc_bounds (mesh);

    BEGIN_WRITE_CHECK (file, M_MESH_BOUNDS, count, max_count);
    max_count -= write_head (file, M_MESH_BOUNDS, count);
    g_vec_store (mesh->aabb.minimum, min);
    g_vec_store (mesh->aabb.maximum, max);
    g_vec_store (mesh->radius_sq, radius);
    max_count -= x_file_put (file, min, sizeof(greal), 3);
    max_count -= x_file_put (file, max, sizeof(greal), 3);
    radius[0] = sqrtf(radius[0]);
    max_count -= x_file_put (file, radius, sizeof(greal), 1);
    END_WRITE_CHECK (file, M_MESH_BOUNDS, count, max_count);
    return count;
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
static xuint32
write_submesh_name_table(xFile          *file,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    return 0;
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
static xuint32
calc_edge_group         (struct EdgeGroup *group)
{
    xuint32 count = HEAD_COUNT;
    /* vertex_set */
    count += sizeof(xuint32);
    /* tri_start */
    count += sizeof(xuint32);
    /* tri_count */
    count += sizeof(xuint32);
    /* edges number */
    count += sizeof(xuint32);
    /* tri_index[2], vert_index[2], shared_vindex[2], degenerate */
    count += (sizeof(xuint32)*6+sizeof(xbyte))*group->edges->len;
    return count;
}
static xuint32
write_edge_group        (xFile          *file,
                         xuint32        max_count,
                         struct EdgeGroup *group)
{
    xsize i;
    const xuint32 count = calc_edge_group (group);
    BEGIN_WRITE_CHECK(file, M_EDGE_GROUP, count, max_count);

    max_count -= write_head (file, M_EDGE_GROUP, count);
    max_count -= x_file_puti32 (file, group->vertex_set);
    max_count -= x_file_puti32 (file, group->tri_start);
    max_count -= x_file_puti32 (file, group->tri_count);
    max_count -= x_file_puti32 (file, group->edges->len);
    for (i=0; i<group->edges->len;++i) {
        struct Edge *edge;
        edge = &x_array_index (group->edges, struct Edge, i);
        max_count -= x_file_puti32 (file, edge->tri_index[0]);
        max_count -= x_file_puti32 (file, edge->tri_index[1]);
        max_count -= x_file_puti32 (file, edge->vert_index[0]);
        max_count -= x_file_puti32 (file, edge->vert_index[1]);
        max_count -= x_file_puti32 (file, edge->shared_vindex[0]);
        max_count -= x_file_puti32 (file, edge->shared_vindex[1]);
        max_count -= x_file_putc (file, edge->degenerate);
    }

    END_WRITE_CHECK(file, M_EDGE_GROUP, count, max_count);
    return count;
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
static xuint32
calc_edge_list_lod      (gEdgeData      *edge_data,
                         xbool          is_manual)
{
    xsize i;
    xuint32 count = HEAD_COUNT;
    /* lodindex */
    count += sizeof(xuint16);
    count += sizeof(xbyte);
    if (!is_manual) {
        /* closed */
        count += sizeof(xbyte);
        /* triangles */
        count += sizeof(xuint32);
        /* groups */
        count += sizeof(xuint32);
        /* index_set,vertex_set,ver_index[3]+shared_vindex[3]+normal[4] */
        count += (sizeof(xuint32)*8+sizeof(greal)*4)*edge_data->triangles->len;

        for (i=0;i<edge_data->groups->len;++i) {
            struct EdgeGroup *group;
            group = &x_array_index(edge_data->groups, struct EdgeGroup, i);
            count += calc_edge_group (group);
        }
    }
    return count;
}
static xuint32
write_edge_list_lod     (xFile          *file,
                         xuint32        max_count,
                         xuint16        lod_index,
                         gEdgeData      *edge_data,
                         xbool          is_manual)
{
    const xuint32 count = calc_edge_list_lod(edge_data, is_manual);
    BEGIN_WRITE_CHECK (file, M_EDGE_LIST_LOD, count, max_count);

    max_count -= write_head(file, M_EDGE_LIST_LOD, count);
    max_count -= x_file_puti16 (file, lod_index);
    max_count -= x_file_putc (file, is_manual);
    if (!is_manual) {
        xsize i,j;
        max_count -= x_file_putc (file, edge_data->closed);
        max_count -= x_file_puti32 (file, edge_data->triangles->len);
        max_count -= x_file_puti32 (file, edge_data->groups->len);
        for (i=0;i<edge_data->triangles->len;++i) {
            struct Triangle *tri;
            greal normal[4];
            tri = &x_array_index (edge_data->triangles, struct Triangle, i);
            max_count -= x_file_puti32 (file, tri->index_set);
            max_count -= x_file_puti32 (file, tri->vertex_set);
            for (j=0;j<3;++j)
                max_count -= x_file_puti32 (file, tri->vert_index[i]);
            for (j=0;j<3;++j)
                max_count -= x_file_puti32 (file, tri->shared_vindex[i]);
            g_vec_store (edge_data->normals[i], normal);
            max_count -= x_file_put (file, normal, sizeof(greal), 4);
        }
        for (i=0;i<edge_data->groups->len;++i) {
            struct EdgeGroup *group;
            group = &x_array_index (edge_data->groups, struct EdgeGroup, i);
            max_count -= write_edge_group (file, max_count, group);
        }
    }
    END_WRITE_CHECK (file, M_EDGE_LIST_LOD, count, max_count);
    return count;
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

static xuint32
calc_edge_list          (gMesh          *mesh)
{
    xsize i;
    xuint32 count = HEAD_COUNT;
    for (i=0;i<mesh->lod_usage->len;++i) {
        gEdgeData *edge_data;
        xbool is_manual;
        edge_data = x_array_index (mesh->lod_usage, gMeshLodUsage, i).edge_data;
        is_manual = mesh->lod_manual && (i > 0);

        count += calc_edge_list_lod (edge_data, is_manual);
    }
    return count;
}
static xuint32
write_edge_list         (xFile          *file,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    xsize i;
    const xuint32 count = calc_edge_list (mesh);

    BEGIN_WRITE_CHECK (file, M_EDGE_LISTS, count, max_count);

    max_count -= write_head (file, M_EDGE_LISTS, count);
    for (i=0;i<mesh->lod_usage->len;++i) {
        gEdgeData *edge_data;
        xbool is_manual;

        edge_data = x_array_index (mesh->lod_usage, gMeshLodUsage, i).edge_data;
        is_manual = mesh->lod_manual && (i > 0);
        max_count -= write_edge_list_lod (file, max_count, i, edge_data, is_manual);
    }

    END_WRITE_CHECK (file, M_EDGE_LISTS, count, max_count);
    return count;
}

static void
read_poses              (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
}
static xuint32
calc_poses              (gMesh          *mesh)
{
    return 0;
}
static xuint32
write_poses             (xFile          *file,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    return 0;
}
static void
read_animations         (xCache         *cache,
                         xuint32        max_count,
                         gMesh          *mesh)
{
}
static xuint32
write_animations        (xFile          *file,
                         xuint32        max_count,
                         gMesh          *mesh)
{
    return 0;
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
g_mesh_read             (gMesh          *mesh,
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

static void
calc_name_index         (xptr           key,
                         xptr           value,
                         xptr           user_data)
{
    xuint32 *count = user_data;
    *count += HEAD_COUNT + sizeof(xuint16)+strlen(key) + 1;
}
static xuint32
calc_submesh_name_table (gMesh          *mesh)
{
    xuint32 count = HEAD_COUNT;
    x_hash_table_foreach (mesh->name_index, calc_name_index, &count);
    return count;
}

static xuint32
calc_morph_frame_size   (struct MorphFrame *kf,
                         xsize          vcount)
{
    xuint32 count = HEAD_COUNT;
    count += sizeof(greal); /* time */
    count += sizeof(greal)*3*vcount; /* x,y,z*/
    return count;
}
static xuint32
calc_animaton_track     (struct VertexTrack *track)
{
    xuint32 count = HEAD_COUNT;

    count += sizeof (xuint16); /* anim type */
    count += sizeof (xuint16); /* vdata index. 0 for shared_vdata */

    if (track->morph) {
        xsize i;
        for (i=0; i<track->frames.keyframes->len;++i) {
            struct MorphFrame *kf;
            kf = &x_array_index (track->frames.keyframes, struct MorphFrame, i);
            count += calc_morph_frame_size (kf, track->vdata->count);
        }
    }
    else{

    }
    return count;
}
static void
calc_animation          (xQuark         name,
                         gAnimation     *anim,
                         xuint32        *count)
{
    xsize i;
    *count = HEAD_COUNT;
    *count += strlen(x_quark_to(name))+1;
    /* anim length */
    count += sizeof (greal);
    for (i=0;i<anim->vertex_track->len; ++i) {
        struct VertexTrack *track;
        track = &x_array_index (anim->vertex_track, struct VertexTrack,i);
        *count += calc_animaton_track(track);
    }
}
static xuint32
calc_animations         (gMesh          *mesh)
{
    xuint32 count = HEAD_COUNT;
    x_hash_table_foreach (mesh->animations, (xHFunc)calc_animation, &count);
    return count;
}

static xuint32
calc_comment            (xcstr          comment)
{
    xuint32 count = HEAD_COUNT;
    xsize slen = 1;
    if (comment)
        slen += strlen (comment);
    count += slen;
    return count;
}
static xuint32
write_comment           (xFile          *file,
                         xcstr          comment,
                         xuint32        max_count)
{
    const xuint32 count = calc_comment (comment);

    BEGIN_WRITE_CHECK (file, M_HEADER, count, max_count);

    max_count -= write_head (file, M_HEADER, count);
    max_count -= x_file_puts (file, comment, TRUE);

    END_WRITE_CHECK (file, M_HEADER, count, max_count);
    return count;
}
static xuint32
calc_mesh               (gMesh          *mesh)
{
    xsize i;
    xuint32 count = HEAD_COUNT;
    count += 1; /* skeleton flag */

    if (mesh->vdata && mesh->vdata->count > 0)
        count += calc_geometry (mesh->vdata);

    if (mesh->submesh && mesh->submesh->len > 0) {
        for (i=0; i<mesh->submesh->len; ++i)
            count += calc_submesh (&x_array_index (mesh->submesh, gSubMesh, i));
    }
    if (mesh->skeleton) {
        count += calc_skeleton_link (mesh);
        if (mesh->bone_assign && mesh->bone_assign->len >0) {
            struct BoneAssign *assign;
            for (i=0;i<mesh->bone_assign->len;++i) {
                assign = &x_array_index (mesh->bone_assign, struct BoneAssign, i);
                count +=  calc_bone_assign (assign);
            }
        }
    }

    if (mesh->lod_usage && mesh->lod_usage->len > 0) 
        ;

    count += calc_bounds (mesh);

    if (mesh->name_index && x_hash_table_size (mesh->name_index) > 0)
        count += calc_submesh_name_table (mesh);

    if (mesh->edge_built)
        count += calc_edge_list (mesh);

    if (mesh->poses && mesh->poses->len)
        count += calc_poses (mesh);

    if (mesh->animations)
        count += calc_animations (mesh);

    return count;
}
static xuint32
write_mesh              (xFile          *file,
                         gMesh          *mesh,
                         xuint32        max_count)
{
    xsize i;
    const xuint32 count = calc_mesh (mesh);

    BEGIN_WRITE_CHECK (file, M_MESH, count, max_count);
    max_count -= write_head (file, M_MESH, count);

    max_count -= x_file_putc (file, mesh->skeleton !=NULL);
    if (mesh->vdata && mesh->vdata->count > 0)
        max_count -= write_geometry (file, max_count, mesh->vdata);

    if (mesh->submesh && mesh->submesh->len > 0) {
        for (i=0; i<mesh->submesh->len;++i) {
            gSubMesh *submesh =  &x_array_index (mesh->submesh, gSubMesh, i);
            max_count -= write_submesh (file, max_count,submesh);
        }
    }

    if (mesh->skeleton) {
        max_count -= write_skeleton_link (file, max_count, mesh);
        if (mesh->bone_assign && mesh->bone_assign->len >0) {
            xsize i, j;
            xArray *blends;
            for (i=0;i<mesh->bone_assign->len;++i) {
                struct BoneAssign *assign;
                blends = x_ptr_array_index (mesh->bone_assign, xArray, i);
                if (blends) {
                    for (j=0;j<blends->len; ++j) {
                        assign = &x_array_index (blends, struct BoneAssign, j);
                        max_count += write_mesh_bone_assign (file, max_count, i, assign);
                    }
                }
            }
        }
    }

    if (mesh->lod_usage && mesh->lod_usage->len > 0) {
    }

    max_count -= write_bounds (file, max_count, mesh);

    if (mesh->name_index && x_hash_table_size (mesh->name_index) > 0) {
        max_count -= write_submesh_name_table (file, max_count, mesh);
    }
    if (mesh->edge_built) {
        max_count -= write_edge_list (file, max_count, mesh);
    }
    if (mesh->poses && mesh->poses->len) {
        max_count -= write_poses (file, max_count, mesh);
    }
    if (mesh->animations && x_hash_table_size (mesh->animations) > 0) {
        max_count -= write_animations (file, max_count, mesh);
    }

    END_WRITE_CHECK (file, M_MESH, count, max_count);
    return count;
}
void
g_mesh_write            (gMesh          *mesh,
                         xFile          *file,
                         xcstr          comment)
{
    x_return_if_fail (X_IS_FILE (file));
    x_return_if_fail (G_IS_MESH (mesh));
    x_return_if_fail (X_RESOURCE(mesh)->state == X_RESOURCE_LOADED);

    write_comment (file, comment, X_MAXUINT32);
    write_mesh (file, mesh, X_MAXUINT32);
}
