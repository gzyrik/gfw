#ifndef __X_I18N_H__
#define __X_I18N_H__
#include "xtype.h"
X_BEGIN_DECLS

#define  _(String) x_i18n_text (String)
#define Q_(String) x_i18n_dptext (NULL, String, 0)
#define N_(String) (String)
#define C_(Context,String) x_i18n_dptext (NULL, Context "\004" String, strlen (Context) + 1)
#define NC_(Context, String) (String)

void        x_i18n_bind_domain  (xcstr          domain,
                                 xcstr          path);

void        x_i18n_bind_codeset (xcstr          domain,
                                 xcstr          codeset);

void        x_i18n_domain       (xcstr          domain);


xOptionGroup* x_i18n_options    (void);

xcstr       x_i18n_text         (xcstr          msgid) X_FORMAT(1);

xcstr       x_i18n_ntext        (xcstr          msgid,
                                 xcstr          msgid_plural,
                                 xulong         n) X_FORMAT(2);

xcstr       x_i18n_dtext        (xcstr          domain,
                                 xcstr          msgid)X_FORMAT(2);

xcstr       x_i18n_dctext       (xcstr          domain,
                                 xcstr          msgid,
                                 xint           category)X_FORMAT(2);

xcstr       x_i18n_dntext       (xcstr          domain,
                                 xcstr          msgid,
                                 xcstr          msgid_plural,
                                 xulong         n) X_FORMAT(3);

xcstr       x_i18n_dcntext      (xcstr          domain,
                                 xcstr          msgid,
                                 xcstr          msgid_plural,
                                 xulong         n,
                                 xint           category)X_FORMAT(2);

xcstr       x_i18n_dptext       (xcstr          domain,
                                 xcstr          msg_ctxt_id,
                                 xsize          msgid_offset) X_FORMAT(2);

xcstr       x_i18n_dptext2      (xcstr          domain,
                                 xcstr          context,
                                 xcstr          msgid) X_FORMAT(3);

xcstr       x_i18n_strip        (xcstr          msgid,
                                 xcstr          msgval) X_FORMAT(1);

X_END_DECLS
#endif /* __X_I18N_H__ */

