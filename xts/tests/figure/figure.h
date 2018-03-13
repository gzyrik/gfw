#ifndef FIGURE_H
#define FIGURE_H
/*
iface Figure : public xIFace
{
public:
    virtual xint get_area (Figure *fig);
};
*/
#include <xts.h>

X_BEGIN_DECLS

#define TYPE_FIGURE              (figure_type())
#define FIGURE(object)           (X_INSTANCE_CAST((object), TYPE_FIGURE, Figure))
#define FIGURE_CLASS(klass)      (X_CLASS_CAST(     (klass), TYPE_FIGURE, FigureIFace))
#define IS_FIGURE(object)        (X_INSTANCE_IS_TYPE((object), TYPE_FIGURE))
#define FIGURE_GET_CLASS(object) (X_INSTANCE_GET_IFACE((object), TYPE_FIGURE, FigureIFace))

typedef struct _Figure          Figure;
typedef struct _FigureIFace     FigureIFace;

struct _FigureIFace {
    xIFace  parent;
    xint (*get_area) (Figure *fig);
};

xType   figure_type (void);
xint    figure_get_area(Figure *fig);

X_END_DECLS
#endif /* FIGURE_H */
/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
