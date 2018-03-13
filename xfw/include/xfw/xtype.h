#ifndef __XFW_TYPE_H__
#define __XFW_TYPE_H__
#include <xts/xtype.h>

X_BEGIN_DECLS

typedef struct _xFile               xFile;
typedef struct _xFileClass          xFileClass;
typedef struct _xArchive            xArchive;
typedef struct _xArchiveClass       xArchiveClass;
typedef struct _xCache              xCache;
typedef struct _xCodec              xCodec;
typedef struct _xImageCodec         xImageCodec;
typedef struct _xImageCodecClass    xImageCodecClass;
typedef struct _xCodecClass         xCodecClass;
typedef struct _xScript             xScript;
typedef struct _xChunk              xChunk;
typedef struct _xResource           xResource;
typedef struct _xResourceClass      xResourceClass;
typedef struct _xPixelBox           xPixelBox;
typedef struct _xImage              xImage;
typedef struct _xcolor              xcolor;
typedef struct _xrect               xrect;
typedef struct _xbox                xbox;
typedef enum _xBoxFace              xBoxFace;
typedef enum _xScaleFilter          xScaleFilter;
typedef enum _xPixelFormat          xPixelFormat;
typedef enum _xSeekOrigin           xSeekOrigin;

struct X_ALIGN(16) _xcolor
{
    xfloat r,g,b,a;
};

struct X_ALIGN(16) _xrect
{
    xsize   left, right;
    xsize   top, bottom;
};
struct X_ALIGN(16) _xbox
{
    xsize   left, right;
    xsize   top, bottom;
    xsize   front, back;
};

X_END_DECLS
#endif /* __XFW_TYPE_H__ */
