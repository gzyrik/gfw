#ifndef __G_MANUAL_H__
#define __G_MANUAL_H__
#include "gmovable.h"
X_BEGIN_DECLS

#define G_TYPE_MANUAL              (g_manual_type())
#define G_MANUAL(object)           (X_INSTANCE_CAST((object), G_TYPE_MANUAL, gManual))
#define G_MANUAL_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_MANUAL, gManualClass))
#define G_IS_MANUAL(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_MANUAL))
#define G_MANUAL_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_MANUAL, gManualClass))

struct _gManual
{
    gMovable                parent;
    struct {
        gvec                pos;
        gvec                normal;
        gvec                texcoord[G_MAX_N_TEXUNITS];
        xbyte               texdim[G_MAX_N_TEXUNITS];
        gcolor              color;
    }                       tmp_v;
    xsize                   tex_index;
    xsize                   decl_size;

    xbool                   first_v:1;
    xbool                   any_indexed:1;
    xbool                   tmp_v_pending:1;
    xbool                   cur_updating : 1;
    xbool                   bit32_ibuf : 1;


    xuint32                 *tmp_ibuf;
    xsize                   tmp_ibuf_size;
    xsize                   est_nindex;

    xbyte                   *tmp_vbuf;
    xsize                   tmp_vbuf_size;
    xsize                   est_nvertex;
    xsize                   est_vsize;

    xArray                  *sections;
    xptr                    current;
};

struct _gManualClass
{
    gMovableClass           parent;
};

xType       g_manual_type           (void);

void        g_manual_estimate       (gManual        *manual,
                                     xsize          vertex_size,
                                     xsize          n_vertex,
                                     xsize          n_index);

void        g_manual_begin          (gManual        *manual,
                                     xcstr          meterial,
                                     xint           primitive);

void        g_manual_update         (gManual        *manual,
                                     xsize          section_idx);

void        g_manual_position         (gManual        *manual,
                                       gvec           pos);

void        g_manual_normal         (gManual        *manual,
                                     gvec           normal);

void        g_manual_texcoord       (gManual        *manual,
                                     xsize          dim,
                                     gvec           texcoord);
#define     g_manual_texcoord2(manual, u, v)                            \
    g_manual_texcoord (manual, 2, g_vec(u, v, 0, 0))

#define     g_manual_texcoord3(manual, v)                               \
    g_manual_texcoord (manual, 3, v)

void        g_manual_color          (gManual        *manual,
                                     gcolor         color);

void        g_manual_index          (gManual        *manual,
                                     xsize          index);

void        g_manual_triangle       (gManual        *manual,
                                     xsize          index0,
                                     xsize          index1,
                                     xsize          index2);

void        g_manual_quad           (gManual        *manual,
                                     xsize          index0,
                                     xsize          index1,
                                     xsize          index2,
                                     xsize          index3);

void        g_manual_end            (gManual        *manual);

gMesh*      g_manual_to_mesh        (gManual        *manual,
                                     xcstr          mesh,
                                     xcstr          group);

X_END_DECLS
#endif /* __G_MANUAL_H__ */
