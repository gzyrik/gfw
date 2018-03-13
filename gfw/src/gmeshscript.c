#include "config.h"
#include "gmesh-prv.h"
#include "grender.h"
#include "gmaterial.h"
#include "ganim-prv.h"
#include "gbuffer.h"
#include <string.h>
#define S_MESH                              "mesh"
#define S_SUBMESH                           "submesh"
#define S_SUBMESH_INDICES_DECLARATION       "declare"
#define S_SUBMESH_INDICES                   "indices"
#define S_GEOMETRY                          "geometry"
#define S_GEOMETRY_VERTEX_DECLARATION       "declare"
#define S_GEOMETRY_VERTEX_ELEMENT           "element"
#define S_GEOMETRY_VERTEX_BUFFER            "source"
#define S_GEOMETRY_VERTEX                   "vertex"
#define S_BONES_ASSIGNMENT                  "boneassignments"
#define S_MESH_BOUNDS                       "bounds"
static xcstr
vertex_type_cstr (gVertexType type)
{
    switch(type) {
    case G_VERTEX_FLOAT1:
        return "float1";
    case G_VERTEX_FLOAT2:
        return "float2";
    case G_VERTEX_FLOAT3:
        return "float3";
    case G_VERTEX_FLOAT4:
        return "float4";
    case G_VERTEX_A8R8G8B8:
    case G_VERTEX_SHORT1:
    case G_VERTEX_SHORT2:
    case G_VERTEX_SHORT3:
    case G_VERTEX_SHORT4:
    case G_VERTEX_UBYTE4:
    default:
        x_error_if_reached();
    }
    return NULL;
}
static gVertexType
get_vertex_type(xcstr key)
{
    if (!strcmp(key, "float1"))
        return G_VERTEX_FLOAT1;
    else if (!strcmp(key, "float2"))
        return G_VERTEX_FLOAT2;
    else if (!strcmp(key, "float3"))
        return G_VERTEX_FLOAT3;
    else if (!strcmp(key, "float4"))
        return G_VERTEX_FLOAT4;
    else
        x_error_if_reached();
    return G_VERTEX_FLOAT1;
}
static xcstr
vertex_semantic_cstr (gVertexSemantic semantic)
{
    switch (semantic) {
    case G_VERTEX_POSITION:
        return "position";
    case G_VERTEX_BWEIGHTS:
        return "bweights";
    case G_VERTEX_BINDICES:
        return "bindices";
    case G_VERTEX_NORMAL:
        return "normal";
    case G_VERTEX_DIFFUSE:
        return "diffuse";
    case G_VERTEX_SPECULAR:
        return "specular";
    case G_VERTEX_TEXCOORD:
        return "texcoord";
    case G_VERTEX_BINORMAL:
        return "binormal";
    case G_VERTEX_TANGENT:
        return "tangent";
    case G_VERTEX_PSIZE:
        return "psize";
    default:
        x_error_if_reached();
    }
    return NULL;
}
static gVertexSemantic
get_vertex_semantic (xcstr key)
{
    if (!strcmp(key, "position"))
        return G_VERTEX_POSITION;
    else if (!strcmp(key, "bweights"))
        return G_VERTEX_BWEIGHTS;
    else if (!strcmp(key, "bindices"))
        return G_VERTEX_BINDICES;
    else if (!strcmp(key, "normal"))
        return G_VERTEX_NORMAL;
    else if (!strcmp(key, "diffuse"))
        return G_VERTEX_DIFFUSE;
    else if (!strcmp(key, "specular"))
        return G_VERTEX_SPECULAR;
    else if (!strcmp(key, "texcoord"))
        return G_VERTEX_TEXCOORD;
    else if (!strcmp(key, "binormal"))
        return G_VERTEX_BINORMAL;
    else if (!strcmp(key, "tangent"))
        return G_VERTEX_TANGENT;
    else if (!strcmp(key, "psize"))
        return G_VERTEX_PSIZE;
    else
        x_error_if_reached();
    return G_VERTEX_POSITION;
}
static void
script_write_vertex(xFile *file, xcstr key, gVertexType type, xptr vertex)
{
    xfloat *f = vertex;
    switch(type) {
    case G_VERTEX_FLOAT1:
        x_script_write(file, key, "%f", f[0]);
        break;
    case G_VERTEX_FLOAT2:
        x_script_write(file, key, "[%f %f]", f[0], f[1]);
        break;
    case G_VERTEX_FLOAT3:
        x_script_write(file, key, "[%f %f %f]", f[0], f[1], f[2]);
        break;
    case G_VERTEX_FLOAT4:
        x_script_write(file, key, "[%f %f %f %f]", f[0], f[1], f[2], f[3]);
        break;
    case G_VERTEX_A8R8G8B8:
    case G_VERTEX_SHORT1:
    case G_VERTEX_SHORT2:
    case G_VERTEX_SHORT3:
    case G_VERTEX_SHORT4:
    case G_VERTEX_UBYTE4:
    default:
        x_error_if_reached();
    }
}
static void
script_read_vertex(xcstr val, gVertexType type, xptr vertex)
{
    xfloat *f = vertex;
    switch(type) {
    case G_VERTEX_FLOAT1:
        sscanf(val, "%f", f+0);
        break;
    case G_VERTEX_FLOAT2:
        sscanf(val, "[%f %f]", f+0, f+1);
        break;
    case G_VERTEX_FLOAT3:
        sscanf(val, "[%f %f %f]", f+0, f+1, f+2);
        break;
    case G_VERTEX_FLOAT4:
        sscanf(val, "[%f %f %f %f]", f+0, f+1, f+2, f+3);
        break;
    case G_VERTEX_A8R8G8B8:
    case G_VERTEX_SHORT1:
    case G_VERTEX_SHORT2:
    case G_VERTEX_SHORT3:
    case G_VERTEX_SHORT4:
    case G_VERTEX_UBYTE4:
    default:
        x_error_if_reached();
    }
}
static xcstr
primitive_cstr(gPrimitiveType primitive)
{
    switch(primitive)
    {
    case G_RENDER_POINT_LIST:
        return "points";
    case G_RENDER_LINE_LIST:
        return "lines";
    case G_RENDER_LINE_STRIP:
        return "line-strip";
    case G_RENDER_TRIANGLE_LIST:
        return "triangles";
    case G_RENDER_TRIANGLE_STRIP:
        return "triangle-strip";
    case G_RENDER_TRIANGLE_FAN:
        return "triangle-fan";
    default:
        x_error_if_reached();
    }
    return NULL;
}
static gPrimitiveType
get_primitive (xcstr key)
{
    if (!strcmp(key, "points"))
        return G_RENDER_POINT_LIST;
    else if (!strcmp(key, "lines"))
        return G_RENDER_LINE_LIST;
    else if (!strcmp(key, "line-strip"))
        return G_RENDER_LINE_STRIP;
    else if (!strcmp(key, "triangles"))
        return G_RENDER_TRIANGLE_LIST;
    else if (!strcmp(key, "triangle-strip"))
        return G_RENDER_TRIANGLE_STRIP;
    else if (!strcmp(key, "triangle-fan"))
        return G_RENDER_TRIANGLE_FAN;
    else
        x_error_if_reached();
    return G_RENDER_POINT_LIST;
}
struct script_velem
{
    gVertexElem elem;
    gVertexData *vdata;
};
static void
velem_script_put(xFile *file, gVertexElem *elem)
{
    x_script_begin (file, S_GEOMETRY_VERTEX_ELEMENT, NULL);
    x_script_write(file, "semantic", "%s", vertex_semantic_cstr(elem->semantic));
    x_script_write(file, "type", "%s", vertex_type_cstr(elem->type));
    if (elem->source != 0)
        x_script_write(file, "source", "%"X_SIZE_FORMAT, elem->source);
    if (elem->index != 0)
        x_script_write(file, "index", "%"X_SIZE_FORMAT, elem->index);
    x_script_end (file);
}
static xptr
velem_on_enter (gVertexData *vdata, xcstr name, xcstr group, xQuark parent_name)
{
    struct script_velem * velem;
    velem = x_new0(struct script_velem, 1);
    velem->vdata = vdata;
    return velem;
}
static void
velem_on_visit (struct script_velem * velem, xcstr key, xcstr val)
{
    if (!strcmp(key, "semantic"))
        velem->elem.semantic = get_vertex_semantic(val);
    else if (!strcmp(key, "type"))
        velem->elem.type = get_vertex_type(val);
    else if (!strcmp(key, "index"))
        velem->elem.index = atoi(val);
    else if (!strcmp(key, "source"))
        velem->elem.source = atoi(val);
}
static void
velem_on_exit(struct script_velem * velem, xptr parent, xcstr group)
{
    g_vertex_data_add (velem->vdata, &velem->elem);
    x_free(velem);
}
static void
vdecl_script_put(xFile *file, xArray *elements)
{
    xsize i;
    x_script_begin (file, S_GEOMETRY_VERTEX_DECLARATION, NULL);
    for (i=0; i< elements->len; ++i) {
        gVertexElem *elem = &x_array_index (elements, gVertexElem, i);
        velem_script_put (file, elem);
    }
    x_script_end (file);
}
static void
vdecl_on_exit(gVertexData *vdata, xptr parent, xcstr group)
{
    g_vertex_data_alloc(vdata);
}
struct script_vertex
{
    xArray  *elements;
    gBuffer *vbuf;
    xsize   source;
    xsize   index[G_VERTEX_PSIZE+1];
    xbyte   *base;
    xsize   vsize;
};
static void
vertex_script_put(xFile *file, gVertexData *vdata, xsize source, xbyte  *base)
{
    xsize j;
    x_script_begin(file, S_GEOMETRY_VERTEX, NULL);
    for (j=0; j< vdata->elements->len; ++j) {
        gVertexElem *elem = &x_array_index (vdata->elements, gVertexElem, j);
        if (elem->source != source)
            continue;
        script_write_vertex(file, vertex_semantic_cstr(elem->semantic), elem->type, base+elem->offset);
    }
    x_script_end (file);
}
static void
vertex_on_visit(struct script_vertex* vertex, xcstr key, xcstr val)
{
    xsize j;
    gVertexSemantic semantic;
    semantic = get_vertex_semantic(key);
    for (j=0; j< vertex->elements->len; ++j) {
        gVertexElem *elem = &x_array_index (vertex->elements, gVertexElem, j);
        if (elem->source != vertex->source || elem->semantic != semantic)
            continue;
        if (vertex->index[semantic] != elem->index)
            continue;
        script_read_vertex(val, elem->type, vertex->base+elem->offset);
        vertex->index[semantic]++;
        break;
    }
}
static void
vertex_on_exit(struct script_vertex* vertex, xptr parent, xcstr group)
{
    memset(vertex->index, 0, sizeof(vertex->index));
    vertex->base += vertex->vsize;
}
static void
vbuf_script_put(xFile *file, gVertexData *vdata, xsize source, gVertexBuffer *vbuf)
{
    xsize i;
    xbyte  *base = g_buffer_lock ((gBuffer*)vbuf);
    if (source > 0)
        x_script_begin (file, S_GEOMETRY_VERTEX_BUFFER, "%"X_SIZE_FORMAT, source);
    else
        x_script_begin (file, S_GEOMETRY_VERTEX_BUFFER, NULL);

    for (i=0; i<vdata->count; ++i, base += vbuf->vertex_size)
        vertex_script_put (file, vdata, source, base);

    g_buffer_unlock ((gBuffer*)vbuf);
    x_script_end (file);
}
static xptr
vbuf_on_enter (gVertexData *vdata, xcstr name, xcstr group, xQuark parent_name)
{
    struct script_vertex* vertex;
    vertex = x_new0(struct script_vertex, 1);
    vertex->elements = vdata->elements;
    if (name && name[0])
        vertex->source = atoi(name);
    vertex->vbuf = g_vertex_data_get(vdata,vertex->source);
    vertex->base = g_buffer_lock(vertex->vbuf);
    vertex->vsize = G_VERTEX_BUFFER(vertex->vbuf)->vertex_size;
    return vertex;
}
static void
vbuf_on_exit(struct script_vertex* vertex, xptr parent, xcstr group)
{
    g_buffer_unlock(vertex->vbuf);
    x_free(vertex);
}
static void
geom_script_put (xFile *file, gVertexData *vdata)
{
    xsize i;
    x_script_begin(file, S_GEOMETRY, NULL);
    x_script_write(file, "count", "%"X_SIZE_FORMAT, vdata->count);
    vdecl_script_put (file, vdata->elements);
    for (i=0; i<vdata->binding->len;++i) {
        gVertexBuffer *vbuf = x_ptr_array_index(vdata->binding, gVertexBuffer, i);
        vbuf_script_put(file, vdata, i, vbuf);
    }
    x_script_end(file);
}
static xptr
geom_on_enter (xptr parent, xcstr name, xcstr group, xQuark parent_name)
{
    if (!strcmp(x_quark_str(parent_name), S_MESH)){
        struct MeshPrivate *priv;
        priv = MESH_PRIVATE(parent);
        if(!priv->vdata)
            priv->vdata = x_new0 (gVertexData, 1);
        return priv->vdata;
    }
    else {
        struct SubMesh *submesh;
        submesh = (struct SubMesh*)parent;
        if(!submesh->vdata)
            submesh->vdata = x_new0 (gVertexData, 1);
        return submesh->vdata;
    }
}
static void
geom_on_visit (gVertexData *vdata, xcstr key, xcstr val)
{
    if (strcmp(key, "count") == 0)
        vdata->count = atoi(val);
}
static void
bassign_script_put(xFile *file, xArray *bones)
{
    xsize i, j;
    gBoneAssign *assign;

    x_script_begin(file, S_BONES_ASSIGNMENT, NULL);
    for (i=0; i<bones->len;++i) {
        assign = &x_array_index (bones, gBoneAssign, i);
        for (j=0;j<assign->count;++j) {
            x_script_write(file, "assign", "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT" %f]", i, assign->bindex[j], assign->weight[j]);
        }
    }
    x_script_end(file);
}
static xptr
bassign_on_enter (xptr parent, xcstr name, xcstr group, xQuark parent_name)
{
    if (!strcmp(x_quark_str(parent_name), S_MESH)) {
        struct MeshPrivate *priv;
        priv = MESH_PRIVATE(parent);
        priv->bone_assign_dirty = TRUE;
        if (!priv->bone_assign)
            priv->bone_assign = x_array_new_full (sizeof(gBoneAssign), sizeof(gBoneAssign)*priv->vdata->count, NULL);
        return priv->bone_assign;
    }
    else {
        struct SubMesh * submesh = (struct SubMesh*)parent;
        submesh->bone_assign_dirty = TRUE;
        if (!submesh->bone_assign)
            submesh->bone_assign = x_array_new_full (sizeof(gBoneAssign), sizeof(gBoneAssign)*submesh->vdata->count, NULL);
        return submesh->bone_assign;
    }
}
static void
bassign_on_visit (xArray *bones, xcstr key, xcstr val)
{
    if (strcmp(key, "assign") == 0) {
        xuint32 vindex;
        xuint16 bindex;
        xfloat32 weight;
        gBoneAssign *assign;
        xsize j,k;

        sscanf(val, "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT" %f]", &vindex, &bindex, &weight);
        if (ABS(weight, 0) < 0.00001f)
            return;
        if (bones->len <= vindex)
            x_array_resize (bones, vindex+1);
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
}
struct script_idecl
{
    struct SubMesh    *submesh;
    xsize       index_count;
    xsize       index_size;
};
static xptr
idecl_on_enter (struct SubMesh *submesh, xcstr name, xcstr group, xQuark parent_name)
{
    struct script_idecl* idecl;
    idecl = x_new0(struct script_idecl, 1);
    idecl->submesh = submesh;
    return idecl;
}
static void
idecl_on_visit (struct script_idecl* idecl, xcstr key, xcstr val)
{
    if (!strcmp(key, "count"))
        idecl->index_count = atoi (val);
    else if (!strcmp(key, "isize"))
        idecl->index_size = atoi (val);
}
static void
idecl_on_exit(struct script_idecl* idecl, xptr parent, xcstr group)
{
    gBuffer *ibuf = g_index_buffer_new(idecl->index_size, idecl->index_count, NULL);
    idecl->submesh->idata = g_index_data_new (idecl->index_count);
    g_index_data_bind (idecl->submesh->idata, G_INDEX_BUFFER (ibuf));
    x_object_unref (ibuf);

    x_free(idecl);
}
static void
idecl_script_put(xFile *file, struct SubMesh *submesh)
{
    x_script_begin (file, S_SUBMESH_INDICES_DECLARATION, NULL);
    x_script_write(file, "count", "%"X_SIZE_FORMAT, submesh->idata->count);
    x_script_write(file, "isize", "%"X_SIZE_FORMAT, submesh->idata->buffer->index_size);
    x_script_end(file);
}
struct script_indices
{
    xsize           faces;
    xuint32         *pInt32;
    xuint16         *pInt16;
    struct SubMesh        *submesh;
};
static xptr
indices_on_enter (struct SubMesh *submesh, xcstr name, xcstr group, xQuark parent_name)
{
    struct script_indices* indi;
    const xsize index_size = submesh->idata->buffer->index_size;

    indi = x_new0(struct script_indices, 1);

    indi->submesh = submesh;
    if (index_size == sizeof(xuint32))
        indi->pInt32 = g_buffer_lock (G_BUFFER(submesh->idata->buffer));
    else
        indi->pInt16 = g_buffer_lock (G_BUFFER(submesh->idata->buffer));

    return indi;
}
static void
indices_on_visit (struct script_indices* indi, xcstr key, xcstr val)
{
    if (!strcmp(key, "face")) {
        xint index[3];
        xsize count, i;

        if (indi->faces == 0) {
            switch(indi->submesh->primitive) {
            case G_RENDER_POINT_LIST:
                index[0] = atoi(val);
                count = 1;
                break;
            case G_RENDER_LINE_STRIP:
            case G_RENDER_LINE_LIST:
                sscanf(val, "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index+0, index+1);
                count = 2;
                break;
            case G_RENDER_TRIANGLE_STRIP:
            case G_RENDER_TRIANGLE_FAN:
            case G_RENDER_TRIANGLE_LIST:
                sscanf(val, "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index+0, index+1, index+2);
                count = 3;
                break;
            default:
                x_error_if_reached ();
            }
        }
        else {
            switch(indi->submesh->primitive){
            case G_RENDER_TRIANGLE_STRIP:
            case G_RENDER_TRIANGLE_FAN:
            case G_RENDER_LINE_STRIP:
            case G_RENDER_POINT_LIST:
                index[0] = atoi(val);
                count = 1;
                break;
            case G_RENDER_LINE_LIST:
                sscanf(val, "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index+0, index+1);
                count = 2;
                break;
            case G_RENDER_TRIANGLE_LIST:
                sscanf(val, "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index+0, index+1, index+2);
                count = 3;
                break;
            default:
                x_error_if_reached ();
            }
        }
        for(i=0;i<count;++i) {
            if (indi->pInt32)
                *(indi->pInt32)++ = index[i];
            else
                *(indi->pInt16)++  = index[i];
        }
        indi->faces++;
    }
}
static void
indices_on_exit(struct script_indices* indi, xptr parent, xcstr group)
{
    g_buffer_unlock (G_BUFFER(indi->submesh->idata->buffer));
    x_free(indi);
}
static void
indices_script_put(xFile *file, struct SubMesh *submesh)
{
    xint index[3];
    gBuffer *ibuf = G_BUFFER(submesh->idata->buffer);
    const xsize index_count = submesh->idata->count;
    const xsize index_size = submesh->idata->buffer->index_size;
    xuint32 *pInt32 = NULL;
    xuint16 *pInt16 = NULL;
    xsize faces,i;

    x_script_begin (file, S_SUBMESH_INDICES, NULL);

    if (index_size == sizeof(xuint32))
        pInt32 = g_buffer_lock (ibuf);
    else
        pInt16 = g_buffer_lock (ibuf);

    switch(submesh->primitive) {
    case G_RENDER_POINT_LIST:
        faces = index_count;
        break;
    case G_RENDER_LINE_LIST:
        faces = index_count/2;
        break;
    case G_RENDER_LINE_STRIP:
        if (pInt32) {
            index[0] = *pInt32++;
            index[1] = *pInt32++;
        }
        else {
            index[0] = *pInt16++;
            index[1] = *pInt16++;
        }
        x_script_write(file, "face", "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index[0], index[1]);
        faces = index_count - 2;
        break;
    case G_RENDER_TRIANGLE_LIST:
        faces = index_count/3;
        break;
    case G_RENDER_TRIANGLE_STRIP:
    case G_RENDER_TRIANGLE_FAN:
        if (pInt32) {
            index[0] = *pInt32++;
            index[1] = *pInt32++;
            index[2] = *pInt32++;
        }
        else {
            index[0] = *pInt16++;
            index[1] = *pInt16++;
            index[2] = *pInt16++;
        }
        x_script_write(file, "face", "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index[0], index[1], index[2]);
        faces = index_count - 3;
        break;
    default:
        x_error_if_reached ();
    }

    for(i=0;i<faces;++i) {
        switch(submesh->primitive ) {
        case G_RENDER_TRIANGLE_STRIP:
        case G_RENDER_TRIANGLE_FAN:
        case G_RENDER_POINT_LIST:
        case G_RENDER_LINE_STRIP:
            x_script_write(file, "face", "%"X_SIZE_FORMAT, pInt32 ? *pInt32++ : *pInt16++);
            break;
        case G_RENDER_LINE_LIST:
            if (pInt32) {
                index[0] = *pInt32++;
                index[1] = *pInt32++;
            }
            else {
                index[0] = *pInt16++;
                index[1] = *pInt16++;
            }
            x_script_write(file, "face", "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index[0], index[1]);
            break;
        case G_RENDER_TRIANGLE_LIST:
            if (pInt32) {
                index[0] = *pInt32++;
                index[1] = *pInt32++;
                index[2] = *pInt32++;
            }
            else {
                index[0] = *pInt16++;
                index[1] = *pInt16++;
                index[2] = *pInt16++;
            }
            x_script_write(file, "face", "[%"X_SIZE_FORMAT" %"X_SIZE_FORMAT" %"X_SIZE_FORMAT"]", index[0], index[1], index[2]);
            break;
        default:
            x_error_if_reached ();
        }
    }
    g_buffer_unlock (ibuf);
    x_script_end (file);
}
static xptr
submesh_on_enter (gMesh *mesh, xcstr name, xcstr group, xQuark parent_name)
{
    struct MeshPrivate *priv;
    struct SubMesh *submesh;

    priv = MESH_PRIVATE(mesh);
    submesh = x_array_append1 (priv->submesh, NULL);
    if (name)
        submesh->name = x_strdup(name);
    return submesh;
}
static void
submesh_on_visit (struct SubMesh *submesh, xcstr key, xcstr val)
{
    if (!strcmp(key, "material"))
        submesh->material = x_strdup (val);
    else if(!strcmp(key, "shared_vdata"))
        submesh->shared_vdata = TRUE;
    else if(!strcmp(key, "primitive"))
        submesh->primitive = get_primitive(val);
}
static void
submesh_script_put(xFile *file, struct SubMesh *submesh)
{
    x_script_begin (file, S_SUBMESH, NULL);
    x_script_write (file, "primitive", "%s", primitive_cstr(submesh->primitive));

    if (submesh->material)
        x_script_write(file, "material", "%s", submesh->material);
    if (submesh->shared_vdata)
        x_script_write(file, "shared_vdata", "true");

    idecl_script_put (file, submesh);
    indices_script_put (file, submesh);

    if (submesh->vdata)
        geom_script_put (file, submesh->vdata);

    if (submesh->bone_assign)
        bassign_script_put (file, submesh->bone_assign);

    x_script_end (file);
}
static void
bounds_script_put (xFile *file, gMesh *mesh)
{
    xfloat32 min[4],max[4], radius[4];
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);

    x_script_begin(file, "bounds", NULL);
    g_vec_store (priv->aabb.minimum, min);
    g_vec_store (priv->aabb.maximum, max);
    g_vec_store (priv->radius_sq, radius);
    x_script_write(file, "minimum", "[%f %f %f]",min[0], min[1], min[2]);
    x_script_write(file, "maximum", "[%f %f %f]",max[0], max[1], max[2]);
    x_script_write(file, "radius", "%f", sqrtf(radius[0]));
    x_script_end(file);
}
static void
bounds_on_visit (gMesh *mesh, xcstr key, xcstr val)
{
    float f[3];
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);

    if(strcmp(key, "minimum")==0) {
        sscanf(val, "[%f %f %f]", &f[0], &f[1], &f[2]);
        priv->aabb.minimum = g_vec(f[0],f[1],f[2], 1);
    }
    else if(strcmp(key, "maximum")==0) {
        sscanf(val, "[%f %f %f]", &f[0], &f[1], &f[2]);
        priv->aabb.maximum = g_vec(f[0],f[1],f[2], 1);
    }
    priv->aabb.extent = 1;
}
void
_g_mesh_script_put (gMesh *mesh, xFile *file)
{
    xsize i;
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);

    x_script_begin (file, S_MESH, "%s", priv->name);

    if (priv->skeleton)
        x_script_write(file, "skeleton", "%s", priv->skeleton);
    if (priv->vdata && priv->vdata->count > 0)
        geom_script_put (file, priv->vdata);

    for (i=0; i<priv->submesh->len;++i) {
        struct SubMesh *submesh;
        submesh = &x_array_index (priv->submesh, struct SubMesh, i);
        if (submesh->idata)
            submesh_script_put (file, submesh);
    }

    if (priv->bone_assign)
        bassign_script_put (file, priv->bone_assign);

    bounds_script_put (file, mesh);

    x_script_end (file);
}
static xptr
mesh_on_enter (gMesh *mesh, xcstr name, xcstr group, xQuark parent_name)
{
    if (!mesh) 
        return x_object_new (G_TYPE_MESH, "name", name, "group", group, NULL);
    return mesh;
}

void
_g_mesh_script_register (void)
{
    xScript *mesh, *bassign, *geom, *submesh;

    x_set_script_priority ("meshx", 200);

    mesh = x_script_set (NULL, x_quark_from_static(S_MESH),
                         mesh_on_enter, x_object_set_str, NULL); 
    x_script_set (mesh, x_quark_from_static(S_MESH_BOUNDS),
                  NULL, bounds_on_visit, NULL);
    bassign = x_script_set (mesh, x_quark_from_static(S_BONES_ASSIGNMENT),
                            bassign_on_enter, bassign_on_visit, NULL);
    geom = x_script_set (mesh, x_quark_from_static(S_GEOMETRY),
                         geom_on_enter, geom_on_visit, NULL); {
        xScript *vdecl, *vbuf;
        vdecl= x_script_set (geom, x_quark_from_static(S_GEOMETRY_VERTEX_DECLARATION),
                             NULL, NULL,vdecl_on_exit); {
            x_script_set (vdecl, x_quark_from_static(S_GEOMETRY_VERTEX_ELEMENT),
                          velem_on_enter, velem_on_visit, velem_on_exit);
        }

        vbuf = x_script_set (geom, x_quark_from_static(S_GEOMETRY_VERTEX_BUFFER),
                             vbuf_on_enter,NULL,vbuf_on_exit); {
            x_script_set (vbuf, x_quark_from_static(S_GEOMETRY_VERTEX),
                          NULL, vertex_on_visit,vertex_on_exit);
        }

    }
    submesh = x_script_set (mesh, x_quark_from_static(S_SUBMESH),
                            submesh_on_enter, submesh_on_visit, NULL); {
        x_script_set (submesh, x_quark_from_static(S_SUBMESH_INDICES_DECLARATION),
                      idecl_on_enter, idecl_on_visit, idecl_on_exit);
        x_script_set (submesh, x_quark_from_static(S_SUBMESH_INDICES),
                      indices_on_enter, indices_on_visit, indices_on_exit);
        x_script_link (submesh, x_quark_from_static(S_GEOMETRY), geom);
        x_script_link (submesh, x_quark_from_static(S_BONES_ASSIGNMENT), bassign);
    }
}
