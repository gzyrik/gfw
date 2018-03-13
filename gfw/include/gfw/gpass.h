#ifndef __G_PASS_H__
#define __G_PASS_H__
#include "gtype.h"
#include "grender.h"
X_BEGIN_DECLS

#define G_TYPE_PASS             (g_pass_type())
#define G_PASS(object)          (X_INSTANCE_CAST((object), G_TYPE_PASS, gPass))
#define G_PASS_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_PASS, gPassClass))
#define G_IS_PASS(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_PASS))
#define G_PASS_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_PASS, gPassClass))

gTexUnit*   g_pass_add_texunit      (gPass          *pass,
                                     xcstr          name);

gShader*    g_pass_set_shader       (gPass          *pass,
                                     xcstr          name);

void        g_pass_foreach_shader   (gPass          *pass,
                                     xCallback      func,
                                     xptr           user_data);
struct _gPass
{
    xResource           parent;
    xstr                name;
    gTechnique          *technique;
    xuint32             hash;
    gCullMode           cull_mode;
    gFillMode           fill_mode;
    xPtrArray           *shaders;
    xPtrArray           *texUnits;
};

struct _gPassClass
{
    xResourceClass      parent;
};

xType       g_pass_type             (void);

X_END_DECLS
#endif /* __G_PASS_H__ */
