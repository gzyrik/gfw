#ifndef __XTS_TYPE_H__
#define __XTS_TYPE_H__
#include <xos/xtype.h>

X_BEGIN_DECLS

typedef struct _xCValue             xCValue;
typedef struct _xValue              xValue;
typedef struct _xParam              xParam;
typedef struct _xParamClass         xParamClass;
typedef struct _xParamInfo          xParamInfo;
typedef struct _xParamValue         xParamValue;
typedef struct _xConstructParam     xConstructParam;
typedef struct _xClosure            xClosure;
typedef struct _xCClosure           xCClosure;
typedef struct _xClosureNotifyData  xClosureNotifyData;
typedef struct _xSignal             xSignal;
typedef struct _xAtomicArray        xAtomicArray;
typedef struct _xEnumValue          xEnumValue;
typedef struct _xFlagsValue         xFlagsValue;
typedef struct _xEnumClass          xEnumClass;
typedef struct _xFlagsClass         xFlagsClass;
typedef struct _xParamChar          xParamChar;
typedef struct _xParamUChar         xParamUChar;
typedef struct _xParamBool          xParamBool;
typedef struct _xParamInt           xParamInt;
typedef struct _xParamUInt          xParamUInt;
typedef struct _xParamLong          xParamLong;
typedef struct _xParamULong         xParamULong;
typedef struct _xParamInt64         xParamInt64;
typedef struct _xParamUInt64        xParamUInt64;
typedef struct _xParamWChar         xParamWChar;
typedef struct _xParamEnum          xParamEnum;
typedef struct _xParamFlags         xParamFlags;
typedef struct _xParamFloat         xParamFloat;
typedef struct _xParamDouble        xParamDouble;
typedef struct _xParamStr           xParamStr;
typedef struct _xParamStrV          xParamStrV;
typedef struct _xParamParam         xParamParam;
typedef struct _xParamBoxed         xParamBoxed;
typedef struct _xParamPtr           xParamPtr;
typedef struct _xParamObject        xParamObject;
typedef struct _xParamOverride      xParamOverride;
typedef struct _xParamXType         xParamXType;

typedef xsize                       xType;
typedef struct _xFundamentalInfo    xFundamentalInfo;
typedef struct _xTypeInfo           xTypeInfo;
typedef struct _xIFaceInfo          xIFaceInfo;
typedef struct _xValueTable         xValueTable;
typedef struct _xIFace              xIFace;
typedef struct _xClass              xClass;
typedef struct _xInstance           xInstance;
typedef struct _xTypePlugin         xTypePlugin;
typedef struct _xTypePluginClass    xTypePluginClass;
typedef struct _xObject             xObject;
typedef struct _xObjectClass        xObjectClass;
typedef struct _xTypeModule         xTypeModule;
typedef struct _xTypeModuleClass    xTypeModuleClass;
typedef struct _xSignalInvocationHint xSignalInvocationHint;

typedef void    (*xNotify)          (xptr           custom,
                                     xptr           source);

#define	X_TYPE_FUNDAMENTAL_SHIFT	(2)
#define	X_TYPE_MAKE_FUNDAMENTAL(x)	((xType) ((x) << X_TYPE_FUNDAMENTAL_SHIFT))
#define X_TYPE_FUNDAMENTAL_USER     31

#define X_TYPE_INVALID			    X_TYPE_MAKE_FUNDAMENTAL (0)
#define X_TYPE_VOID                 X_TYPE_MAKE_FUNDAMENTAL (1)
#define X_TYPE_IFACE                X_TYPE_MAKE_FUNDAMENTAL (2)
#define X_TYPE_CHAR                 X_TYPE_MAKE_FUNDAMENTAL (3)
#define X_TYPE_UCHAR                X_TYPE_MAKE_FUNDAMENTAL (4)
#define X_TYPE_BOOL                 X_TYPE_MAKE_FUNDAMENTAL (5)
#define X_TYPE_INT                  X_TYPE_MAKE_FUNDAMENTAL (6)
#define X_TYPE_UINT                 X_TYPE_MAKE_FUNDAMENTAL (7)
#define X_TYPE_FLOAT                X_TYPE_MAKE_FUNDAMENTAL (8)
#define X_TYPE_DOUBLE               X_TYPE_MAKE_FUNDAMENTAL (9)
#define X_TYPE_LONG                 X_TYPE_MAKE_FUNDAMENTAL (10)
#define X_TYPE_ULONG                X_TYPE_MAKE_FUNDAMENTAL (11)
#define X_TYPE_INT64                X_TYPE_MAKE_FUNDAMENTAL (12)
#define X_TYPE_UINT64               X_TYPE_MAKE_FUNDAMENTAL (13)
#define X_TYPE_ENUM                 X_TYPE_MAKE_FUNDAMENTAL (14)
#define X_TYPE_FLAGS                X_TYPE_MAKE_FUNDAMENTAL (15)
#define X_TYPE_STR                  X_TYPE_MAKE_FUNDAMENTAL (16)
#define X_TYPE_PTR                  X_TYPE_MAKE_FUNDAMENTAL (17)
#define X_TYPE_PARAM                X_TYPE_MAKE_FUNDAMENTAL (18)
#define X_TYPE_BOXED                X_TYPE_MAKE_FUNDAMENTAL (19)
#define X_TYPE_OBJECT               X_TYPE_MAKE_FUNDAMENTAL (20)

X_END_DECLS
#endif /* __XTS_TYPE_H__ */
