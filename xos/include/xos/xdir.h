#ifndef __X_DIR_H__
#define __X_DIR_H__
#include "xtype.h"
X_BEGIN_DECLS

xDir*       x_dir_open              (xcstr          path,
                                     xError         **error);

xcstr       x_dir_read              (xDir           *dir);

void        x_dir_rewind            (xDir           *dir);

void        x_dir_close             (xDir           *dir);

xQuark      x_file_domain           (void);

void        x_dir_foreach           (xcstr          path,
                                     xVisitFunc     visit,
                                     xptr           user_data);
X_END_DECLS
#endif /* __X_DIR_H__ */

