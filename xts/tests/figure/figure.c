#include "figure.h"
#include <xos.h>
X_DEFINE_IFACE (Figure, figure);

static void
figure_default_init(FigureIFace *iface)
{
}
xint
figure_get_area(Figure *fig)
{
    FigureIFace *iface =  FIGURE_GET_CLASS(fig);
    return iface->get_area(FIGURE(fig));
}
