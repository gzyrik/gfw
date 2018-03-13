#include "config.h"
#include "grenderqueue.h"
#include "gmaterial.h"
#include "gscene.h"
#include "grenderable.h"
#include "gscenerender.h"
#include "gpass.h"
#include "gtarget.h"
#include "gtechnique.h"

struct rpass
{
    gPass           *pass;
    gRenderContext  cxt;
};

struct rgroup
{
    gPass           *pass;
    xArray          *cxt_list; /* gRenderCxt */
};
struct collection
{
    xuint           organisation;/* whelther grouped or descending used */
    xArray          *grouped;    /* sorted struct rgroup */
    xArray          *descending; /* descending sorted  struct rpass */
};
static void
collection_free         (struct collection  *collect)
{
    x_array_unref (collect->grouped);
    x_array_unref (collect->descending);
}
static xint
rgroup_cmp              (struct rgroup  *rg0,
                         struct rgroup  *rg1)
{
    if (rg0->pass->hash < rg1->pass->hash)
        return -1;
    else if (rg0->pass->hash > rg1->pass->hash)
        return 1;
    else if (rg0->pass < rg1->pass)
        return -1;
    else
        return rg0->pass > rg1->pass;
}
static void rgroup_free (struct rgroup  *rg)
{
    x_array_unref(rg->cxt_list);
}
static void rgroup_clear (struct rgroup  *rg)
{
    rg->cxt_list->len = 0;
}
static void
collection_init         (struct collection  *collect)
{
    collect->organisation = OM_PASS_GROUP;
    collect->grouped      = x_array_new (sizeof (struct rgroup));
    collect->descending   = x_array_new (sizeof (struct rpass));
    collect->grouped->free_func = rgroup_free;
}
static void
collection_clear        (struct collection  *collect)
{
    x_array_foreach(collect->grouped, rgroup_clear, NULL);
    collect->grouped->len = 0;
    collect->descending->len = 0;
}
static void collect_pass(gPass *pass, xptr param[2])
{
    struct collection  *collect = param[0];
    gRenderContext     *cxt = param[1];

    if (collect->organisation & OM_SORT_DESCENDING){
        struct rpass rp;
        rp.pass = pass;
        rp.cxt = *cxt;
        x_array_append1 (collect->descending, &rp);
    }

    if (collect->organisation & OM_PASS_GROUP){
        xint j;
        struct rgroup *rg;

        j = x_array_insert_sorted (collect->grouped, &pass, NULL,
            (xCmpFunc)rgroup_cmp, FALSE);
        rg = &x_array_index (collect->grouped,struct rgroup, j);
        if (rg->pass != pass) {
            rg->pass = pass;
            rg->cxt_list = x_array_new (sizeof(gRenderContext));
        }
        x_array_append1 (rg->cxt_list, cxt);
    }
}
static void
collection_add_technique(struct collection  *collect,
                         gTechnique         *technique,
                         gRenderContext     *cxt)
{
    xptr param[2] = {collect, cxt};
    g_technique_foreach (technique, collect_pass, param);
}
static void
collection_sort         (struct collection  *collect,
                         const gCamera      *camera)
{
    if (collect->organisation & OM_SORT_DESCENDING) {
        /* sort */
    }
}
static void
collection_foreach      (struct collection  *collect,
                         xuint              organisation,
                         gSceneRender       *render,
                         xPtrArray          *manual_lights,
                         xbool              scissoring)
{
    xsize i,j;
    struct rgroup   *rg;
    struct rpass    *rp;
    gRenderContext  *cxt;
    gSceneRenderClass *klass;

    klass = G_SCENE_RENDER_GET_CLASS (render);
    switch (organisation) {
    case OM_PASS_GROUP:
        for (i=0; i<collect->grouped->len; ++i) {
            rg = &x_array_index (collect->grouped, struct rgroup, i);
            klass->on_pass (render, rg->pass);
            for (j=0; j<rg->cxt_list->len; ++j) {
                cxt = &x_array_index (rg->cxt_list, gRenderContext, j);
                klass->on_renderable (render, cxt,
                                       manual_lights, scissoring);
            }
        }
        break;
    case OM_SORT_DESCENDING:
        for (i=0; i<collect->descending->len; ++i) {
            rp = &x_array_index(collect->descending, struct rpass, i);
            klass->on_render(render, rp->pass, &rp->cxt,
                                    manual_lights, scissoring);
        }
        break;
    case OM_SORT_ASCENDING:
        for (i=collect->descending->len; i>0;) {
            --i;
            rp = &x_array_index(collect->descending, struct rpass, i);
            klass->on_render (render, rp->pass, &rp->cxt,
                                    manual_lights, scissoring);
        }
        break;
    }
}

