#ifndef __X_IMAGE_H__
#define __X_IMAGE_H__
#include "xpixel.h"
X_BEGIN_DECLS
/*
                                         +-------+
                                         |       |
                                         |  UP   |                
              + -------+         +-------+-------+-------+-------+
             /  UP   / |         |       |       |       |       |
            +-------+  |  LT     |  RT   |  BK   |  LT   |  FR   |
        RT  |  +    |  +         +-------+-------+-------+-------+
            | /     | /                  |       |
            +-------+                    |  DN   |
                                         +-------+ 
*/
enum _xBoxFace {
    X_BOX_FRONT,
    X_BOX_BACK,
    X_BOX_LEFT,
    X_BOX_RIGHT,
    X_BOX_UP,
    X_BOX_DOWN,
    X_BOX_FACES
};
struct _xImage
{
    xsize           width;
    xsize           height;
    xsize           depth;
    xsize           mipmaps;
    xPixelFormat    format;
    xsize           bytes;
    xsize           faces;
    xbyte           *data[6];
    xint            ref_count;
};

xImage*     x_image_load            (xcstr          filename,
                                     xcstr          group,
                                     xbool          cube);

xImage*     x_image_ref             (xImage         *image);

xPixelBox   x_image_pixel_box       (xImage         *image,
                                     xsize          face,
                                     xsize          mipmap);

xint        x_image_unref           (xImage         *image);

X_END_DECLS
#endif /* __X_IMAGE_H__ */
