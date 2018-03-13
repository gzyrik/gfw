#ifndef RECT_H
#define RECT_H

/*
class Rect : public Figure
{
public:
    virtual xint get_area (Figure *fig);
};
*/

#include <xts.h>

X_BEGIN_DECLS

#define TYPE_RECT              (rect_type())
#define RECT(object)           (X_INSTANCE_CAST((object), TYPE_RECT, Rect))
#define RECT_CLASS(klass)      (X_CLASS_CAST((klass), TYPE_RECT, RectClass))
#define IS_RECT(object)        (X_INSTANCE_IS_TYPE((object), TYPE_RECT))
#define IS_RECT_CLASS(klass)   (X_CLASS_IS_TYPE((klass), TYPE_RECT))
#define RECT_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), TYPE_RECT, RectClass))

typedef struct _Rect        Rect;
typedef struct _RectClass   RectClass;

struct _Rect {
    xObject parent;
    xptr    priv;
};

struct _RectClass {
    xObjectClass parent;
};

xType      rect_type   (void);

X_END_DECLS

#endif /* RECT_H */
