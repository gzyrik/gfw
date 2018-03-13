#ifndef __G_PSYSTEM_H__
#define __G_PSYSTEM_H__
#include "gmovable.h"
X_BEGIN_DECLS

#define G_TYPE_PSYSTEM              (g_psystem_type())
#define G_PSYSTEM(object)           (X_INSTANCE_CAST((object), G_TYPE_PSYSTEM, gPSystem))
#define G_PSYSTEM_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_PSYSTEM, gPSystemClass))
#define G_IS_PSYSTEM(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_PSYSTEM))
#define G_PSYSTEM_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_PSYSTEM, gPSystemClass))

xType       g_psystem_type          (void);

gPEmitter*  g_psystem_add_emitter   (gPSystem       *psystem,
                                     xcstr          emitter_type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

gPAffector* g_psystem_add_affector  (gPSystem       *psystem,
                                     xcstr          afftector_type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

gPRender*   g_psystem_set_render    (gPSystem       *psystem,
                                     xcstr          prender_type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

struct _gParticle
{
    gvec                position;
    gvec                direction;
    gcolor              color;
    greal               width;
    greal               height;
    xbool               visiable;

    gPEmitter           *emitter;
    gquat               rotate;
    grad                r_speed;
    greal               v_speed;
    greal               ttl; /* time to live */
    greal               life_time;
};

struct _gPSystem
{
    gMovable            parent;
    greal               speed_factor;
    xsize               pool_size;
    xbool               emitting;
    xPtrArray           *emitters;
    xPtrArray           *affectors;
    gPRender            *render;
    gPRenderIFace       *render_iface;

    greal               bounds_duration;
    xsize               p_quota;
    xsize               ee_quota;
    xint                fhook_add;

    /* free_p->len + active_p->len <= p_quota */
    xPtrArray           *free_p; /* free particle set */
    xPtrArray           *active_p; /* active particle set */

    /* count of each ee_set.n_ee in ee_pool == ee_quota */
    xArray              *ee_pool;  /* struct ee_set array */
    xsize               n_ee; /* emitted-emitter count <= ee_quota */
};

struct _gPSystemClass
{
    gMovableClass       parent;
};

X_END_DECLS
#endif /* __G_PSYSTEM_H__ */

