#ifndef __G_PRENDER_H__
#define __G_PRENDER_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_PRENDER              (g_prender_type())
#define G_PRENDER(object)           (X_INSTANCE_CAST((object), G_TYPE_PRENDER, gPRender))
#define G_PRENDER_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_PRENDER, gPRenderIFace))
#define G_IS_PRENDER(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_PRENDER))
#define G_PRENDER_GET_CLASS(object) (X_INSTANCE_GET_IFACE((object), G_TYPE_PRENDER, gPRenderIFace))

xType       g_prender_type          (void);

struct _gPRenderIFace
{
    xIFace              parent;
    void (*expand_pool) (gPRender *render, xsize pool_size);
    void (*partices_moved) (gPRender *render, xPtrArray* active_p);
    void (*partice_emitted) (gPRender *render, gParticle *p);
    void (*partice_expired) (gPRender *render, gParticle *p);
    void (*enqueue) (gPRender *render, gMovableContext *context, gPSystem *psystem);
};

X_END_DECLS
#endif /* __G_PRENDER_H__ */
