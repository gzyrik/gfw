#ifndef __G_MESH_PRV_H__
#define __G_MESH_PRV_H__
#include "gmesh.h"
#include "ganimation.h"
#include "grender.h"
X_BEGIN_DECLS
typedef struct _gBoneAssign         gBoneAssign;
typedef struct _gMeshLodUsage       gMeshLodUsage;
typedef struct _gEdgeData           gEdgeData;
typedef struct _gEdge               gEdge;
typedef struct _gTriangle           gTriangle;
typedef struct _gEdgeGroup          gEdgeGroup;

struct SubMesh
{
    xstr                name;
    gPrimitiveType      primitive;
    gVertexData         *vdata;
    gIndexData          *idata;
    xbool               shared_vdata;
    xstr                material;
    xArray              *bone_assign;
    xHashTable          *tex_aliases;
    xArray              *blend_map;
    gAnimationType      vdata_anim_type;
    xbool               bone_assign_dirty;
};

struct _gBoneAssign
{
    xsize       count;   
    xuint16     bindex[G_MAX_N_WEIGHTS];
    greal       weight[G_MAX_N_WEIGHTS];
};

struct _gEdge
{
    xsize       tri_index[2];
    xsize       vert_index[2];
    xsize       shared_vindex[2];
    /* 是否为退化边,即没有两个三角面. */
    xbool       degenerate;
};
struct _gTriangle
{
    xsize       index_set; 
    xsize       vertex_set; /* 指定所在顶点集合序号 */
    xsize       vert_index[3]; /* 三个顶点分别在相应顶点集合中的序号 */
    xsize       shared_vindex[3]; /* 在EdgeListBuilder::mVertices中的序号 */

};
/* 基于相同顶点数据集的一组边 */
struct _gEdgeGroup
{
    xsize       vdata_idx;  /* 顶点数据集序号*/
    gVertexData *vdata;     /* 顶点数据集指针 */
    xsize       tri_start;  /* 三角面在gEdgeData::triangles的开始序号 */
    xsize       tri_count;  /* 三角面的个数. */
    xArray      *edges;     /* 边的集合 */
};
struct _gEdgeData
{
    /* 所有三角面的列表,顺序与 groups 顺序一致 */
    xArray      *triangles;
    /* 所有三角面的法向,与 triangles 个数一致 */
    gvec        *normals;
    /* 所有三角面的光线朝向状态,与 triangles 个数一致 */
    xbyte       *faceings;
    /* 所有边集合的列表 */
    xArray      *groups;
    xbool       closed;
};
struct _gMeshLodUsage
{
    gEdgeData       *edge_data;
};


struct MeshPrivate
{
    xstr                name;
    xstr                skeleton;
    xstr                material;
    gVertexData         *vdata;
    xArray              *bone_assign;
    xArray              *blend_map;
    xArray              *submesh;
    xArray              *lod_usage;
    xbool               audo_edge;
    gaabb               aabb;
    gvec                radius_sq;
    xbool               edge_built;
    xbool               lod_manual;
    xHashTable          *animations;
    xArray              *poses;
    gMeshBuildParam     *build_param;
    gAnimationType      vdata_anim_type;
    xbool               bone_assign_dirty;
};
#define MESH_PRIVATE(o)  ((struct MeshPrivate*) (G_MESH (o)->priv))

X_INTERN_FUNC
void        _g_build_manual_mesh    (gMesh          *mesh,
                                     gMeshBuildParam *param);
X_INTERN_FUNC
void        _g_mesh_script_register (void);
X_INTERN_FUNC
void        _g_mesh_chunk_register  (void);
X_INTERN_FUNC
void        _g_mesh_chunk_put       (gMesh          *mesh,
                                     xFile          *file);
X_INTERN_FUNC
void        _g_mesh_script_put      (gMesh          *mesh,
                                     xFile          *file);
X_INTERN_FUNC
xArray*     _g_mesh_bone_blend_map  (gMesh          *mesh,
                                     xsize          sub_index);
X_INTERN_FUNC
void        _g_skeleton_script_register (void);
X_INTERN_FUNC
void        _g_skeleton_chunk_register (void);
X_INTERN_FUNC
void        _g_skeleton_chunk_put   (gSkeleton      *skeleton,
                                     xFile          *file);
X_INTERN_FUNC
void        _g_skeleton_script_put  (gSkeleton      *skeleton,
                                     xFile          *file);

X_END_DECLS
#endif /* __G_MESH_PRV_H__ */
