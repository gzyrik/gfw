#ifndef __G_PAFFECTOR_H__
#define __G_PAFFECTOR_H__
#include "gtype.h"

#define G_TYPE_PAFFECTOR              (g_paffector_type())
#define G_PAFFECTOR(object)           (X_INSTANCE_CAST((object), G_TYPE_PAFFECTOR, gPAffector))
#define G_PAFFECTOR_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_PAFFECTOR, gPAffectorClass))
#define G_IS_PAFFECTOR(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_PAFFECTOR))
#define G_PAFFECTOR_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_PAFFECTOR, gPAffectorClass))


xType       g_paffector_type        (void);

struct _gPAffector
{
    xObject             parent;
};

struct _gPAffectorClass
{
    xObjectClass        parent;
    void (*init_particle) (gPAffector *affector, gParticle *particle);
    void (*affect_particles) (gPAffector *affector, gPSystem *psystem, greal elapsed_time);
};

X_END_DECLS
#endif /* __G_PAFFECTOR_H__ */