struct priority_group
{
    xint                priority;
    struct collection   solid_no_receive_shadow;
    struct collection   solid_basic;
    struct collection   transparents_unsorted;
    struct collection   transparents;
    xbool               self_shadow;
};
static void
priority_group_free     (struct priority_group *pg)
{
    collection_free (&pg->solid_no_receive_shadow);
    collection_free (&pg->solid_basic);
    collection_free (&pg->transparents_unsorted);
    collection_free (&pg->transparents);
}
static xint
priority_group_cmp      (struct priority_group *pg0,
                         struct priority_group *pg1)
{
    return pg0->priority - pg1->priority;
}
static void
priority_group_init     (struct priority_group  *group)
{
    collection_init (&group->solid_no_receive_shadow);
    collection_init (&group->solid_basic);
    collection_init (&group->transparents_unsorted);
    collection_init (&group->transparents);
    /* Transparents will always be sorted this way */
    group->transparents.organisation=OM_SORT_DESCENDING;
}
static void
priority_group_clear    (struct priority_group  *group)
{
    collection_clear (&group->solid_no_receive_shadow);
    collection_clear (&group->solid_basic);
    collection_clear (&group->transparents_unsorted);
    collection_clear (&group->transparents);
}
static void
priority_group_sort     (struct priority_group  *group,
                         gCamera                *camera)
{
    collection_sort (&group->solid_basic, camera);
    collection_sort (&group->solid_no_receive_shadow, camera);
    collection_sort (&group->transparents, camera);
    collection_sort (&group->transparents_unsorted, camera);
}
static void
priority_group_add      (struct priority_group  *group,
                         gTechnique             *technique,
                         gRenderContext         *cxt,
                         xbool                  shadow_endabled)
{
    if (technique->transparent) {
        if (technique->unsorted_transparent)
            collection_add_technique (&group->transparents_unsorted,
                                      technique, cxt);
        else
            collection_add_technique (&group->transparents,
                                      technique, cxt);
    }
    else {
        if (!shadow_endabled ||
            (!technique->receive_shadow ||
             (!group->self_shadow && g_renderable_shadow (cxt->renderable, cxt->user_data) )))
            collection_add_technique (&group->solid_basic,
                                      technique, cxt);
        else
            collection_add_technique (&group->solid_no_receive_shadow,
                                      technique, cxt);
    }
}

struct render_group
{
    xint            group_id;
    xbool           shadow_enabled;
    xArray          *priorities; /* sorted struct priority_group */
};
static void
render_group_free       (struct render_group *rg)
{
    x_array_unref (rg->priorities);
}
static xint
render_group_cmp        (struct render_group *rg0,
                         struct render_group *rg1)
{
    return rg0->group_id - rg1->group_id;
}
static void
render_group_init       (struct render_group *group)
{
    group->priorities = x_array_new (sizeof(struct priority_group));
    group->priorities->free_func = priority_group_free;
}
static void
render_group_clear      (struct render_group *group)
{
    x_array_foreach(group->priorities, priority_group_clear, NULL);
}
static void
render_group_add        (struct render_group *group,
                         xint                priority,
                         gTechnique          *technique,
                         gRenderContext      *cxt)
{
    xint i;
    struct priority_group *p_group;

    i = x_array_insert_sorted (group->priorities, &priority, NULL,
                               (xCmpFunc)priority_group_cmp, FALSE);

    p_group = &x_array_index (group->priorities, struct priority_group, i);
    if (p_group->priority != priority) {
        p_group->priority = priority;
        priority_group_init (p_group);
    }

    priority_group_add (p_group, technique, cxt, group->shadow_enabled);
}

