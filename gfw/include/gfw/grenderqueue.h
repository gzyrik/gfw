#ifndef __G_RENDER_QUEUE_H__
#define __G_RENDER_QUEUE_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_RENDER_QUEUE              (g_render_queue_type())
#define G_RENDER_QUEUE(object)           (X_INSTANCE_CAST((object), G_TYPE_RENDER_QUEUE, gRenderQueue))
#define G_RENDER_QUEUE_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_RENDER_QUEUE, gRenderQueueClass))
#define G_IS_RENDER_QUEUE(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_RENDER_QUEUE))
#define G_RENDER_QUEUE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_RENDER_QUEUE, gRenderQueueClass))

typedef enum {
    G_RENDER_DEFAULT = 0,
    G_RENDER_BACKGOUND,
    G_RENDER_SKIES_EARLY,
    G_RENDER_MAIN,
    G_RENDER_LAST = 100,
} gRenderGroup;

void        g_render_queue_clear    (gRenderQueue   *queue);

void        g_render_queue_add_full (gRenderQueue   *queue,
                                     xint           group_id,
                                     xint           priority,
                                     gRenderable    *renderable,
                                     gNode          *node,
                                     xptr           user_data);

#define     g_render_queue_add_data(queue, renderable, node, data)             \
    g_render_queue_add_full (queue, 0, 0, renderable, node, data)

#define     g_render_queue_add(queue, renderable, node)                        \
    g_render_queue_add_data (queue, renderable, node, NULL)

void        g_render_queue_foreach  (gRenderQueue   *queue,
                                     gSceneRender   *render);


struct _gRenderQueue
{
    xObject         parent;
    xArray          *groups;
    xint            dft_group;
    xint            dft_priority;
};

struct _gRenderQueueClass
{
    xObjectClass    parent;
    void (*clear)   (gRenderQueue *queue);
    void (*add)     (gRenderQueue *queue, xint group, xint priority,
                     gRenderable *renderable, gNode *node, xptr user);
    void (*foreach) (gRenderQueue *queue, gSceneRender *render);
};

xType       g_render_queue_type     (void);

X_END_DECLS
#endif /* __G_RENDER_QUEUE_H__ */
