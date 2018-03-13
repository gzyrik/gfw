#include "config.h"
#include "gmesh-prv.h"
#include "grender.h"
#include "gbuffer.h"
#include "ganimation.h"
#include "gskeleton.h"
#include "gfw-prv.h"
#include <string.h>
#include <stdlib.h>

static xHashTable       *_meshes;
X_DEFINE_TYPE(gMesh, g_mesh, X_TYPE_RESOURCE);

/* mesh property */
enum {
    PROP_0,
    PROP_NAME,
    PROP_MATERIAL,
    PROP_SKELETON,
    N_PROPERTIES
};
static void
set_property            (xObject            *mesh,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    struct MeshPrivate *priv;
    priv = MESH_PRIVATE(mesh);

    switch(property_id) {
    case PROP_NAME:
        if (priv->name) {
            x_hash_table_remove (_meshes, priv->name);
            x_free (priv->name);
        }
        priv->name = x_value_dup_str (value);
        if (priv->name)
            x_hash_table_insert (_meshes, priv->name, mesh);
        break;
    case PROP_MATERIAL:
        x_free (priv->material);
        priv->material = x_value_dup_str (value);
        break;
    case PROP_SKELETON:
        x_free (priv->skeleton);
        priv->skeleton = x_value_dup_str (value);
        break;
    }
}

static void
get_property            (xObject            *mesh,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    struct MeshPrivate *priv;
    priv = MESH_PRIVATE(mesh);

    switch(property_id) {
    case PROP_NAME:
        x_value_set_str (value, priv->name);
        break;
    case PROP_MATERIAL:
        x_value_set_str (value, priv->material);
        break;
    case PROP_SKELETON:
        x_value_set_str (value, priv->skeleton);
        break;
    }
}
static void
mesh_prepare            (xResource      *mesh,
                         xbool          backThread)
{
    struct MeshPrivate *priv;
    priv = MESH_PRIVATE(mesh);

    if (priv->build_param) 
        _g_build_manual_mesh (G_MESH(mesh), priv->build_param);
    else if (priv->name && x_str_suffix(priv->name,".mesh")&&priv->submesh->len == 0) {
        xFile *file;
        file = x_repository_open (mesh->group, priv->name, NULL);
        if (file)
            x_chunk_import_file (file, mesh->group, mesh);
        else
            x_warning("can't open mesh file `%s'", priv->name);
    }
    if (priv->skeleton) {
        gSkeleton *skeleton;
        skeleton = g_skeleten_new (priv->skeleton, mesh->group, NULL);
        x_resource_prepare (skeleton, backThread);
    }
}

static void
mesh_load               (xResource      *resource,
                         xbool          backThread)
{
    gMesh *mesh = (gMesh*) resource;
}
static void
submesh_free            (struct SubMesh       *submesh)
{
    g_vertex_data_delete (submesh->vdata);
    g_index_data_delete (submesh->idata);
    x_free (submesh->material);
    x_array_unref (submesh->bone_assign);
    x_hash_table_unref (submesh->tex_aliases);
}

