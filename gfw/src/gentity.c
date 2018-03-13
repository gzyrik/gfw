#include "config.h"
#include "gentity.h"
#include "gfw-prv.h"
#include "gmaterial.h"
#include "gmesh-prv.h"
#include "grenderable.h"
#include "gnode.h"
#include "grender.h"
#include "grenderqueue.h"
#include "gskeleton.h"
#include "ganimation.h"

static xQuark _quark_cached_map = 0;

enum {
    BIND_SOFTWARE_SKELETAL,
    BIND_HARDWARE_MORPH,
    BIND_ORIGINAL,
    BIND_SOFTWARE_MORPH
};
struct cached_wmatrix
{
    gmat4       *bone_wmat4;
    xsize       n_bones;
    xuint       node_stamp;
    xuint       states_stamp;
};
static void
cached_wmatrix_free     (struct cached_wmatrix *cached)
{
    x_align_free (cached->bone_wmat4);
    x_free (cached);
}
/* 是否含有骨骼动画 */
static xbool
entity_skeleton_animated(gEntity        *entity)
{
    return entity->skeleton !=0 && entity->animation != 0;
}
static xint
entity_vdata_binding    (gEntity        *entity,
                         gAnimationType anim_type)
{
    if (entity->skeleton !=0) {
        if (!entity->hardware_animated)
            return BIND_SOFTWARE_SKELETAL;
        else if  (anim_type == G_ANIMATION_MORPH)
            return BIND_HARDWARE_MORPH;
        else
            return BIND_ORIGINAL;
    }
    else if (anim_type == G_ANIMATION_MORPH) {
        if (entity->hardware_animated)
            return BIND_HARDWARE_MORPH;
        else
            return BIND_SOFTWARE_MORPH;
    }
    else
        return BIND_ORIGINAL;
}
static gTechnique*
entity_technique        (gRenderable    *renderable,
                         xptr           user_data)
{
    gEntity *entity;
    gSubEntity *subentity;

    entity = (gEntity*)renderable;
    subentity = &x_array_index(entity->subentity, gSubEntity,
                               X_PTR_TO_SIZE (user_data));
    if (subentity->material)
        return g_material_technique(subentity->material);

    return g_material_technique(entity->material);
}
static xsize
entity_model_matrix     (gRenderable    *renderable,
                         gmat4          *mat,
                         xptr           user_data)
{
    xsize i;
    gEntity *entity;
    xArray *blend_map;

    entity = (gEntity*)renderable;
    blend_map = _g_mesh_bone_blend_map (entity->mesh, X_PTR_TO_SIZE (user_data));

    /* 没有骨骼动画或者使用软件混合骨骼动画 */
    if (entity->skeleton == NULL || !entity->hardware_animated) {
        *mat = g_mat4_eye();
        return 1;
    }
    if (entity->states_stamp > 0) {
        /* 当前存在动画中的骨骼,使用缓冲中的最终变换矩阵 */
        gmat4 *bone_mat4 = g_skeleton_mat4 (entity->skeleton, entity);
        for (i=0;i<blend_map->len;++i) {
            xsize ref = x_array_index (blend_map, xuint16, i);
            *mat++ = bone_mat4[ref];
        }
    }
    else {
        /* 所有骨骼处于静止中,所有相对变换矩阵为单位阵,
         * 故最终变换矩阵都为当前SceneNode的世界变换矩阵 */
        gmat4 xform = g_mat4_eye();
        for (i=0;i<blend_map->len;++i)
            *mat++ = xform;
    }
    return blend_map->len;
}

