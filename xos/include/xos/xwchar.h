#ifndef __X_WCHAR_H__
#define __X_WCHAR_H__
#include "xtype.h"
X_BEGIN_DECLS

xwchar          x_str_get_wchar         (xcstr          str);

xwchar          x_str_get_wchar_s       (xcstr          str,
                                         xssize         max_len);

const xchar*    x_utf8_skip             (void);

#define         x_str_next_wchar(p)     (((xchar*)p) + x_utf8_skip()[*(xuchar*)(p)])

xbool           x_wchar_validate        (xwchar         wc);

xbool           x_wchar_iswide          (xwchar         wc);

xsize           x_wchar_width           (xwchar         wc);

xbool           x_str_validate         (xcstr          str,
                                         xssize         len,
                                         xcstr          *end);

xsize           x_str_width             (xcstr          str);

xsize           x_wchar_to_utf8         (xwchar         wc,
                                         xstr           outbuf);

#define         x_wchar_utf8_len(wc)                                    \
    ((wc) < 0x80 ? 1 :                                                  \
     ((wc) < 0x800 ? 2 :                                                \
      ((wc) < 0x10000 ? 3 :                                             \
       ((wc) < 0x200000 ? 4 :                                           \
        ((wc) < 0x4000000 ? 5 : 6))))                                   )

X_END_DECLS
#endif /* __X_WCHAR_H__ */

