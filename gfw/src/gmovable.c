#include "config.h"
#include "gmovable.h"
#include "gnode.h"
#include "gsimple.h"
#include "gentity.h"
#include "gbboardset.h"
#include "gmaterial.h"
#include "grenderqueue.h"
struct X_ALIGN(16) MovablePrivate
{
    xstr            name;
    xuint           query_flags;
    xint            render_group;
    gaabb           aabb;
    greal           radius;
    gBoundingBox    *boundingbox;

    xbool           visible      : 1;
    xbool           cast_shadows : 1;
};
X_DEFINE_TYPE_EXTENDED (gMovable, g_movable, X_TYPE_OBJECT, X_TYPE_ABSTRACT,);
#define MOVABLE_PRIVATE(o)  ((struct MovablePrivate*)(G_MOVABLE (o)->priv))
static  xHashTable      *_movables;
enum {
    PROP_0,
    PROP_GROUP,
    PROP_QUERY_FLAGS,
    PROP_VISIBLE,
    PROP_NAME,
    PROP_LBBOX_VISIBLE,
    N_PROPERTIES
};
enum {
    SIG_ATTACHED,
    SIG_DETACHED,
    SIG_RESIZE,
    SIG_FINALIZE,
    N_SIGNALS
};
static xuint            _signal[N_SIGNALS];
static void
set_property            (xObject            *movable,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    struct MovablePrivate *priv;
    priv = MOVABLE_PRIVATE (movable);

    switch(property_id) {
    case PROP_GROUP:
        priv->render_group =  x_value_get_uint (value);
        break;
    case PROP_QUERY_FLAGS:
        priv->query_flags =  x_value_get_uint (value);
        break;
    case PROP_VISIBLE:
        priv->visible =  x_value_get_bool (value);
        break;
    case PROP_NAME:
        if (priv->name) {
            x_hash_table_remove (_movables, priv->name);
            x_free (priv->name);
        }
        priv->name = x_value_dup_str (value);
        if (priv->name) {
            x_hash_table_insert (_movables, priv->name, movable);
        }
        break;
    case PROP_LBBOX_VISIBLE:
        if (x_value_get_bool (value)) {
            if (!priv->boundingbox) {
                priv->boundingbox = x_object_new (G_TYPE_BOUNDINGBOX, NULL);
                g_boundingbox_set (priv->boundingbox, &priv->aabb);
            }
        }
        else if (priv->boundingbox) {
            x_object_unref (priv->boundingbox);
            priv->boundingbox = NULL;
        }
        break;
    }
}

static void
get_property            (xObject            *movable,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    struct MovablePrivate *priv;
    priv = MOVABLE_PRIVATE (movable);

    switch(property_id) {
    case PROP_GROUP:
        x_value_set_uint (value, priv->render_group);
        break;
    case PROP_QUERY_FLAGS:
        x_value_set_uint (value, priv->query_flags);
        break;
    case PROP_VISIBLE:
        x_value_set_bool (value, priv->visible);
        break;
    case PROP_NAME:
        x_value_set_str (value, priv->name);
        break;
    case PROP_LBBOX_VISIBLE:
        x_value_set_bool (value, priv->boundingbox != NULL);
        break;
    }
}
static void
movable_finalize        (xObject        *movable)
{
    struct MovablePrivate *priv;
    priv = MOVABLE_PRIVATE (movable);

    x_signal_emit (movable, _signal[SIG_FINALIZE], 0);

    if (priv->name) {
        x_hash_table_remove (_movables, priv->name);
        x_free (priv->name);
        priv->name = NULL;
    }

    X_OBJECT_CLASS(g_movable_parent_class)->finalize (movable);
}
static void
g_movable_init          (gMovable       *movable)
{
    struct MovablePrivate *priv;
    priv = x_instance_private(movable, G_TYPE_MOVABLE);
    movable->priv = priv;

    x_object_unsink (movable);
    priv->aabb.extent = 1;
    priv->aabb.minimum = g_vec(-1,-1,-1,0);
    priv->aabb.maximum = g_vec(1,1,1,0);
}
static void
g_movable_class_init    (gMovableClass  *klass)
{
    xParam         *param;
    xObjectClass   *oclass;

    x_class_set_private(klass, sizeof(struct MovablePrivate));
    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = movable_finalize;

    if (!_movables)
        _movables = x_hash_table_new (x_str_hash, x_str_cmp);

    param = x_param_str_new ("name",
                             "Movable name",
                             "The name of the movable object",
                             NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_NAME, param);

    param = x_param_uint_new ("group",
                              "Group",
                              "The render group id of the movable object",
                              0, G_RENDER_LAST, G_RENDER_DEFAULT,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_GROUP,param);

    param = x_param_uint_new ("query-flags",
                              "Query flags",
                              "The query flags mask of the movable object",
                              0, X_MAXUINT32, -1,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_QUERY_FLAGS,param);

    param = x_param_bool_new ("visible",
                              "Visible",
                              "The visible of the movable object",
                              TRUE,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE | X_PARAM_CONSTRUCT);
    x_oclass_install_param(oclass, PROP_VISIBLE,param);

    param = x_param_bool_new ("lbbox-visible",
                              "Local bounding box visible",
                              "Whether the local bounding box is visible",
                              FALSE,
                              X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_LBBOX_VISIBLE,param);

    _signal[SIG_ATTACHED] =
        x_signal_new ("attached",
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_FIRST,
                      X_STRUCT_OFFSET (gMovableClass, attached),
                      NULL,
                      NULL,
                      X_TYPE_VOID,
                      1,
                      G_TYPE_NODE);

    _signal[SIG_DETACHED] =
        x_signal_new ("detached",
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_LAST,
                      X_STRUCT_OFFSET (gMovableClass, detached),
                      NULL,
                      NULL,
                      X_TYPE_VOID,
                      1,
                      G_TYPE_NODE);

    _signal[SIG_RESIZE] =
        x_signal_new ("resize",
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_LAST,
                      X_STRUCT_OFFSET (gMovableClass, resized),
                      NULL,
                      NULL,
                      X_TYPE_VOID,
                      0);

    _signal[SIG_FINALIZE] = 
        x_signal_new ("finalize",
                      X_CLASS_TYPE (klass),
                      X_SIGNAL_RUN_LAST,
                      X_STRUCT_OFFSET (xObjectClass, finalize),
                      NULL,
                      NULL,
                      X_TYPE_VOID,
                      0);
}

