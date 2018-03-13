#ifndef __XOS_TYPE_H__
#define __XOS_TYPE_H__
#include "xconfig.h"
X_BEGIN_DECLS

typedef char                        xchar;
typedef unsigned char               xuchar, xuint8, xbyte;
typedef int                         xint;
typedef unsigned                    xuint;
typedef short                       xshort;
typedef unsigned short              xushort;
typedef long                        xlong;
typedef unsigned long               xulong;
typedef float                       xfloat;
typedef double                      xdouble;
typedef void                        *xptr;
typedef const void                  *xcptr;
typedef size_t                      xsize;
typedef ptrdiff_t                   xssize;
typedef xint                        xbool;
typedef xchar                       *xstr;
typedef const xchar                 *xcstr;
typedef xcstr                       xistr;
typedef xcstr                       xsstr;
typedef xstr                        *xstrv;
typedef xcstr                       *xcstrv;
typedef xsize                       xQuark;

typedef float                       xfloat32;
typedef short                       xint16;
typedef unsigned short              xuint16;
typedef int                         xint32;
typedef unsigned int                xuint32;
typedef long long                   xint64;
typedef unsigned long long          xuint64;

typedef xuint32                     xwchar;
typedef xuint16                     *xutf16;
typedef xsize                       xenum;
typedef const xuint16               *xcutf16;
typedef struct _xHashTable          xHashTable;
typedef struct _xTrash              xTrash;
typedef struct _xTimeVal            xTimeVal;
typedef struct _xSList              xSList;
typedef struct _xList               xList;
typedef struct _xModule             xModule;
typedef union  _xMutex              xMutex;
typedef struct _xRWLock             xRWLock;
typedef struct _xCond               xCond;
typedef struct _xRecMutex           xRecMutex;
typedef struct _xPrivate            xPrivate;
typedef struct _xThread             xThread;
typedef struct _xOnce               xOnce;
typedef struct _xIConv              xIConv;
typedef struct _xError              xError;
typedef struct _xDataList           xDataList;
typedef struct _xHook		        xHook;
typedef struct _xHookList           xHookList;
typedef struct _xString             xString;
typedef struct _xTestSuite          xTestSuite;
typedef struct _xTestCase           xTestCase;
typedef struct _xTimer              xTimer;
typedef struct _xDir                xDir;
typedef struct _xUri                xUri;
typedef struct _xOption             xOption;
typedef struct _xOptionGroup        xOptionGroup;
typedef struct _xOptionContext      xOptionContext;
typedef struct _xQueue              xQueue;
typedef struct _xASQueue            xASQueue;
typedef struct _xRWQueue            xRWQueue;
typedef struct _xThreadPool         xThreadPool;
typedef struct _xPtrArray           xPtrArray;
typedef struct _xArray              xArray;
typedef struct _xMainContext        xMainContext;
typedef struct _xMainLoop           xMainLoop;
typedef struct _xSource             xSource;
typedef struct _xSourceCallback     xSourceCallback;
typedef struct _xSourceFuncs        xSourceFuncs;
typedef struct _xWakeup             xWakeup;
typedef struct _xPollFd             xPollFd;
typedef struct _xGC                 xGC;
typedef struct _xJsonToken          xJsonToken;
typedef struct _xJsonParser         xJsonParser;

typedef xptr    (*xPtrFunc)         (xptr           data,
                                     ...);

typedef xint    (*xIntFunc)         (xptr           data,
                                     ...);

typedef void    (*xVoidFunc)        (xptr           data,
                                     ...);

typedef xbool   (*xBoolFunc)        (xptr           data,
                                     ...);

typedef xBoolFunc                   xWalkFunc;
typedef xBoolFunc                   xSourceFunc;
typedef xPtrFunc                    xThreadFunc;
typedef xPtrFunc                    xDupFunc;
typedef xPtrFunc                    xTranslateFunc;
typedef xVoidFunc                   xFreeFunc;
typedef xVoidFunc                   xVisitFunc;
typedef xVoidFunc                   xWorkFunc;
typedef xVoidFunc                   xCallback;
typedef xIntFunc                    xHashFunc;
typedef xIntFunc                    xCmpFunc;

#define X_BOOL_FUNC(fun)            ((xBoolFunc)(fun))
#define X_VOID_FUNC(fun)            ((xVoidFunc)(fun))
#define X_INT_FUNC(fun)             ((xIntFunc)(fun))
#define X_PTR_FUNC(fun)             ((xPtrFunc)(fun))
#define X_CALLBACK(fun)             ((xCallback)fun)
#define X_WALKFUNC(fun)             ((xWalkFunc)fun)

X_END_DECLS
#endif /* __XOS_TYPE_H__ */