static void
g_mesh_init             (gMesh          *mesh)
{
    struct MeshPrivate *priv;
    priv = x_instance_private(mesh, G_TYPE_MESH);
    mesh->priv = priv;

    priv->submesh = x_array_new_full (sizeof (struct SubMesh), 0, (xFreeFunc)submesh_free);
}
static void
mesh_finalize           (xObject        *mesh)
{
    struct MeshPrivate *priv;
    priv = MESH_PRIVATE(mesh);

    x_hash_table_remove (_meshes, priv->name);
    x_free (priv->name);
    x_free (priv->skeleton);
    x_free (priv->material);
    g_vertex_data_delete (priv->vdata);
    x_array_unref (priv->bone_assign); 
    x_array_unref (priv->submesh);
    x_array_unref (priv->lod_usage); 
    x_align_free (priv->build_param);

    x_hash_table_unref (priv->animations);
}
static void
g_mesh_class_init       (gMeshClass     *klass)
{
    xParam         *param;
    xObjectClass   *oclass;
    xResourceClass *rclass;

    x_class_set_private(klass, sizeof(struct MeshPrivate));
    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = mesh_finalize;

    rclass = X_RESOURCE_CLASS (klass);
    rclass->prepare = mesh_prepare;
    rclass->load = mesh_load;

    if (!_meshes)
        _meshes = x_hash_table_new (x_str_hash, x_str_cmp);

    param = x_param_str_new ("name",
                             "Name",
                             "The name of the mesh object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_NAME, param);

    param = x_param_str_new ("material",
                             "Material",
                             "The material name of the mesh object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_MATERIAL, param);

    param = x_param_str_new ("skeleton",
                             "Skeleton",
                             "The skeleton name of the mesh object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_SKELETON, param);
}


static gMesh* mesh_get (xcstr name)
{
    x_return_val_if_fail (name != NULL, NULL);
    if (!_meshes)
        return NULL;

    return x_hash_table_lookup (_meshes, name);
}
gMesh*
g_mesh_attach           (xcstr          name,
                         xcptr          stub)
{
    gMesh* mesh;

    x_return_val_if_fail (name != NULL, NULL);

    mesh = x_object_ref (mesh_get (name));
    if (!mesh) return NULL;

    return mesh;
}
void
g_mesh_detach           (gMesh          *mesh,
                         xcptr          stub)
{
    if (!mesh) return;
    x_return_if_fail (G_IS_MESH (mesh));

    //x_hash_table_remove (mesh->clients, (xptr)stub);
    x_object_unref (mesh);
}
gMesh*
g_mesh_new              (xcstr          file,
                         xcstr          group,
                         xcstr          first_property,
                         ...)
{
    va_list argv;
    gMesh* mesh;

    va_start(argv, first_property);
    mesh = mesh_get (file);

    if (mesh)
        x_object_set_valist(mesh, first_property, argv);
    else
        mesh = x_object_new_valist1 (G_TYPE_MESH,
                                     first_property, argv,
                                     "name", file,
                                     "group", group,
                                     NULL);
    va_end(argv);
    return mesh;
}

gMesh*
g_mesh_build            (gMeshBuildParam*param,
                         xcstr          first_property,
                         ...)
{
    gMesh *mesh;
    va_list argv;
    struct MeshPrivate *priv;

    va_start(argv, first_property);
    mesh = x_object_new_valist (G_TYPE_MESH, first_property, argv);
    va_end(argv);

    x_return_val_if_fail(mesh != NULL, NULL);
    priv = MESH_PRIVATE(mesh);

    priv->build_param = x_align_malloc(sizeof (gMeshBuildParam), G_VEC_ALIGN);
    memcpy (priv->build_param, param, sizeof (gMeshBuildParam));

    return mesh;
}
static int
bone_index_cmp          (const xuint16  *b1,
                         const xuint16  *b2)
{
    return *b1 - *b2;
}
static void
compile_bone_assign     (gMesh          *mesh,
                         xArray         *bone_assign,
                         gVertexData    *vdata,
                         xArray         *blend2bone)
{
    xsize i,j;
    xbool non_skinned_vertex;
    xsize max_blend_weights;
    gBoneAssign *assign;
    greal sum;
    struct MeshPrivate *priv;

    x_return_if_fail(mesh != NULL);
    x_return_if_fail(bone_assign->len == vdata->count);

    priv = MESH_PRIVATE(mesh);

    non_skinned_vertex = FALSE;
    max_blend_weights = 0;
    blend2bone->len = 0;
    for (i=0; i<vdata->count;++i) {
        assign = &x_array_index (bone_assign, gBoneAssign, i);
        if (assign->count == 0) {
            non_skinned_vertex =TRUE;
            continue;
        }
        if (assign->count > max_blend_weights)
            max_blend_weights = assign->count;

        sum = 0;
        for (j=0;j<assign->count;++j) {
            sum += assign->weight[j];
            x_array_insert_sorted (blend2bone,
                                   &assign->bindex[i], &assign->bindex[i],
                                   (xCmpFunc)bone_index_cmp, FALSE);
        }
        if (!g_real_aeq (sum ,1)) {
            for (j=0; j<assign->count; ++j)
                assign->weight[i] /= sum;
        }
    }
    if (non_skinned_vertex) {
        x_warning ("The mesh '%s' includes vertices without bone assignments. "
                   "Those vertices will transform to wrong position when skeletal "
                   "animation enabled. To eliminate this, assign at least one bone "
                   "assignment per vertex on your mesh.",
                   priv->name);
    }
    if (max_blend_weights > G_MAX_N_WEIGHTS) {
        x_warning ("The mesh '%s' includes vertices with more than %d bone assignments."
                   "The lowest weighted assignments beyond this limit have been removed, so "
                   "your animation may look slightly different. To eliminate this, reduce "
                   "the number of bone assignments per vertex on your mesh to %d",
                   priv->name, G_MAX_N_WEIGHTS, G_MAX_N_WEIGHTS);
        max_blend_weights = G_MAX_N_WEIGHTS;
    }
   
    if (blend2bone->len > 0) {
        xsize source = -1;
        gBuffer *vbuf;
        xbyte *indices;
        xfloat *weight;
        xsize vertex_size;
        gVertexElem velem;

        /* remove old blend and weight source */
        for (i=0;i<vdata->elements->len;++i) {
            gVertexElem *elem;
            elem = &x_array_index (vdata->elements, gVertexElem, i);
            if (elem->semantic == G_VERTEX_BINDICES
                || elem->semantic == G_VERTEX_BWEIGHTS) {
                source = elem->source;
            }
        }

        if (source != -1)
            g_vertex_data_remove (vdata, source);
        else
            source = vdata->binding->len;

        memset(&velem, 0, sizeof(velem));
        velem.source = source;
        velem.semantic = G_VERTEX_BINDICES;
        velem.type = G_VERTEX_UBYTE4;
        g_vertex_data_add (vdata, &velem);

        memset(&velem, 0, sizeof(velem));
        velem.source = source;
        velem.semantic = G_VERTEX_BWEIGHTS;
        velem.type = G_VERTEX_FLOAT1+max_blend_weights-1;
        g_vertex_data_add (vdata, &velem);

        vertex_size = 4+sizeof(float)*max_blend_weights;
        vbuf = g_vertex_buffer_new (vertex_size,vdata->count, NULL);
        g_vertex_data_bind (vdata, source, G_VERTEX_BUFFER (vbuf));
        x_object_unref (vbuf);

        indices = g_buffer_lock (vbuf);
        for (i=0;i<vdata->count;++i) {
            assign = &x_array_index (bone_assign, gBoneAssign, i);
            if (assign->count == 0)  continue;
            weight = (xfloat*)(indices + 4);

            for (j=0;j<max_blend_weights;++j) {
                if (j<assign->count) {
                    indices[j] = x_array_find_sorted (blend2bone, &assign->bindex[j], (xCmpFunc)bone_index_cmp);
                    weight[j] = assign->weight[j];
                }
                else {
                    indices[j] = 0;
                    weight[j] = 0;
                }
            }
            indices += vertex_size;
        }
        g_buffer_unlock (vbuf);
    }
}
xArray*
_g_mesh_bone_blend_map  (gMesh          *mesh,
                         xsize          sub_index)
{
    struct SubMesh *submesh;
    struct MeshPrivate *priv;

    x_return_val_if_fail(mesh != NULL, NULL);
    priv = MESH_PRIVATE(mesh);

    submesh = &x_array_index(priv->submesh, struct SubMesh, sub_index);

    if (submesh->shared_vdata) {
        if (priv->bone_assign_dirty) {
            if (!priv->blend_map)
                priv->blend_map = x_array_new (sizeof(xuint16));
            compile_bone_assign (mesh, priv->bone_assign, priv->vdata, priv->blend_map);
            priv->bone_assign_dirty = FALSE;
        }
        return priv->blend_map;
    }
    else {
        if (submesh->bone_assign_dirty) {
            if (!submesh->blend_map)
                submesh->blend_map = x_array_new (sizeof(xuint16));
            compile_bone_assign (mesh, submesh->bone_assign, submesh->vdata, submesh->blend_map);
            submesh->bone_assign_dirty = FALSE;
        }
        return submesh->blend_map;
    }
}
static void
add_state (xptr name, gAnimation *anim, gStateSet *states)
{
    g_states_insert (states, (xQuark)name, anim->length);
}
void
g_mesh_states           (gMesh          *mesh,
                         gStateSet      *states)
{
    struct MeshPrivate *priv;

    x_return_if_fail(mesh != NULL);
    priv = MESH_PRIVATE(mesh);

    x_hash_table_foreach (priv->animations, add_state, states);
}
void
g_mesh_write            (gMesh          *mesh,
                         xFile          *file,
                         xbool          binary)
{
    x_return_if_fail(mesh != NULL);
    x_return_if_fail(file != NULL);

    if (binary)
        _g_mesh_chunk_put (mesh, file);
    else
        _g_mesh_script_put (mesh, file);
}
void
g_mesh_foreach          (gMesh          *mesh,
                         xCallback      callback,
                         xptr           user_data)
{
    struct MeshPrivate *priv;

    x_return_if_fail(mesh != NULL);
    priv = MESH_PRIVATE(mesh);

    x_array_foreach(priv->submesh, callback, user_data);
}