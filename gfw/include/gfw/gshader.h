#ifndef __G_SHADER_H__
#define __G_SHADER_H__
#include "gtype.h"
#include "gbuffer.h"
X_BEGIN_DECLS

#define G_TYPE_SHADER               (g_shader_type())
#define G_SHADER(object)            (X_INSTANCE_CAST((object), G_TYPE_SHADER, gShader))
#define G_SHADER_CLASS(klass)       (X_CLASS_CAST((klass), G_TYPE_SHADER, gShaderClass))
#define G_IS_SHADER(object)         (X_INSTANCE_IS_TYPE((object), G_TYPE_SHADER))
#define G_SHADER_GET_CLASS(object)  (X_INSTANCE_GET_CLASS((object), G_TYPE_SHADER, gShaderClass))

#define G_TYPE_PROGRAM              (g_program_type())
#define G_PROGRAM(object)           (X_INSTANCE_CAST((object), G_TYPE_PROGRAM, gProgram))
#define G_PROGRAM_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_PROGRAM, gProgramClass))
#define G_IS_PROGRAM(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_PROGRAM))
#define G_PROGRAM_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_PROGRAM, gProgramClass))

typedef enum {
    G_SHADER_VERTEX,
    G_SHADER_PIXEL,
    G_SHADER_GEOMETRY,
    G_SHADER_COUNT,
} gShaderType;

/* uniform data type */
typedef enum {
    G_UNIFORM_VOID,
    G_UNIFORM_INT1,
    G_UNIFORM_INT2,
    G_UNIFORM_INT3,
    G_UNIFORM_INT4,
    G_UNIFORM_SAMPLER1D,
    G_UNIFORM_SAMPLER2D,
    G_UNIFORM_SAMPLER3D,
    G_UNIFORM_SAMPLERCUBE,
    G_UNIFORM_SAMPLER1DSHADOW,
    G_UNIFORM_SAMPLER2DSHADOW,
    G_UNIFORM_FLOAT1,
    G_UNIFORM_FLOAT2,
    G_UNIFORM_FLOAT3,
    G_UNIFORM_FLOAT4,
    G_UNIFORM_MATRIX_2X2,
    G_UNIFORM_MATRIX_2X3,
    G_UNIFORM_MATRIX_2X4,
    G_UNIFORM_MATRIX_3X2,
    G_UNIFORM_MATRIX_3X3,
    G_UNIFORM_MATRIX_3X4,
    G_UNIFORM_MATRIX_4X2,
    G_UNIFORM_MATRIX_4X3,
    G_UNIFORM_MATRIX_4X4
} gUniformType;

#define G_UNIFORM_IS_FLOAT(type) (G_UNIFORM_FLOAT1 <= type)

/* auto uniform spec */
typedef enum {
    G_UNIFORM_MANUAL,

    G_UNIFORM_MODEL_MATRIX,
    G_UNIFORM_MODEL_AFFINE_ARRAY,

    G_UNIFORM_NODE_MATRIX,
    G_UNIFORM_NODEVIEWPROJ_MATRIX,

    G_UNIFORM_WORLD_MATRIX,
    G_UNIFORM_INVERSE_WORLD_MATRIX,
    G_UNIFORM_TRANSPOSE_WORLD_MATRIX,
    G_UNIFORM_INVERSE_TRANSPOSE_WORLD_MATRIX,
    G_UNIFORM_WORLD_MATRIX_ARRAY_3x4,
    G_UNIFORM_WORLD_MATRIX_ARRAY,

    G_UNIFORM_VIEW_MATRIX,
    G_UNIFORM_INVERSE_VIEW_MATRIX,
    G_UNIFORM_TRANSPOSE_VIEW_MATRIX,
    G_UNIFORM_INVERSE_TRANSPOSE_VIEW_MATRIX,

    G_UNIFORM_PROJECTION_MATRIX,
    G_UNIFORM_INVERSE_PROJECTION_MATRIX,
    G_UNIFORM_TRANSPOSE_PROJECTION_MATRIX,
    G_UNIFORM_INVERSE_TRANSPOSE_PROJECTION_MATRIX,

    G_UNIFORM_VIEWPROJ_MATRIX,
    G_UNIFORM_INVERSE_VIEWPROJ_MATRIX,
    G_UNIFORM_TRANSPOSE_VIEWPROJ_MATRIX,
    G_UNIFORM_INVERSE_TRANSPOSE_VIEWPROJ_MATRIX,

    G_UNIFORM_WORLDVIEW_MATRIX,
    G_UNIFORM_INVERSE_WORLDVIEW_MATRIX,
    G_UNIFORM_TRANSPOSE_WORLDVIEW_MATRIX,
    G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEW_MATRIX,

    G_UNIFORM_WORLDVIEWPROJ_MATRIX,
    G_UNIFORM_INVERSE_WORLDVIEWPROJ_MATRIX,
    G_UNIFORM_TRANSPOSE_WORLDVIEWPROJ_MATRIX,
    G_UNIFORM_INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX,

    G_UNIFORM_FOG_COLOUR,
    G_UNIFORM_FOG_PARAMS,

    G_UNIFORM_LIGHT_COUNT,
    ///当前环境光,float4类型
    G_UNIFORM_AMBIENT_LIGHT_COLOUR,
    ///某光源的光颜色,float4类型,需额外参数指定该光源索引号
    G_UNIFORM_LIGHT_DIFFUSE_COLOUR,
    G_UNIFORM_LIGHT_SPECULAR_COLOUR,
    G_UNIFORM_LIGHT_ATTENUATION,
    ///某光源位置,float4类型,需额外参数指定该光源索引号
    G_UNIFORM_LIGHT_POSITION,
    G_UNIFORM_LIGHT_POSITION_OBJECT_SPACE,
    G_UNIFORM_LIGHT_DIRECTION,
    G_UNIFORM_LIGHT_INDEX,

    G_UNIFORM_CAMERA_POSITION,
    ///物体局部坐标系中的镜头位置
    G_UNIFORM_CAMERA_POSITION_OBJECT_SPACE,
    G_UNIFORM_TEXTURE_VIEWPROJ_MATRIX,

    /* elapsed time */
    G_UNIFORM_TIME,
    G_UNIFORM_FRAME_TIME,
    G_UNIFORM_FPS,

    /* elapsed time mod extra */
    G_UNIFORM_TIME_0_X,
    G_UNIFORM_COS_TIME_0_X,
    G_UNIFORM_SIN_TIME_0_X,
    G_UNIFORM_TAN_TIME_0_X,
    /* 4-element vector of time0_x, sintime0_x, costime0_x, tantime0_x */
    G_UNIFORM_TIME_0_X_PACKED,

    /* as TIME_0_X but scaled to [0,1] */
    G_UNIFORM_TIME_0_1,
    G_UNIFORM_COS_TIME_0_1,
    G_UNIFORM_SIN_TIME_0_1,
    G_UNIFORM_TAN_TIME_0_1,
    /* as time0_x_packed but all values scaled to [0,1] */
    G_UNIFORM_TIME_0_1_PACKED,


    G_UNIFORM_TEXTURE_SIZE,
    G_UNIFORM_TEXTURE_MATRIX,
    G_UNIFORM_VIEWPORT_SIZE,
    G_UNIFORM_FOV,

    /* custom, elem_size must 4, extra is global id */
    G_UNIFORM_CUSTOM,
} gUniformSemantic;

