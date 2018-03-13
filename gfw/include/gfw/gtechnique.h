#ifndef __G_TECHNIQUE_H__
#define __G_TECHNIQUE_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_TECHNIQUE             (g_technique_type())
#define G_TECHNIQUE(object)          (X_INSTANCE_CAST((object), G_TYPE_TECHNIQUE, gTechnique))
#define G_TECHNIQUE_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_TECHNIQUE, gTechniqueClass))
#define G_IS_TECHNIQUE(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_TECHNIQUE))
#define G_TECHNIQUE_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_TECHNIQUE, gTechniqueClass))

gPass*      g_technique_add_pass    (gTechnique     *technique,
                                     xcstr          name);

void        g_technique_foreach     (gTechnique     *technique,
                                     xCallback      func,
                                     xptr           user_data);

struct _gTechnique
{
    xResource           parent;
    xstr                name;
    gMaterial           *material;
    xbool               transparent;
    xbool               unsorted_transparent;
    xbool               receive_shadow;
    xPtrArray           *passes;            
};

struct _gTechniqueClass
{
    xResourceClass      parent;
};

xType       g_technique_type        (void);

X_END_DECLS
#endif /* __G_TECHNIQUE_H__ */
