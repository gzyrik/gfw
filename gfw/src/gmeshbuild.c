#include "config.h"
#include "gmesh-prv.h"
#include "grender.h"
#include "gbuffer.h"
#include <string.h>
static void
tesselate_mesh          (struct SubMesh       *submesh,
                         const xsize    width,
                         const xsize    height,
                         const xbool    double_side)
{
    xuint16 *p;
    gBuffer *ibuf;
    gIndexData *idata;
    int vInc, uInc, v, u, i;
    int vCount, uCount;

    submesh->primitive = G_RENDER_TRIANGLE_LIST;
    if (double_side) {
        i= 2;
        vInc = 1;
        v = 0; /* Start with front */
    }
    else {
        i= 1;
        vInc = 1;
        v = 0;
    }
    if (submesh->idata)
        g_index_data_delete (submesh->idata);
    idata = g_index_data_new ((width-1) * (height-1) * 2 * i* 3);
    ibuf = g_index_buffer_new (sizeof(xuint16), idata->count, NULL);
    g_index_data_bind(idata, G_INDEX_BUFFER(ibuf));

    p = g_buffer_lock (ibuf);
    while (i--) {
        u = 0;
        uInc = 1;

        vCount = height- 1;
        while (vCount--) {
            uCount = width- 1;
            while (uCount--) {
                /* first triangle in cell */
                *p++ = ((v + vInc) * width) + u;
                *p++ = (v * width) + u;
                *p++ = ((v + vInc) * width) + (u + uInc);
                /* second triangle in cell */
                *p++ = ((v + vInc) * width) + (u + uInc);
                *p++ = (v * width) + u;
                *p++ = (v * width) + (u + uInc);
                /* next column */
                u += uInc;
            }
            /* next row */
            v += vInc;
            u = 0;
        }

        /* Reverse vInc for double side */
        v = height- 1;
        vInc = -vInc;
    }
    g_buffer_unlock (ibuf);
    submesh->idata = idata;
}
static void
build_plane             (gMesh          *mesh,
                         gMeshBuildParam *param)
{
    gBuffer *vbuf;
    greal *p;
    gVertexData *vdata;
    struct SubMesh *submesh;
    xsize i;
    gVertexElem velem;
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);
    vdata = g_vertex_data_new ((param->x_segments+1)*(param->y_segments+1));

    memset(&velem, 0, sizeof(velem));
    velem.semantic = G_VERTEX_POSITION;
    velem.type = G_VERTEX_FLOAT3;
    g_vertex_data_add (vdata, &velem);

    if (param->normals) {
        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_NORMAL;
        velem.type = G_VERTEX_FLOAT3;
        g_vertex_data_add (vdata, &velem);
    }
    for (i=0;i<param->n_texcoords;++i) {
        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_TEXCOORD;
        velem.type = G_VERTEX_FLOAT2;
        velem.index = i;
        g_vertex_data_add (vdata, &velem);
    }
    g_vertex_data_alloc (vdata);

    vbuf = g_vertex_data_get (vdata, 0);
    p = g_buffer_lock (vbuf);
    X_STMT_START {
        greal xSpace = param->width / param->x_segments;
        greal ySpace = param->height / param->y_segments;
        greal halfWidth = param->width / 2;
        greal halfHeight = param->height / 2;
        greal xTex = (1.0f * param->u_tile) / param->x_segments;
        greal yTex = (1.0f * param->v_tile) / param->y_segments;
        xsize y, x,i;

        for (y = 0; y < param->y_segments + 1; ++y) {
            for (x = 0; x < param->x_segments + 1; ++x) {
                greal tmp[4];
                gvec vec = g_vec ((x * xSpace) - halfWidth,
                                  (y * ySpace) - halfHeight,
                                  0,0);
                vec = g_vec3_rotate (vec, param->rotate);
                vec = g_vec_add (vec, param->offset);
                g_vec_store (vec, tmp);
                *p++ = tmp[0]; *p++ = tmp[1]; *p++ = tmp[2];

                g_aabb_extent (&priv->aabb, vec);
                priv->radius_sq = g_vec_max(priv->radius_sq, g_vec3_len_sq (vec));

                if (param->normals) {
                    vec = g_vec3_rotate (G_VEC_Z, param->rotate);
                    g_vec_store (vec, tmp);
                    *p++ = tmp[0]; *p++ = tmp[1]; *p++ = tmp[2];
                }

                for (i = 0; i < param->n_texcoords; ++i) {
                    *p++ = x * xTex;
                    *p++ = 1 - (y * yTex);
                }
            }
        }
    } X_STMT_END;
    g_buffer_unlock (vbuf);
    g_vertex_data_delete (priv->vdata);
    priv->vdata = vdata;
    submesh = x_array_append1 (priv->submesh, NULL);
    submesh->shared_vdata = TRUE;
    tesselate_mesh (submesh, param->x_segments + 1, param->y_segments+1, FALSE);
}
static void
build_curved_plane      (gMesh          *mesh,
                         gMeshBuildParam *param)
{

}
static void
build_illusion_plane    (gMesh          *mesh,
                         gMeshBuildParam *param)
{
    gBuffer *vbuf;
    greal *p;
    gVertexData *vdata;
    struct SubMesh *submesh;
    gVertexElem velem;
    xsize i;
    greal cam_pos, sphere_radius;
    const greal SPHERE_RAD = 100.0;
    const greal CAM_DIST = 5.0;
    struct MeshPrivate *priv;

    priv = MESH_PRIVATE(mesh);
    vdata = g_vertex_data_new ((param->x_segments+1)*(param->keep_y_segments+1));

    memset(&velem, 0, sizeof(velem));
    velem.semantic = G_VERTEX_POSITION;
    velem.type = G_VERTEX_FLOAT3;
    g_vertex_data_add (vdata, &velem);

    if (param->normals) {
        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_NORMAL;
        velem.type = G_VERTEX_FLOAT3;
        g_vertex_data_add (vdata, &velem);
    }
    for (i=0;i<param->n_texcoords;++i) {
        memset(&velem, 0, sizeof(velem));
        velem.semantic = G_VERTEX_TEXCOORD;
        velem.type = G_VERTEX_FLOAT2;
        velem.index = i;
        g_vertex_data_add (vdata, &velem);
    }

    g_vertex_data_alloc (vdata);

    vbuf = g_vertex_data_get (vdata, 0);

    sphere_radius = SPHERE_RAD - param->curvature;
    cam_pos = sphere_radius - CAM_DIST;
    p = g_buffer_lock (vbuf);
    X_STMT_START {
        greal xSpace = param->width / param->x_segments;
        greal ySpace = param->height / param->y_segments;
        greal halfWidth = param->width / 2;
        greal halfHeight = param->height / 2;
        xsize y,x,i;
        greal sphDist;

        for (y = param->y_segments - param->keep_y_segments; y < param->y_segments + 1; ++y) {
            for (x = 0; x < param->x_segments + 1; ++x) {
                greal tmp[4],s,t;
                gvec vec = g_vec((x * xSpace) - halfWidth,
                                 (y * ySpace) - halfHeight,
                                 0,0);
                vec = g_vec3_rotate (vec, param->rotate);
                vec = g_vec_add (vec, param->offset);
                g_vec_store (vec, tmp);
                *p++ = tmp[0]; *p++ = tmp[1]; *p++ = tmp[2];

                g_aabb_extent (&priv->aabb, vec);
                priv->radius_sq = g_vec_max(priv->radius_sq, g_vec3_len_sq (vec));

                if (param->normals) {
                    /* Default normal is along unit Z */
                    gvec norm = g_vec3_rotate (G_VEC_Z, param->rotate);
                    g_vec_store (norm, tmp);
                    *p++ = tmp[0]; *p++ = tmp[1]; *p++ = tmp[2];
                }
                /* Generate texture coords */
                //vec = g_vec3_inv_rot (vec, param->rotate);
                vec = g_vec3_norm (vec);
                g_vec_store (vec, tmp);
                /* Find distance to sphere */
                sphDist = sqrtf(cam_pos*cam_pos * (tmp[1]*tmp[1]-1.0f)
                               + sphere_radius*sphere_radius) - cam_pos*tmp[1];

                tmp[0] *= sphDist;
                tmp[2] *= sphDist;

                /* x and y on sphere as texture coordinates, tiled */
                s = tmp[0] * (0.01f * param->u_tile);
                t = 1 - (tmp[2] * (0.01f * param->v_tile));
                for (i = 0; i < param->n_texcoords; ++i) {
                    *p++ = s;
                    *p++ = t;
                }
            }
        }
    } X_STMT_END;
    g_buffer_unlock (vbuf);
    g_vertex_data_delete (priv->vdata);
    priv->vdata = vdata;
    submesh = x_array_append1 (priv->submesh, NULL);
    submesh->shared_vdata = TRUE;
    tesselate_mesh (submesh, param->x_segments + 1, param->keep_y_segments+1, FALSE);
}
void
_g_build_manual_mesh    (gMesh          *mesh,
                         gMeshBuildParam *param)
{
    switch (param->type) {
    case G_MESH_BUILD_PLANE:
        build_plane (mesh, param);
        break;
    case G_MESH_BUILD_CURVED_PLANE:
        build_curved_plane (mesh, param);
        break;
    case G_MESH_BUILD_ILLUSION_PLANE:
        build_illusion_plane (mesh, param);
        break;
    }
}
