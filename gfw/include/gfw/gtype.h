#ifndef __G_TYPE_H__
#define __G_TYPE_H__
#include <xos/xtype.h>
#include <gml/gtype.h>

#define G_MAX_N_WEIGHTS             4
#define G_MAX_N_TEXUNITS            16
#define G_MAX_N_SHADERS             4

X_BEGIN_DECLS
typedef struct _gNode               gNode;
typedef struct _gNodeClass          gNodeClass;
typedef struct _gSceneNode          gSceneNode;
typedef struct _gSceneNodeClass     gSceneNodeClass;
typedef struct _gShader             gShader;
typedef struct _gShaderClass        gShaderClass;
typedef struct _gProgram            gProgram;
typedef struct _gProgramClass       gProgramClass;
typedef struct _gUniform            gUniform;
typedef struct _gPass               gPass;
typedef struct _gPassClass          gPassClass;
typedef struct _gMaterial           gMaterial;
typedef struct _gMaterialClass      gMaterialClass;
typedef struct _gMaterialTemplet    gMaterialTemplet;
typedef struct _gMaterialTempletClass gMaterialTempletClass;
typedef struct _gTechnique          gTechnique;
typedef struct _gTechniqueClass     gTechniqueClass;
typedef struct _gTexUnit            gTexUnit;
typedef struct _gTexUnitClass       gTexUnitClass;
typedef struct _gTexture            gTexture;
typedef struct _gTextureClass       gTextureClass;
typedef struct _gRenderQueue        gRenderQueue;
typedef struct _gRenderQueueClass   gRenderQueueClass;
typedef struct _gRenderable         gRenderable;
typedef struct _gTarget             gTarget;
typedef struct _gTargetClass        gTargetClass;
typedef struct _gRender             gRender;
typedef struct _gRenderClass        gRenderClass;
typedef struct _gRenderableIFace    gRenderableIFace;
typedef struct _gMovable            gMovable;
typedef struct _gMovableClass       gMovableClass;
typedef struct _gManual             gManual;
typedef struct _gManualClass        gManualClass;
typedef struct _gSimple             gSimple;
typedef struct _gSimpleClass        gSimpleClass;
typedef struct _gRectangle          gRectangle;
typedef struct _gRectangleClass     gRectangleClass;
typedef struct _gBoundingBox        gBoundingBox;
typedef struct _gBoundingBoxClass   gBoundingBoxClass;
typedef struct _gFrustum            gFrustum;
typedef struct _gFrustumClass       gFrustumClass;
typedef struct _gCamera             gCamera;
typedef struct _gCameraClass        gCameraClass;
typedef struct _gLight              gLight;
typedef struct _gViewport           gViewport;
typedef struct _gSceneRender        gSceneRender;
typedef struct _gSceneRenderClass   gSceneRenderClass;
typedef struct _gScene              gScene;
typedef struct _gSceneClass         gSceneClass;
typedef struct _gBuffer             gBuffer;
typedef struct _gBufferClass        gBufferClass;
typedef struct _gMesh               gMesh;
typedef struct _gMeshClass          gMeshClass;
typedef struct _gAnimation          gAnimation;
typedef struct _gState              gState;
typedef struct _gStateSet           gStateSet;
typedef struct _gVertexBuffer       gVertexBuffer;
typedef struct _gVertexBufferClass  gVertexBufferClass;
typedef struct _gIndexBuffer        gIndexBuffer;
typedef struct _gIndexBufferClass   gIndexBufferClass;
typedef struct _gPixelBuffer        gPixelBuffer;
typedef struct _gPixelBufferClass   gPixelBufferClass;
typedef struct _gVertexData         gVertexData;
typedef struct _gSkeleton           gSkeleton;
typedef struct _gSkeletonClass      gSkeletonClass;
typedef struct _gSubEntity          gSubEntity;
typedef struct _gEntity             gEntity;
typedef struct _gEntityClass        gEntityClass;
typedef struct _gIndexData          gIndexData;
typedef struct _gVertexElem         gVertexElem;
typedef struct _gRenderOp           gRenderOp;
typedef struct _gVisibleBounds      gVisibleBounds;
typedef struct _gMovableContext     gMovableContext;
typedef struct _gVertexSpec         gVertexSpec;
typedef struct _gRenderContext      gRenderContext;
typedef struct _gSkyDomeParam       gSkyDomeParam;
typedef struct _gMeshBuildParam     gMeshBuildParam;
typedef struct _gFrameTimer         gFrameTimer;
typedef struct _gBBoardSet          gBBoardSet;
typedef struct _gBBoardSetClass     gBBoardSetClass;
typedef struct _gBBoard             gBBoard;
typedef struct _gBBoardChain        gBBoardChain;
typedef struct _gParamVec           gParamVec;
typedef struct _gQuery              gQuery;
typedef struct _gQueryClass         gQueryClass;
typedef struct _gRayQuery           gRayQuery;
typedef struct _gRayQueryClass      gRayQueryClass;
typedef struct _gRegionQuery        gRegionQuery;
typedef struct _gRegionQueryClass   gRegionQueryClass;
typedef struct _gSphereQuery        gSphereQuery;
typedef struct _gSphereQueryClass   gSphereQueryClass;
typedef struct _gAabbQuery          gAabbQuery;
typedef struct _gAabbQueryClass     gAabbQueryClass;
typedef struct _gCollideQuery       gCollideQuery;
typedef struct _gCollideQueryClass  gCollideQueryClass;
typedef struct _gParticle           gParticle;
typedef struct _gPEmitter           gPEmitter;
typedef struct _gPEmitterClass      gPEmitterClass;
typedef struct _gPAffector          gPAffector;
typedef struct _gPAffectorClass     gPAffectorClass;
typedef struct _gPRender            gPRender;
typedef struct _gPRenderIFace       gPRenderIFace;
typedef struct _gPSystem            gPSystem;
typedef struct _gPSystemClass       gPSystemClass;

typedef void   (*gFrameHook)        (xptr           self,
                                     greal          time_elapsed);

X_END_DECLS
#endif /* __G_TYPE_H__ */
