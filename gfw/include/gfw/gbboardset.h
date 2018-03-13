#ifndef __G_BBOARDSET_H__
#define __G_BBOARDSET_H__
#include "gsimple.h"
X_BEGIN_DECLS

#define G_TYPE_BBOARD_SET              (g_bboardset_type())
#define G_BBOARD_SET(object)           (X_INSTANCE_CAST((object), G_TYPE_BBOARD_SET, gBBoardSet))
#define G_BBOARD_SET_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_BBOARD_SET, gBBoardSetClass))
#define G_IS_BBOARD_SET(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_BBOARD_SET))
#define G_BBOARD_SET_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_BBOARD_SET, gBBoardSetClass))

struct _gBBoard
{
    gvec        position;
    gvec        direction;
    gcolor      color;
    greal       width;
    greal       height;
    xbool       visiable;
    grad        rotate;
    xssize      tex_index;
    grect       tex_rect;
};

xType       g_bboardset_type        (void);

void        g_bboardset_texcoord    (gBBoardSet     *bboardset,
                                     xsize          start,
                                     xsize          count,
                                     grect          *texcoord);

void        g_bboardset_texgrid     (gBBoardSet     *bboardset,
                                     xsize          rows,
                                     xsize          cols);

xArray*     g_bboardset_lock        (gBBoardSet     *bboardset);

void        g_bboardset_unlock      (gBBoardSet     *bboardset);

X_END_DECLS
#endif /* __G_BBOARDSET_H__ */