/* 普通渲染 */
static void
render_basic_object             (struct render_group    *group,
                                 gSceneRender           *render)
{
    xsize i;
    struct priority_group *p_group;

    for (i=0; i<group->priorities->len; ++i) {
        p_group = &x_array_index (group->priorities, struct priority_group, i);
        priority_group_sort (p_group, render->camera);
        collection_foreach (&p_group->solid_basic,
                            render->organisation, render, NULL, TRUE) ;
        collection_foreach (&p_group->transparents_unsorted,
                            render->organisation, render, NULL, TRUE);
        collection_foreach (&p_group->transparents,
                            OM_SORT_DESCENDING, render, NULL, TRUE);
    }
}
/* 处于'渲染纹理'步骤,渲染所有阴影源 */
static void
render_texture_shadow_caster    (struct render_group    *group,
                                 gSceneRender           *render)
{
    static xPtrArray null_lights={0,};
    xsize i;
    struct priority_group *p_group;
    xuint shadow_techinque = render->shadow_technique;

    if (shadow_techinque & SHADOWDETAILTYPE_ADDITIVE)
        g_scene_render_set_ambient (render, G_VEC_X);
    else /* 在调制模式下,阴影颜色作为环境光,无纹理时,绘制该颜色的阴影*/
        g_scene_render_set_ambient (render, render->shadow_color);

    for (i=0; i<group->priorities->len; ++i) {
        p_group = &x_array_index (group->priorities, struct priority_group, i);
        priority_group_sort (p_group, render->camera);
        collection_foreach (&p_group->solid_basic, render->organisation,
                            render, &null_lights, FALSE) ;
        collection_foreach (&p_group->solid_no_receive_shadow,render->organisation,
                            render, &null_lights, FALSE) ;
        collection_foreach (&p_group->transparents_unsorted, render->organisation,
                            render, &null_lights, FALSE) ;

        render->transparent_shadow_caster = TRUE;
        collection_foreach (&p_group->transparents,OM_SORT_DESCENDING,
                            render, &null_lights, FALSE) ;
        render->transparent_shadow_caster = FALSE;
    }
    g_scene_render_set_ambient (render, render->ambient_color);
}
/* 调制阴影,降低被阴影遮盖处的亮度.
 * - 绘制所有实心物体(阴影接收者)和不接收阴影的物体
 * - 针对每盏灯,渲染组中实心物体
 * - 最后渲染透明物体
 */
static void
render_modulative_texture_shadow(struct render_group *group,
                                 gSceneRender        *render)
{ 
    xsize i;
    struct priority_group *p_group;

    for (i=0; i<group->priorities->len; ++i) {
        p_group = &x_array_index (group->priorities, struct priority_group, i);
        priority_group_sort (p_group, render->camera);
        collection_foreach (&p_group->solid_basic,
                            render->organisation, render, NULL, TRUE) ;
        collection_foreach (&p_group->solid_no_receive_shadow,
                            render->organisation, render, NULL, TRUE) ;
    }
    if (render->illumination_stage == IRS_NONE) {
        render->illumination_stage = IRS_RENDER_RECEIVER_PASS;
        /*
           while () {
           visit->shadow_texture = ;
           render_texture_shadow_receiver (group, visit);
           }
           */
        render->illumination_stage = IRS_NONE;
    }
    for (i=0; i<group->priorities->len; ++i) {
        p_group = &x_array_index (group->priorities, struct priority_group, i);
        collection_foreach (&p_group->transparents_unsorted,
                            render->organisation, render, NULL, TRUE) ;
        collection_foreach (&p_group->transparents,
                            OM_SORT_DESCENDING, render, NULL, TRUE) ;
    }
}
static void
render_additive_texture_shadow  (struct render_group    *group,
                                 gSceneRender           *render)
{
}
static void
render_group_foreach        (struct render_group    *group,
                             gSceneRender           *render)
{
    xbool do_shadow = group->shadow_enabled && render->viewport->shadow_enabled;
    xint shadow_technique = render->shadow_technique;

    if (do_shadow && shadow_technique == SHADOWTYPE_STENCIL_ADDITIVE)
        ;/* render_additive_stencil_shadow (group, visitor); */

    else if (do_shadow && shadow_technique == SHADOWTYPE_STENCIL_ADDITIVE)
        ;/* render_modulative_stencil_shadow (group, visitor); */

    else if (shadow_technique & SHADOWDETAILTYPE_TEXTURE) {
        if (render->illumination_stage == IRS_RENDER_TO_TEXTURE)
            render_texture_shadow_caster (group, render);
        else {
            if (do_shadow) {
                if (shadow_technique & SHADOWDETAILTYPE_ADDITIVE)
                    /* 叠加阴影,多次渲染物体,增强非阴影区 */
                    render_additive_texture_shadow (group, render);
                else 
                    /* 调制阴影,降低被阴影遮盖处的亮度 */
                    render_modulative_texture_shadow (group, render);
            }
            else
                render_basic_object (group, render);
        }
    }
    else
        render_basic_object (group, render);
}
static void
queue_foreach           (gRenderQueue   *queue,
                         gSceneRender   *render)
{
    xsize i, n;
    struct render_group *group;
    gSceneRenderClass *klass;

    if (!queue->groups)
        return;
    klass = G_SCENE_RENDER_GET_CLASS (render);
    for (i=0; i<queue->groups->len; ++i) {
        group = &x_array_index (queue->groups, struct render_group, i);
        n = 0;
        while (klass->on_group(render, group->group_id, n++))
            render_group_foreach (group, render);
    }
}


