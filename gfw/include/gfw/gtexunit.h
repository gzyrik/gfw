#ifndef __G_TEXUNIT_H__
#define __G_TEXUNIT_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_TEXUNIT             (g_texunit_type())
#define G_TEXUNIT(object)          (X_INSTANCE_CAST((object), G_TYPE_TEXUNIT, gTexUnit))
#define G_TEXUNIT_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_TEXUNIT, gTexUnitClass))
#define G_IS_TEXUNIT(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_TEXUNIT))
#define G_TEXUNIT_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_TEXUNIT, gTexUnitClass))

void        g_texunit_bind          (gTexUnit       *texUnit);

void        g_texunit_unbind        (gTexUnit       *texUnit);

void        g_texunit_set_texture   (gTexUnit       *texUnit,
                                     gTexture       *texture);

gTexture*   g_texunit_texture       (gTexUnit       *texUnit,
                                     xsize          frame_index);

void        g_texunit_apply_textures(gTexUnit       *texUnit,
                                     xHashTable     *texture_aliases);

struct _gTexUnit
{
    xResource           parent;
    xsize               index;
    xstr                name;
    gPass               *pass;
    xsize               frame;
    xPtrArray           *textures;
    volatile xint       bound;
};

struct _gTexUnitClass
{
    xResourceClass      parent;
};

xType       g_texunit_type          (void);

X_END_DECLS
#endif /* __G_TEXUNIT_H__ */