static void
entity_detached         (gMovable       *movable,
                         gNode          *node)
{
    xHashTable *cached_map;

    cached_map = x_object_get_qdata (node, _quark_cached_map);
    if (cached_map)
        x_hash_table_remove (cached_map, movable);
}
static xsize
entity_world_matrix     (gRenderable    *renderable,
                         gmat4          *mat,
                         gNode          *node,
                         xptr           user_data)
{
    xsize i;
    const gmat4 *wmat;
    gEntity *entity;
    xArray *blend_map;

    entity = (gEntity*)renderable;
    blend_map = _g_mesh_bone_blend_map (entity->mesh, X_PTR_TO_SIZE (user_data));

    wmat = g_node_mat4 (node);
    if (entity->skeleton == 0 || !entity->hardware_animated) {
        *mat = *wmat;
        return 1;
    }
    if (entity->states_stamp > 0) {
        xHashTable *cached_map;
        struct cached_wmatrix *cached;

        cached_map = x_object_get_qdata (node, _quark_cached_map);
        if (!cached_map) {
            cached_map = x_hash_table_new_full (x_direct_hash, x_direct_cmp,
                                                NULL, (xFreeFunc)cached_wmatrix_free);
            x_object_set_qdata_full (node, _quark_cached_map,
                                     cached_map, (xFreeFunc)x_hash_table_unref);
        }
        cached = x_hash_table_lookup (cached_map, entity);
        if (!cached) {
            cached = x_new0 (struct cached_wmatrix, 1);
            x_hash_table_insert (cached_map, entity, cached);
        }
        if (entity->states_stamp != cached->states_stamp
            || cached->node_stamp != node->stamp) {

            gmat4 *bone_mat4 = g_skeleton_mat4 (entity->skeleton, entity);
            xsize n_bones = g_skeleton_bones(entity->skeleton)->len;
            if (cached->n_bones != n_bones) {
                x_align_free (cached->bone_wmat4);
                cached->bone_wmat4 =
                    x_align_malloc (sizeof(gmat4)*n_bones, G_VEC_ALIGN);
                cached->n_bones = n_bones;
            }

            for (i=0;i<cached->n_bones; ++i) {
                cached->bone_wmat4[i] = g_mat4_mul (bone_mat4+i, wmat);
            }
            cached->states_stamp = entity->states_stamp;
            cached->node_stamp = node->stamp;
        }
        for (i=0;i<blend_map->len;++i) {
            xsize ref = x_array_index (blend_map, xuint16, i);
            *mat++ = cached->bone_wmat4[ref];
        }
    }
    else {
        for (i=0;i<blend_map->len;++i)
            *mat++ = *wmat;
    }
    return blend_map->len;
}
static xbool
entity_proj_matrix      (gRenderable    *renderable,
                         gmat4          *mat,
                         xptr           user_data)
{
    gEntity *entity = (gEntity*)renderable;
    return FALSE;
}
static xbool
entity_view_matrix      (gRenderable    *renderable,
                         gmat4          *mat,
                         xptr           user_data)
{
    gEntity *entity = (gEntity*)renderable;
    return FALSE;
}
static xbool
entity_cast_shadow      (gRenderable    *renderable,
                         xptr           user_data)
{
    gEntity *entity = (gEntity*)renderable;
    return FALSE;
}
static void
entity_render_op        (gRenderable    *renderable,
                         gRenderOp      *op,
                         xptr           user_data)
{
    xint c;
    gEntity *entity;
    struct SubMesh *submesh;
    gSubEntity *subentity;
    struct MeshPrivate *mesh_priv;

    entity = (gEntity*)renderable;
    mesh_priv = MESH_PRIVATE(entity->mesh);

    subentity = &x_array_index(entity->subentity,
                               gSubEntity,
                               X_PTR_TO_SIZE (user_data));
    submesh = &x_array_index (mesh_priv->submesh,
                              struct SubMesh,
                              X_PTR_TO_SIZE (user_data));

    op->primitive = submesh->primitive;
    op->idata = submesh->idata;
    op->renderable = renderable;
    if (submesh->shared_vdata) {
        c = entity_vdata_binding (entity, mesh_priv->vdata_anim_type);
        switch (c) {
        case BIND_HARDWARE_MORPH:
            op->vdata = entity->hardware_vdata;
            break;
        case BIND_SOFTWARE_MORPH:
            op->vdata = entity->software_vdata;
            break;
        case BIND_SOFTWARE_SKELETAL:
            op->vdata = entity->skeleton_vdata;
            break;
        case BIND_ORIGINAL:
            op->vdata = mesh_priv->vdata;
            break;
        }
    }
    else {
        c = entity_vdata_binding (entity, submesh->vdata_anim_type);
        switch (c) {
        case BIND_HARDWARE_MORPH:
            op->vdata = subentity->hardware_vdata;
            break;
        case BIND_SOFTWARE_MORPH:
            op->vdata = subentity->software_vdata;
            break;
        case BIND_SOFTWARE_SKELETAL:
            op->vdata = subentity->skeleton_vdata;
            break;
        case BIND_ORIGINAL:
            op->vdata = submesh->vdata;
            break;
        }
    }
}
static void
renderable_init         (gRenderableIFace *iface)
{
    iface->technique  = entity_technique;
    iface->model_matrix = entity_model_matrix;
    iface->world_matrix = entity_world_matrix;
    iface->view_matrix = entity_view_matrix;
    iface->proj_matrix = entity_proj_matrix;
    iface->cast_shadow = entity_cast_shadow;
    iface->render_op   = entity_render_op;
}
X_DEFINE_TYPE_EXTENDED (gEntity, g_entity, G_TYPE_MOVABLE,0,
                        X_IMPLEMENT_IFACE (G_TYPE_RENDERABLE, renderable_init));