static void
queue_clear             (gRenderQueue         *queue)
{
    if (!queue->groups) return;
    x_array_foreach(queue->groups, render_group_clear, NULL);
}

static void
queue_add_full          (gRenderQueue   *queue,
                         xint           group_id,
                         xint           priority,
                         gRenderable    *renderable,
                         gNode          *node,
                         xptr           user_data)
{
    xint i;
    struct render_group *group;
    gTechnique* technique;
    gRenderContext cxt = {node, user_data, renderable};

    technique = g_renderable_technique (renderable, user_data);
    x_return_if_fail(technique != NULL);
    x_assert (queue->dft_group > 0 && queue->dft_priority > 0);

    if (!group_id)
        group_id = queue->dft_group;
    if (!priority)
        priority = queue->dft_priority;

    i = x_array_insert_sorted (queue->groups, &group_id, NULL,
                               (xCmpFunc)render_group_cmp, FALSE);

    group = &x_array_index (queue->groups, struct render_group, i);
    if (group->group_id != group_id) {
        group->group_id = group_id;
        render_group_init (group);
    }

    render_group_add (group, priority, technique, &cxt);
}
X_DEFINE_TYPE (gRenderQueue, g_render_queue, X_TYPE_OBJECT);

static void
g_render_queue_init (gRenderQueue *queue)
{
    queue->groups = x_array_new (sizeof(struct render_group));
    queue->groups->free_func = render_group_free;
    queue->dft_group = 100;
    queue->dft_priority = 100;
    x_object_unsink (queue);
}

static void
g_render_queue_class_init (gRenderQueueClass *klass)
{
    klass->clear = queue_clear;
    klass->add = queue_add_full;
    klass->foreach = queue_foreach;
}

void
g_render_queue_clear    (gRenderQueue   *queue)
{
    gRenderQueueClass *klass;
    x_return_if_fail (G_IS_RENDER_QUEUE (queue));

    klass = G_RENDER_QUEUE_GET_CLASS (queue);
    if (klass->clear)
        klass->clear (queue);
}

void
g_render_queue_add_full (gRenderQueue   *queue,
                         xint           group_id,
                         xint           priority,
                         gRenderable    *renderable,
                         gNode          *node,
                         xptr           user_data)
{
    gRenderQueueClass *klass;
    x_return_if_fail (G_IS_RENDER_QUEUE (queue));

    klass = G_RENDER_QUEUE_GET_CLASS (queue);
    if (klass->add)
        klass->add (queue, group_id, priority, renderable, node, user_data);
}

void
g_render_queue_foreach  (gRenderQueue   *queue,
                         gSceneRender   *render)
{
    gRenderQueueClass *klass;
    x_return_if_fail (G_IS_RENDER_QUEUE (queue));

    klass = G_RENDER_QUEUE_GET_CLASS (queue);
    if (klass->foreach)
        klass->foreach (queue, render);
}