typedef enum {
    G_UNIFORM_GLOBAL = 1,
    G_UNIFORM_OBJECT = 2,
    G_UNIFORM_LIGHT  = 4,
    G_UNIFORM_PASS   = 8,
    G_UNIFORM_ALL    = 0xFFFF
} gUniformScope;

/* alloc client */
gShader*    g_shader_attach         (xcstr          name,
                                     xcptr          client);
/* remove client */
void        g_shader_detach         (gShader        *shader,
                                     xcptr          client);

/* register uniform */
void        g_shader_register       (gShader        *shader,
                                     gUniform       *uniform);

/* manual write a uniform */
void        g_shader_put            (gShader        *shader,
                                     xcptr          client,
                                     xcstr          uniform,
                                     xcptr          value);

/* manual write a uniform */
void        g_shader_puts           (gShader        *shader,
                                     xcptr          client,
                                     xcstr          uniform,
                                     xcstr          value);

/* link the uniform, then can be auto refresh */
void        g_shader_link           (gShader        *shader,
                                     xcstr          uniform,
                                     gUniformSemantic semantic,
                                     greal          extra);

/* auto write all linked uniform from scene info */
void        g_shader_refresh        (gShader        *shader,
                                     xcptr          client,
                                     gSceneRender   *render,
                                     gUniformScope  scope);

void        g_shader_upload         (gShader        *shader,
                                     xcptr          client);

/* bind to gpu,upload uniform to gpu memory */
void        g_shader_bind           (gShader        *shader);


/* unbind from gpu */
void        g_shader_unbind         (gShader        *shader);

/* vertex attribute index */
xint        g_shader_vindex         (gShader        *shader,
                                     gVertexSemantic semantic,
                                     xint           index);

struct _gUniform
{
    xcstr           name;
    xint            index; /* platform special register */
    gUniformType    type;
    xsize           elem_size;
    xsize           array_count;
    gUniformSemantic semantic;
    greal           extra;
};
struct _gShader
{
    xResource           parent;
    gShaderType         type;
    xstr                name;
    volatile xint       bound;
    xHashTable          *clients;  /* client -> struct client */
    xHashTable          *uniforms; /* sorted struct uniform */
    xsize               ibuf_size; /* int uniform buffer size */
    xsize               fbuf_size; /* float uniform buffer size */
    xsize               manual_ibuf_size;
    xsize               manual_fbuf_size;
    gProgram            *current;
    xPtrArray           *programs;
};
struct _gShaderClass
{
    xResourceClass      parent;
};
struct _gProgram
{
    xObject             parent;
    gShader             *shader;
    xbool               supported : 1;
};
struct _gProgramClass
{
    xObjectClass        parent;

    void (*prepare)(gProgram *program, xbool backThread);
    void (*load)   (gProgram *program, xbool backThread);
    void (*unload) (gProgram *program, xbool backThread);
    void (*bind)   (gProgram *program);
    void (*unbind) (gProgram *program);
    void (*upload) (gProgram *program, const gUniform *uniform, xcptr data);
};

xType       g_shader_type           (void);

xType       g_program_type          (void);

gProgram*   g_program_new           (xcstr          type,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;



X_END_DECLS
#endif /* __G_SHADER_H__ */
