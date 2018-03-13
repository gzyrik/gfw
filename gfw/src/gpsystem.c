#include "config.h"
#include "gpsystem.h"
#include "grender.h"
#include "gprender.h"
#include "gpemitter.h"
#include "gpaffector.h"

struct ee_set {
    xType       type;
    xsize       n_ee; /* the number of emitted-emmitter */
    xArray      *params; /* the array of xParamValue */
    xPtrArray   *free_ee;
};
static void
ee_set_free             (struct ee_set  *ees)
{
    x_array_unref (ees->params);
    x_ptr_array_unref (ees->free_ee);
}
static void
paritcle_free           (gParticle      *p)
{
    x_slice_free(gParticle, p);
}
X_DEFINE_TYPE (gPSystem, g_psystem, G_TYPE_MOVABLE);

enum {
    PROP_0,
    PROP_PARTICLE_QUOTA,
    PROP_EMITTER_QUOTA,
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gPSystem *psystem = (gPSystem*)object;

    switch(property_id) {
    case PROP_PARTICLE_QUOTA:
        psystem->p_quota = x_value_get_uint (value);
        break;
    case PROP_EMITTER_QUOTA:
        psystem->ee_quota = x_value_get_uint (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    gPSystem *psystem = (gPSystem*)object;

    switch(property_id) {
    case PROP_PARTICLE_QUOTA:
        x_value_set_uint (value, psystem->p_quota);
        break;
    case PROP_EMITTER_QUOTA:
        x_value_set_uint (value, psystem->ee_quota);
        break;
    }
}
static void
particle_motion         (gParticle      *p,
                         greal          time_elapsed)
{
    p->position = g_vec_add (p->position, g_vec_scale (p->direction, p->v_speed*time_elapsed));
}
static xbool
particle_elapsed        (gParticle      *p,
                         greal          time_elapsed)
{
    p->ttl -= time_elapsed;
    return p->ttl > 0;
}
static void
psystem_apply_motion    (gPSystem       *psystem,
                         greal          time_elapsed)
{
    xsize i;
    gParticle *p;

    for (i=0; i<psystem->active_p->len; ++i) {
        p = psystem->active_p->data[i];
        particle_motion(p, time_elapsed);
    }
    if (psystem->render_iface)
        psystem->render_iface->partices_moved (psystem->render, psystem->active_p);
}
static void
psystem_run_affectors   (gPSystem       *psystem,
                         greal          time_elapsed)
{
    xsize i;
    gPAffector *affector;
    gPAffectorClass *klass;

    for (i=0; i<psystem->affectors->len; ++i) {
        affector = psystem->affectors->data[i];
        klass = G_PAFFECTOR_GET_CLASS (affector);
        klass->affect_particles (affector, psystem, time_elapsed);
    }
}
static gParticle*
psystem_new_particle    (gPSystem       *psystem,
                         xint           ee_index)
{
    gParticle *p;

    p = x_ptr_array_pop (psystem->free_p);
    if (p) {
        x_ptr_array_push (psystem->active_p, p);
        if (ee_index >= 0){
            struct ee_set *ees;
            ees = &x_array_index (psystem->ee_pool, struct ee_set, ee_index);
            p->emitter = x_ptr_array_pop (ees->free_ee);
            if (p->emitter)
                x_ptr_array_push (psystem->emitters, p->emitter);
        }
    }
    return p;
}
static void
psystem_do_emitter    (gPSystem       *psystem,
                       gPEmitter      *emitter,
                       xsize          requestd,
                       greal          time_elapsed)
{
    xsize i,j;
    gParticle  *p;
    greal dtime;
    gPAffector *affector;
    gPEmitterClass *eclass;
    gPAffectorClass *fclass;

    dtime = time_elapsed/requestd;
    time_elapsed = 0;
    for (i=0; i<requestd; ++i) {
        p = psystem_new_particle (psystem, emitter->ee_index);
        if (!p) return;

        /* inited by the emitter */
        eclass = G_PEMITTER_GET_CLASS (emitter);
        eclass->init_particle (emitter, p);

        /* apply motion */
        particle_motion (p, time_elapsed);

        /* inited by all affectors */
        for (j=0; j<psystem->affectors->len; ++j) {
            affector = psystem->affectors->data[j];
            fclass = G_PAFFECTOR_GET_CLASS (affector);
            fclass->init_particle (affector, p);
        }

        time_elapsed += dtime;
        psystem->render_iface->partice_emitted (psystem->render, p);
    }
}


static void
psystem_run_emitters    (gPSystem       *psystem,
                         greal          time_elapsed)
{
    xsize i;
    xsize totoal_req;
    xsize *request;
    gPEmitter *emitter;
    gPEmitterClass *klass;

    totoal_req = 0; /* count the request number */
    request = x_alloca (sizeof(xsize) * psystem->emitters->len);
    for (i=0; i<psystem->emitters->len; ++i) {
        emitter = psystem->emitters->data[i];
        klass = G_PEMITTER_GET_CLASS (emitter);
        request[i] = klass->emit_count (emitter, time_elapsed);
        totoal_req += request[i];
    }
    /* if reach limit, average scale */
    if (totoal_req > psystem->free_p->len) {
        greal ratio = (greal)psystem->free_p->len/(greal)totoal_req;
        for (i=0;i<psystem->emitters->len;++i) {
            request[i] = (xsize)(request[i]*ratio);
        }
    }
    /* foreach to emit new particle */
    for (i=0; i<psystem->emitters->len; ++i) {
        emitter = psystem->emitters->data[i];
        psystem_do_emitter (psystem, emitter, request[i], time_elapsed);
    }
}
static void
psystem_expire          (gPSystem       *psystem,
                         greal          time_elapsed)
{
    xsize i;
    gParticle *p;

    for (i=0;i<psystem->active_p->len;) {
        p = psystem->active_p->data[i];
        if (particle_elapsed (p, time_elapsed)){
            ++i;
            continue;
        }
        x_ptr_array_erase (psystem->active_p, i);
        x_ptr_array_push (psystem->free_p, p);
        /* notify particle render */
        psystem->render_iface->partice_expired (psystem->render, p);
        if (p->emitter) {
            struct ee_set *ees;
            ees = &x_array_index (psystem->ee_pool, struct ee_set, p->emitter->ee_index);
            x_ptr_array_push (ees->free_ee, p->emitter);
            x_ptr_array_erase_data (psystem->emitters, p->emitter);
            p->emitter = NULL;
        }
    }
}
static void
psystem_expand_pool     (gPSystem       *psystem)
{
    xsize i, j, k;
    gParticle *p;
    gPEmitter *ee;

    i=psystem->free_p->len+psystem->active_p->len;
    if (i != psystem->p_quota) {
        while (i<psystem->p_quota) {
            p = x_slice_new0 (gParticle);
            x_ptr_array_push (psystem->free_p, p);
            ++i;
        }
        psystem->render_iface->expand_pool (psystem->render, psystem->p_quota);
    }

    if (psystem->n_ee < psystem->ee_quota && psystem->ee_pool->len > 0) {
        xsize n_ee; /* per type count */
        struct ee_set *ees;
        n_ee = psystem->ee_quota / psystem->ee_pool->len;
        for (j=0;j<psystem->ee_pool->len;++j) {
            ees = &x_array_index (psystem->ee_pool, struct ee_set, j);
            for (k=ees->n_ee; k< n_ee; ++k) {
                ee = x_object_newv (ees->type, ees->params->len, ees->params->data);
                ee->ee_index = j;
                x_ptr_array_push (ees->free_ee, ee);
            }
            ees->n_ee = n_ee;
        }
    }
}
static void
psystem_update_bounds   (gPSystem       *psystem,
                         greal          time_elapsed)
{
    psystem->bounds_duration -= time_elapsed;
    if (psystem->bounds_duration < 0) {
        xsize i;
        gaabb aabb;
        gParticle *p;

        aabb.extent = 0;
        psystem->bounds_duration = 1;
        for (i=0; i<psystem->active_p->len; ++i) {
            p = x_ptr_array_index (psystem->active_p, gParticle, i);
            g_aabb_extent(&aabb, p->position);
        }
        g_movable_resize ((gMovable*)psystem, &aabb);
    }
}
static void
psystem_update          (gPSystem       *psystem,
                         greal          time_elapsed)
{
    time_elapsed *= psystem->speed_factor;

    psystem_expand_pool (psystem);
    psystem_expire (psystem, time_elapsed);
    psystem_run_affectors (psystem, time_elapsed);
    psystem_apply_motion (psystem, time_elapsed);
    if (psystem->emitting)
        psystem_run_emitters (psystem, time_elapsed);
    psystem_update_bounds (psystem, time_elapsed);
}

static void
psystem_attached        (gMovable       *movable,
                         gNode          *node)
{
    gPSystem * psystem= (gPSystem*)movable;
    if (!psystem->fhook_add)
        g_render_add_fhook (psystem_update, psystem);
    ++psystem->fhook_add;
}
static void
psystem_detached        (gMovable       *movable,
                         gNode          *node)
{
    gPSystem * psystem= (gPSystem*)movable;
    --psystem->fhook_add;
    if (!psystem->fhook_add)
        g_render_remove_fhook (psystem_update, psystem);
}
static void
psystem_enqueue         (gMovable       *movable,
                         gMovableContext*context)
{
    gPSystem * psystem= (gPSystem*)movable;
    gPRenderIFace *riface = psystem->render_iface;

    riface->enqueue (psystem->render, context,psystem);
}
static void
g_psystem_init          (gPSystem       *self)
{
    self->emitting = TRUE;
    self->speed_factor = 1;
    self->p_quota = 100;
    self->ee_quota = 100;
    self->emitters = x_ptr_array_new_full (0, x_object_unref);
    self->affectors = x_ptr_array_new_full (0, x_object_unref);
    self->free_p = x_ptr_array_new();
    self->active_p = x_ptr_array_new();
    self->ee_pool = x_array_new_full (sizeof(struct ee_set), 0, ee_set_free);
}
static void
g_psystem_class_init    (gPSystemClass  *klass)
{
    xParam *param;
    xObjectClass *oclass;
    gMovableClass *mclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;

    mclass = G_MOVABLE_CLASS (klass);
    mclass->type_flags = G_QUERY_PARTICLE;
    mclass->attached = psystem_attached;
    mclass->detached = psystem_detached;
    mclass->enqueue = psystem_enqueue;

    param = x_param_uint_new ("particle-quota",
                              "particle pool size",
                              "The size of the pool of particle",
                              1, X_MAXUINT32, 100,
                              X_PARAM_STATIC_STR
                              | X_PARAM_READWRITE
                              | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_PARTICLE_QUOTA, param);

    param = x_param_uint_new ("emitter-quota",
                              "emitted-emitter pool size",
                              "The size of the pool of emitted-emitter",
                              1, X_MAXUINT32, 100,
                              X_PARAM_STATIC_STR
                              | X_PARAM_READWRITE
                              | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_EMITTER_QUOTA, param);
}





gPEmitter*
g_psystem_add_emitter   (gPSystem       *psystem,
                         xcstr          emitter_type,
                         xcstr          first_property,
                         ...)
{
    xType xtype;
    gPEmitter *emitter;
    va_list argv;

    x_return_val_if_fail (G_IS_PSYSTEM (psystem), NULL);

    xtype = x_type_from_mime (G_TYPE_PEMITTER, emitter_type);
    x_return_val_if_fail (xtype != X_TYPE_INVALID, NULL);

    va_start(argv, first_property);
    emitter = x_object_new_valist (xtype, first_property, argv);
    va_end(argv);

    x_object_sink (emitter);
    emitter->ee_index = -1;
    emitter->emitted = FALSE;
    x_ptr_array_append1 (psystem->emitters, emitter);

    return emitter;
}
gPAffector*
g_psystem_add_affector  (gPSystem       *psystem,
                         xcstr          afftector_type,
                         xcstr          first_property,
                         ...)
{
    xType xtype;
    gPAffector* affector;
    va_list argv;

    x_return_val_if_fail (G_IS_PSYSTEM (psystem), NULL);

    xtype = x_type_from_mime (G_TYPE_PAFFECTOR, afftector_type);
    x_return_val_if_fail (xtype != X_TYPE_INVALID, NULL);

    va_start(argv, first_property);
    affector = x_object_new_valist (xtype, first_property, argv);
    va_end(argv);

    x_object_sink (affector);
    x_ptr_array_append1 (psystem->affectors, affector);

    return affector;
}
gPRender*
g_psystem_set_render    (gPSystem       *psystem,
                         xcstr          prender_type,
                         xcstr          first_property,
                         ...)
{
    xType xtype;
    gPRender* prender;
    va_list argv;

    x_return_val_if_fail (G_IS_PSYSTEM (psystem), NULL);

    xtype = x_type_from_mime (G_TYPE_PRENDER, prender_type);
    x_return_val_if_fail (xtype != X_TYPE_INVALID, NULL);

    va_start(argv, first_property);
    prender = x_object_new_valist (xtype, first_property, argv);
    va_end(argv);

    x_object_sink (prender);
    psystem->render = prender;
    psystem->render_iface = G_PRENDER_GET_CLASS(prender);

    return prender;
}
