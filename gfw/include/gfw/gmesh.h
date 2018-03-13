#ifndef __G_MESH_H__
#define __G_MESH_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_MESH             (g_mesh_type())
#define G_MESH(object)          (X_INSTANCE_CAST((object), G_TYPE_MESH, gMesh))
#define G_MESH_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_MESH, gMeshClass))
#define G_IS_MESH(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_MESH))
#define G_MESH_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_MESH, gMeshClass))

typedef enum 
{
    G_MESH_BUILD_PLANE,
    G_MESH_BUILD_CURVED_PLANE,
    G_MESH_BUILD_ILLUSION_PLANE
} gMeshBuildType;

gMesh*      g_mesh_new              (xcstr          file,
                                     xcstr          group,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

gMesh*      g_mesh_attach           (xcstr          name,
                                     xcptr          client);

void        g_mesh_detach           (gMesh*         mesh,
                                     xcptr          client);

gMesh*      g_mesh_build            (gMeshBuildParam*param,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

gMesh*      g_mesh_bezier           (xcstr          name,
                                     xcstr          group,
                                     ...) X_NULL_TERMINATED;

void        g_mesh_write            (gMesh          *mesh,
                                     xFile          *file,
                                     xbool          binery);

void        g_mesh_foreach          (gMesh          *mesh,
                                     xCallback      callback,
                                     xptr           user_data);

void        g_mesh_states           (gMesh          *mesh,
                                     gStateSet      *states);



struct _gMesh
{
    xResource           parent;
    xptr                priv;
};

struct _gMeshClass
{
    xResourceClass      parent;
};

struct _gMeshBuildParam
{
    gMeshBuildType      type;
    greal               width;      /* 网格宽度 */
    greal               height;     /* 网格长度 */
    greal               curvature; 
    xsize               x_segments; /* 网格行数 */
    xsize               y_segments; /* 网格列数 */
    xsize               keep_y_segments;
    xbool               normals;    /* 是否产生顶点法向 */
    xsize               n_texcoords;/* 纹理坐标数,0即不产生纹理坐标 */
    greal               u_tile;     /* 纹理横行坐标范围[0,u_tile] */
    greal               v_tile;     /* 纹理纵向坐标范围[0,v_tile] */
    gquat               rotate;     /* 网格的旋转 */
    gvec                offset;     /* 网格的平移 */
};

xType       g_mesh_type             (void);

X_END_DECLS
#endif /* __G_MESH_H__ */