static void
update_animation        (gEntity        *entity)
{
    gMesh *mesh = entity->mesh;
    if (entity->skeleton && entity->states_stamp != entity->states->stamp) {
        entity->states_stamp = entity->states->stamp;
        g_skeleton_pose (entity->skeleton, entity, entity->states);
    }
}
static void
entity_enqueue          (gMovable       *movable,
                         gMovableContext*context)
{
    xsize i;
    gEntity *entity;
    struct MeshPrivate *mesh_priv;

    entity = (gEntity*)movable;
    if (!entity->mesh) return;
    mesh_priv = MESH_PRIVATE(entity->mesh);

    if (entity->subentity) {
        gSubEntity *subentity;
        for(i=0;i<entity->subentity->len; ++i) {
            subentity = &x_array_index (entity->subentity, gSubEntity, i);
            if (subentity->visiable)
                g_render_queue_add_data (context->queue, G_RENDERABLE(entity),
                                         context->node, X_SIZE_TO_PTR(i));
        }
    }
    if (entity->skeleton || mesh_priv->animations) {
        update_animation (entity);
    }
}
static void on_submesh(struct SubMesh * submesh, gEntity *entity)
{
    gSubEntity * subentity;

    subentity = x_array_append1 (entity->subentity, NULL);
    subentity->visiable = TRUE;
    if (submesh->tex_aliases)
        subentity->material = g_material_new (submesh->material, submesh->tex_aliases);
    else if (submesh->material)
        subentity->material = g_material_attach (submesh->material, NULL);

}
static void
build_subentity_list    (gEntity        *entity)
{
    x_array_resize (entity->subentity, 0);
    g_mesh_foreach (entity->mesh, on_submesh, NULL);
}
static void
entity_finalize         (xObject        *object)
{
    gEntity *entity = (gEntity*)object;
    x_object_unref (entity->mesh);
    g_material_detach(entity->material, entity);
    entity->mesh = NULL;
    entity->material = NULL;
}
static void
prepare_blend_bufs      (gEntity        *entity)
{
    gMesh *mesh = entity->mesh;
    /*
       g_vertex_data_delete (entity->skeleton);
       if (entity->skeleton && !entity->hardware_animated) {
       if (mesh->shared_vdata) {
    // 创建临时顶点混合缓冲用于软件混合,但没有复制数据
    }
    }
    for (i=0;i<entity->subentity->len;++i) {

    }*/
}
static void
init_states             (gEntity        *entity)
{
    struct MeshPrivate *mesh_priv;

    mesh_priv = MESH_PRIVATE(entity->mesh);
    entity->states = g_states_new();
    if (mesh_priv->skeleton) {
        gSkeleton *skeleton =  g_skeleton_get (mesh_priv->skeleton);
        if (skeleton) {
            entity->skeleton = x_object_ref (skeleton);
            g_skeleton_attach (skeleton, entity);
            g_skeleton_states (skeleton, entity->states);
        }
    }
    /* 收集顶点动画 */
    if (mesh_priv->animations)
        g_mesh_states (entity->mesh, entity->states);

    /* 预先准备动画过程中使用的临时缓冲区 */
    prepare_blend_bufs (entity);
}

static void
entity_init (gEntity *entity, xcstr mesh_name)
{
    struct MeshPrivate *mesh_priv;

    entity->mesh = g_mesh_attach (mesh_name, entity);
    x_return_if_fail(entity->mesh != NULL);


    mesh_priv = MESH_PRIVATE(entity->mesh);
    x_resource_load (entity->mesh, FALSE);
    g_movable_resize ((gMovable*)entity, &mesh_priv->aabb);

    if (mesh_priv->tex_aliases) {
        g_material_detach (entity->material, NULL);
        entity->material = g_material_new (mesh_priv->material, mesh_priv->tex_aliases);
    }
    else if (mesh_priv->material) {
        g_material_detach (entity->material, NULL);
        entity->material = g_material_attach(mesh_priv->material, NULL);
    }

    /* 构建 submesh 对应的 subentity */
    build_subentity_list (entity);
    /* 提取所有动画 */
    init_states (entity);
}

enum {
    PROP_0,
    PROP_MATERIAL,
    PROP_MESH,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gEntity *entity = (gEntity*)object;

    switch(property_id) {
    case PROP_MATERIAL:
        g_material_detach (entity->material, NULL);
        entity->material = g_material_attach (x_value_get_str(value), NULL);
        break;
    case PROP_MESH:
        if (entity->mesh)
            entity_finalize (object);
        entity_init (entity, x_value_get_str(value));
        break;
    }
}
static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gEntity *entity = (gEntity*)object;
    switch(property_id) {
    case PROP_MATERIAL:
        if (entity->material)
            x_object_get_value(entity->material, "name", value);
        break;
    case PROP_MESH:
        if (entity->mesh)
            x_object_get_value(entity->mesh, "name", value);
        break;
    }
}
static void
g_entity_init           (gEntity        *entity)
{
    entity->hardware_animated = TRUE;
    entity->subentity = x_array_new (sizeof(gSubEntity));
}
static void
g_entity_class_init     (gEntityClass   *klass)
{
    xParam         *param;
    xObjectClass   *oclass;
    gMovableClass *mclass;

    if (!_quark_cached_map)
        _quark_cached_map = x_quark_from_static ("-g-entity-cached-map");

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = entity_finalize;

    mclass = G_MOVABLE_CLASS (klass);
    mclass->type_flags = G_QUERY_ENTITY;
    mclass->enqueue = entity_enqueue;
    mclass->detached = entity_detached;

    param = x_param_str_new ("material",
                             "Material",
                             "The material name of the simple object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_MATERIAL, param);

    param = x_param_str_new("mesh",
                            "Mesh",
                            "The mesh name of the entity represented",
                            NULL,
                            X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_MESH, param);
}
