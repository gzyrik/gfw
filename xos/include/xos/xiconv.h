#ifndef __X_ICONV_H__
#define __X_ICONV_H__
#include "xtype.h"
X_BEGIN_DECLS

xQuark      x_convert_domain        (void);

xIConv*     x_iconv_open            (xcstr          tocode,
                                     xcstr          fromcode);

void        x_iconv_close           (xIConv         *converter);

xsize       x_iconv                 (xIConv         *converter,
                                     xcptr          inbuf,
                                     xsize          *inbytes_left,
                                     xptr           outbuf,
                                     xsize          *outbytes_left);


xstr        x_convert               (xcptr          inbuf,
                                     xssize         len,            
                                     xcstr          to_codeset,
                                     xcstr          from_codeset,
                                     xsize          *bytes_read,     
                                     xsize          *bytes_written,  
                                     xError         **error) X_MALLOC;

xstr        x_convert_with_iconv    (xcptr          inbuf,
                                     xssize         len,
                                     xIConv         *converter,
                                     xsize          *bytes_read,     
                                     xsize          *bytes_written,  
                                     xError         **error) X_MALLOC;

xutf16      x_str_to_utf16          (xcstr          str,
                                     xssize         len,
                                     xsize          *bytes_read,
                                     xsize          *items_written,
                                     xError         **error) X_MALLOC;


xstr        x_utf16_to_str          (xcutf16        str,
                                     xssize         len,
                                     xsize          *bytes_read,
                                     xsize          *items_written,
                                     xError         **error) X_MALLOC;

xptr        x_str_to_locale         (xcstr          str,
                                     xssize         len,
                                     xsize          *bytes_read,
                                     xsize          *items_written,
                                     xError         **error) X_MALLOC;

xstr        x_locale_to_str         (xcptr          str,
                                     xssize         len,
                                     xsize          *bytes_read,
                                     xsize          *items_written,
                                     xError         **error) X_MALLOC;
X_END_DECLS
#endif /* __X_ICONV_H__ */
