#include "config.h"
#include "gpemitter.h"
#include "gpsystem.h"

X_DEFINE_TYPE (gPEmitter, g_pemitter, X_TYPE_OBJECT);

static void
pemitter_set_enabled    (gPEmitter      *emitter,
                         xbool          enabled)
{
    if (emitter->enabled == enabled)
        return;
    emitter->enabled = enabled;
    if (enabled) {
        emitter->remain_duration = g_real_rand (emitter->min_duration,
                                                emitter->max_duration);
    }
    else {
        emitter->remain_repeat_delay = g_real_rand (emitter->min_repeat_delay,
                                                    emitter->max_repeat_delay);
    }
}
static void
pemitter_init_particle  (gPEmitter      *emitter,
                         gParticle      *particle)
{
    particle->position = G_VEC_0;
    particle->color = g_vec_rlerp (emitter->color_start, emitter->color_end);
    particle->ttl = g_real_rlerp (emitter->min_ttl, emitter->max_ttl);
    particle->v_speed = g_real_rlerp (emitter->min_speed, emitter->max_speed);
    particle->direction = g_vec3_rand (emitter->direction, emitter->up,
                                       g_real_rand(0, 1)*emitter->angle);
    particle->visiable = TRUE;
}
static xsize 
pemitter_emit_count     (gPEmitter      *emitter,
                         greal          elapsed_time)
{
    if (emitter->enabled) {
        int count;
        emitter->remainder += emitter->emit_rate * elapsed_time;
        count = (int)emitter->remainder;
        emitter->remainder -= count;
        if (emitter->remain_duration > 0) {
            emitter->remain_duration -= elapsed_time;
            if (emitter->remain_duration <= 0)
                pemitter_set_enabled(emitter, FALSE);
        }
        return count;
    }
    else {
        if (emitter->remain_repeat_delay > 0) {
            emitter->remain_repeat_delay -= elapsed_time;
            if (emitter->remain_repeat_delay <= 0)
                pemitter_set_enabled(emitter, TRUE);
        }
    }
    return 0;
}
static void
g_pemitter_init         (gPEmitter      *self)
{
    self->max_speed = 10;
    self->min_speed = 15;
    self->max_ttl = 10;
    self->min_ttl = 10;
    self->emit_rate = 10;
    self->angle = G_PI;
    self->enabled = TRUE;
    self->color_end = G_VEC_1;
    self->direction = G_VEC_Y;
    self->up = G_VEC_Z;
}
static void
g_pemitter_class_init   (gPEmitterClass *klass)
{
    klass->init_particle = pemitter_init_particle;
    klass->emit_count = pemitter_emit_count;
}