gMovable*
g_movable_get           (xcstr          name)
{
    x_return_val_if_fail (name != NULL, NULL);

    return x_hash_table_lookup (_movables, name);
}
gMovable*
g_movable_new           (xcstr          type,
                         xcstr          first_property,
                         ...)
{
    gMovable *movable;
    va_list argv;

    xType xtype = x_type_from_mime (G_TYPE_MOVABLE, type);
    x_return_val_if_fail (xtype != X_TYPE_INVALID, NULL);

    va_start(argv, first_property);
    movable = x_object_new_valist (xtype, first_property, argv);
    va_end(argv);

    return movable;
}

xbool
g_movable_match         (gMovable       *movable,
                         xuint          type_mask,
                         xuint          query_mask)
{
    struct MovablePrivate *priv;

    x_return_val_if_fail (G_IS_MOVABLE (movable), FALSE);
    priv = MOVABLE_PRIVATE (movable);

    return priv->query_flags & query_mask;
}
void
g_movable_resize        (gMovable       *movable,
                         const gaabb    *aabb)
{
    struct MovablePrivate *priv;

    x_return_if_fail (G_IS_MOVABLE (movable));
    priv = MOVABLE_PRIVATE (movable);

    if (!g_aabb_eq (&priv->aabb, aabb)) {
        priv->aabb = *aabb;
        if (priv->boundingbox)
            g_boundingbox_set (priv->boundingbox, aabb);
        x_signal_emit (movable, _signal[SIG_RESIZE], 0);
    }
}

gaabb*
g_movable_lbbox         (gMovable       *movable)
{
    struct MovablePrivate *priv;

    x_return_val_if_fail (G_IS_MOVABLE (movable), NULL);
    priv = MOVABLE_PRIVATE (movable);

    return &priv->aabb;
}

void
g_movable_enqueue       (gMovable       *movable,
                         gMovableContext* context)
{
    gMovableClass  *klass;
    struct MovablePrivate *priv;

    x_return_if_fail (G_IS_MOVABLE (movable));
    priv = MOVABLE_PRIVATE (movable);

    klass = G_MOVABLE_GET_CLASS (movable);
    x_return_if_fail (klass->enqueue != NULL);

    if (priv->visible)
        klass->enqueue (movable, context);
    if (priv->boundingbox)
        g_movable_enqueue ((gMovable*)priv->boundingbox,context);
}

void
_g_movable_init         (void)
{
    G_TYPE_RECTANGLE;
    G_TYPE_ENTITY;
    G_TYPE_BBOARD_SET;
}
