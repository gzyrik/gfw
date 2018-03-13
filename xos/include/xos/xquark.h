#ifndef __X_QUARK_H__
#define __X_QUARK_H__
#include "xtype.h"
X_BEGIN_DECLS

xQuark      x_quark_try             (xcstr          str);

xQuark      x_quark_from_static     (xsstr          str);

xQuark      x_quark_from            (xcstr          str);

xistr       x_quark_str             (xQuark         quark);

xistr       x_intern_str_try        (xcstr          str);

xistr       x_intern_str            (xcstr          str);

xistr       x_intern_static_str     (xsstr          str);

X_END_DECLS
#endif /* __X_QUARK_H__ */
