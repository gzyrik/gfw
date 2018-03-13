#ifndef __X_MODULE_H__
#define __X_MODULE_H__
#include "xtype.h"
X_BEGIN_DECLS

xQuark      x_module_domain         (void);

xModule*    x_module_open           (xcstr          file_name,
                                     xError         **error);

void        x_module_close          (xModule        *module);

void        x_module_resident       (xModule        *module);

xptr        x_module_symbol         (xModule        *module,
                                     xcstr          symbol_name,
                                     xError         **error);
X_END_DECLS
#endif /* __X_MODULE_H__ */

