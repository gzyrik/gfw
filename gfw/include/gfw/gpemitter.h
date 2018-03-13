#ifndef __G_PEMITTER_H__
#define __G_PEMITTER_H__
#include "gtype.h"

#define G_TYPE_PEMITTER              (g_pemitter_type())
#define G_PEMITTER(object)           (X_INSTANCE_CAST((object), G_TYPE_PEMITTER, gPEmitter))
#define G_PEMITTER_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_PEMITTER, gPEmitterClass))
#define G_IS_PEMITTER(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_PEMITTER))
#define G_PEMITTER_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_PEMITTER, gPEmitterClass))

xType       g_pemitter_type         (void);

struct _gPEmitter
{
    xObject             parent;
    xbool               emitted; /* emitted by the other emitter */
    xint                ee_index;   /* the index at psystem->free_ee */
    xbool               enabled;
    greal               remainder;

    greal               min_duration;
    greal               max_duration;
    greal               remain_duration;

    greal               emit_rate;

    greal               min_repeat_delay;
    greal               max_repeat_delay;
    greal               remain_repeat_delay;

    grad                angle;
    gvec                direction;
    gvec                up;

    greal               min_speed;
    greal               max_speed;

    greal               min_ttl;
    greal               max_ttl;

    gcolor              color_start;
    gcolor              color_end;
};

struct _gPEmitterClass
{
    xObjectClass        parent;
    void (*init_particle) (gPEmitter *emitter, gParticle *particle);
    xsize (*emit_count) (gPEmitter *emitter, greal elapsed_time);
};

X_END_DECLS
#endif /* __G_PEMITTER_H__ */
