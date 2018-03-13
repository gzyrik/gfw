#include "rect.h"
#include "figure.h"
#include <xos.h>
static void rect_figure_iface       (FigureIFace    *iface);
X_DEFINE_DYNAMIC_TYPE_EXTENDED (Rect, rect, X_TYPE_OBJECT, 0,
                                X_IMPLEMENT_IFACE_DYNAMIC (TYPE_FIGURE,rect_figure_iface));
#define RECT_PRIVATE(o)  ((struct RectPrivate*) (RECT (o)->priv))
enum { PROP_0, PROP_WIDTH, PROP_HEIGHT, N_PROPERTIES};
struct RectPrivate
{
    xint            width;
    xint            height;
};
static xint
rect_get_area           (Figure *fig)
{
    struct RectPrivate *priv;
    x_return_val_if_fail (IS_RECT (fig), 0);

    priv = RECT_PRIVATE (fig);
    return priv->height * priv->width;
}
static void
rect_figure_iface       (FigureIFace    *iface)
{
    iface->get_area = rect_get_area;
}

static void
set_property            (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    struct RectPrivate *priv = RECT_PRIVATE(object);
    switch(property_id)
    {
    case PROP_WIDTH:
        priv->width =  x_value_get_int (value);
        break;
    case PROP_HEIGHT:
        priv->height =  x_value_get_int (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{

}

static void
rect_init               (Rect           *rect)
{
    struct RectPrivate *priv;
    priv = x_instance_private(rect, TYPE_RECT);
    rect->priv = priv;
}

static void
rect_class_init         (RectClass      *klass)
{
    xObjectClass   *oclass;
    xParam         *param;
    oclass = X_OBJECT_CLASS(klass);
    x_class_set_private (klass, sizeof (struct RectPrivate));
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    param = x_param_int_new ("width","width","width",
        0, X_MAXINT32, 0,
        X_PARAM_STATIC_STR
        |X_PARAM_CONSTRUCT_ONLY
        |X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_WIDTH,param);

    param = x_param_int_new ("height","height","height",
        0, X_MAXINT32, 0,
        X_PARAM_STATIC_STR
        |X_PARAM_CONSTRUCT_ONLY
        |X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_HEIGHT,param);
}
static void
rect_class_finalize     (RectClass      *klass)
{

}
xbool X_MODULE_EXPORT
type_module_main        (xTypeModule    *module)
{
    rect_register(module);
    return TRUE;
}
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
