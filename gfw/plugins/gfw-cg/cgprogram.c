#define X_LOG_DOMAIN                "gfw-cg"
#include <gfw.h>
#include <string.h>
#include <Cg/cg.h>
#include <Cg/cgD3D9.h>
#pragma comment(lib, "cg.lib cgD3D9.lib cgGL.lib")

#define CHECK_CG_ERROR(context) X_STMT_START{                           \
    CGerror error = cgGetError ();                                      \
    if (error != CG_NO_ERROR){                                          \
        x_debug ("cg error: %s",  cgGetErrorString (error));            \
        if (error == CG_COMPILER_ERROR) {                               \
            x_critical ("cg compiler error: %s",                        \
                        cgGetLastListing (context));                    \
        }                                                               \
    }                                                                   \
} X_STMT_END

typedef struct _cgProgram            cgProgram;
typedef struct _cgProgramClass       cgProgramClass;
struct vsepc_index
{
    gVertexSpec         spec;
    unsigned long       index;
};
struct _cgProgram
{
    gProgram            parent;
    CGprogram           handle;
    CGprofile           profile;

    xstr                file;
    xstr                source;
    xstrv               profiles;
    xstr                entry;

    char**              cg_argv;
    xArray              *vindex; /* sorted struct vsepc_index */
    xbool               binary_source;
    xbool               loaded;
};
struct _cgProgramClass
{
    gProgramClass       parent;
};
static CGcontext        _context;

X_DEFINE_DYNAMIC_TYPE (cgProgram, cg_program,G_TYPE_PROGRAM);

xbool X_MODULE_EXPORT
type_module_main        (xTypeModule    *module)
{
    _context = cgCreateContext();
    if (_context != NULL) {
        cgSetParameterSettingMode(_context, CG_DEFERRED_PARAMETER_SETTING);
        return cg_program_register (module);
    }
    return FALSE;
}

static xint
cg_vindex_cmp           (gVertexSpec    *s1,
                         gVertexSpec    *s2)
{
    if (s1->semantic < s2->semantic)
        return -1;
    else if (s1->semantic > s2->semantic)
        return 1;
    else if (s1->index < s2->index)
        return -1;
    else
        return s1->index > s2->index;
}
static void
cg_program_preprocess   (cgProgram       *cg)
{
    
}
static void
populate_uniform        (CGtype         cgType,
                         xbool          isRegisterCombiner, 
                         gUniform       *def)
{
    if (isRegisterCombiner) {
        /* register combiners are the only single-float entries in our buffer */
        def->type = G_UNIFORM_FLOAT1;
        def->elem_size = sizeof(float);
    }
    else {
        switch(cgType) {
        case CG_FLOAT:
        case CG_FLOAT1:
        case CG_HALF:
        case CG_HALF1:
            def->type = G_UNIFORM_FLOAT1;
            def->elem_size = 4*sizeof(float); /* padded to 4 elements */
            break;
        case CG_FLOAT2:
        case CG_HALF2:
            def->type = G_UNIFORM_FLOAT2;
            def->elem_size = 4*sizeof(float); /* padded to 4 elements */
            break;
        case CG_FLOAT3:
        case CG_HALF3:
            def->type = G_UNIFORM_FLOAT3;
            def->elem_size = 4*sizeof(float); /* padded to 4 elements */
            break;
        case CG_FLOAT4:
        case CG_HALF4:
            def->type = G_UNIFORM_FLOAT4;
            def->elem_size = 4*sizeof(float); 
            break;
        case CG_FLOAT2x2:
        case CG_HALF2x2:
            def->type = G_UNIFORM_MATRIX_2X2;
            def->elem_size = 8*sizeof(float); /* Cg pads this to 2 float4s */
            break;
        case CG_FLOAT2x3:
        case CG_HALF2x3:
            def->type = G_UNIFORM_MATRIX_2X3;
            def->elem_size = 8*sizeof(float); /* Cg pads this to 2 float4s */
            break;
        case CG_FLOAT2x4:
        case CG_HALF2x4:
            def->type = G_UNIFORM_MATRIX_2X4;
            def->elem_size = 8*sizeof(float); 
            break;
        case CG_FLOAT3x2:
        case CG_HALF3x2:
            def->type = G_UNIFORM_MATRIX_2X3;
            def->elem_size = 12*sizeof(float); /* Cg pads this to 3 float4s */
            break;
        case CG_FLOAT3x3:
        case CG_HALF3x3:
            def->type = G_UNIFORM_MATRIX_3X3;
            def->elem_size = 12*sizeof(float); /* Cg pads this to 3 float4s */
            break;
        case CG_FLOAT3x4:
        case CG_HALF3x4:
            def->type = G_UNIFORM_MATRIX_3X4;
            def->elem_size = 12*sizeof(float); 
            break;
        case CG_FLOAT4x2:
        case CG_HALF4x2:
            def->type = G_UNIFORM_MATRIX_4X2;
            def->elem_size = 16*sizeof(float); /* Cg pads this to 4 float4s */
            break;
        case CG_FLOAT4x3:
        case CG_HALF4x3:
            def->type = G_UNIFORM_MATRIX_4X3;
            def->elem_size = 16*sizeof(float); /* Cg pads this to 4 float4s */
            break;
        case CG_FLOAT4x4:
        case CG_HALF4x4:
            def->type = G_UNIFORM_MATRIX_4X4;
            def->elem_size = 16*sizeof(float); /* Cg pads this to 4 float4s */
            break;
        case CG_INT:
        case CG_INT1:
            def->type = G_UNIFORM_INT1;
            def->elem_size = 4*sizeof(int); /* Cg pads this to int4 */
            break;
        case CG_INT2:
            def->type = G_UNIFORM_INT2;
            def->elem_size = 4*sizeof(int); /* Cg pads this to int4 */
            break;
        case CG_INT3:
            def->type = G_UNIFORM_INT3;
            def->elem_size = 4*sizeof(int); /* Cg pads this to int4 */
            break;
        case CG_INT4:
            def->type = G_UNIFORM_INT4;
            def->elem_size = 4*sizeof(int); 
            break;
        default:
            def->type = G_UNIFORM_VOID;
            break;
        }
    }
}
static void
recurse_register        (cgProgram       *cg,
                         CGparameter    parameter,
                         CGparameter    parent,
                         xsize          contextArraySize)
{
    gUniform def;
    xchar paramName[256];
    xstr  str;

    while (parameter != 0) {
        CGtype paramType = cgGetParameterType(parameter);
        CGenum var = cgGetParameterVariability(parameter);
        CGenum dir = cgGetParameterDirection(parameter);
        if(var == CG_VARYING && dir != CG_OUT) {
            gVertexSpec attrib;
            xcstr semantic = (xcstr)cgGetParameterSemantic(parameter);
            xcstr name;
            attrib = g_vertex_guess_spec (semantic);
            if (attrib.semantic == G_VERTEX_NONE){ /* 再从变量名去推测其语义 */
                name = (xcstr)cgGetParameterName(parameter);
                attrib = g_vertex_guess_spec (name);
            }
            if(attrib.semantic != G_VERTEX_NONE) {
                xsize i = x_array_insert_sorted(cg->vindex, &attrib, NULL,
                                                (xCmpFunc)cg_vindex_cmp, FALSE);
                struct vsepc_index *vindex = &x_array_index (cg->vindex, struct vsepc_index, i);
                vindex->spec = attrib;
                vindex->index = cgGetParameterResourceIndex(parameter);
            }
            else {
                x_warning ("CgProgram Warning: invalid param '%s': %s",
                           name, semantic);
            }
        }
        else if (var == CG_UNIFORM &&
                 paramType != CG_SAMPLER1D &&
                 paramType != CG_SAMPLER2D &&
                 paramType != CG_SAMPLER3D &&
                 paramType != CG_SAMPLERCUBE &&
                 paramType != CG_SAMPLERRECT &&
                 dir != CG_OUT && 
                 cgIsParameterReferenced(parameter)) {
            xsize arraySize;
            unsigned long logicalIndex;
            unsigned long regCombinerPhysicalIndex;
            xbool isRegisterCombiner;
            CGresource res;

            switch(paramType) {
            case CG_STRUCT:
                recurse_register (cg, cgGetFirstStructParameter(parameter), parameter,1);
                break;
            case CG_ARRAY:
                /* Support only 1-dimensional arrays */
                arraySize = cgGetArraySize(parameter, 0);
                recurse_register (cg,cgGetArrayParameter(parameter, 0),
                                  parameter,arraySize);
                break;
            default:
                /* Normal path (leaf) */
                strcpy (paramName, cgGetParameterName(parameter));
                logicalIndex = cgGetParameterResourceIndex(parameter);
                /* Get the parameter resource, to calculate the physical index */
                res = cgGetParameterResource(parameter);
                isRegisterCombiner = FALSE;
                regCombinerPhysicalIndex = 0;
                switch (res) {
                case CG_COMBINER_STAGE_CONST0:
                    /* register combiner, const 0
                       the index relates to the texture stage; store this as (stage * 2) + 0 */
                    regCombinerPhysicalIndex = logicalIndex * 2;
                    isRegisterCombiner = TRUE;
                    break;
                case CG_COMBINER_STAGE_CONST1:
                    /* register combiner, const 1
                       the index relates to the texture stage; store this as (stage * 2) + 1 */
                    regCombinerPhysicalIndex = (logicalIndex * 2) + 1;
                    isRegisterCombiner = TRUE;
                    break;
                default:
                    /* normal constant */
                    break;
                }

                /* Trim the '[0]' suffix if it exists, we will add our own indexing later */
                str = x_str_suffix (paramName, "[0]");
                if (str) *str = '\0';
     

                def.array_count = contextArraySize;
                populate_uniform (paramType, isRegisterCombiner, &def);

                if (def.type == G_UNIFORM_VOID) {
                    x_warning ("Problem parsing the following Cg Uniform: '%s' in file ", paramName);
                    /* next uniform */
                    parameter = cgGetNextParameter(parameter);
                    continue;
                }
                if(contextArraySize > 1 && parent)
                    logicalIndex = (size_t)parent;
                else
                    logicalIndex = (size_t)parameter;
                def.index = (xint)(logicalIndex);
                def.name = paramName;
                def.semantic = G_UNIFORM_MANUAL;
                def.extra = 0;
                g_shader_register (cg->parent.shader, &def);
                break;
            }
        }
        parameter = cgGetNextParameter(parameter);
    }
}
static void
cg_program_prepare      (gProgram       *program,
                         xbool          backThread)
{
    xsize i;
    cgProgram *cg = (cgProgram*)program;

    x_assert (program->shader != NULL);
    x_return_if_fail (program->supported);
    program->supported = FALSE;

    if (!cg->profiles) {
        x_critical("program `%s' no cg profiles", program->shader->name);
        return;
    }

    i=0;
    cg->profile = CG_PROFILE_UNKNOWN;
    while (cg->profiles[i]) {
        /*if (g_render_support_profile (program->parent.render,profiles[i])) */{
            cg->profile = cgGetProfile (cg->profiles[i]);
            break;
        }
        ++i;
    }
    if (cg->profile == CG_PROFILE_UNKNOWN) {
        x_critical("program `%s' no supported cg profile", program->shader->name);
        return;
    }
    if (cg->file) {
        xFile *file = x_res_group_open (X_RESOURCE(program->shader)->group, cg->file, NULL);
        if (!file) {
            x_critical("program `%s' can't open file %s",
                       program->shader->name, cg->file);
            return;
        }
        cg->source = x_file_read_all (file, NULL);
        x_object_unref (file);
    }
    if (!cg->binary_source) {
        cg_program_preprocess (cg);
    }
    program->supported = TRUE;
}
static LPDIRECT3DDEVICE9 _d3dDevice9 = NULL;
static void
cg_program_load         (gProgram       *program,
                         xbool          backThread)
{
    cgProgram *cg = (cgProgram*)program;

    x_assert (program->shader != NULL);
    x_return_if_fail (program->supported);
    program->supported = FALSE;

    x_return_if_fail (cg->source != NULL);

    x_assert (cg->source != NULL);
    cg->handle = cgCreateProgram (_context,
                                  cg->binary_source ? CG_OBJECT : CG_SOURCE,
                                  cg->source, cg->profile,
                                  cg->entry, cg->cg_argv);
    x_free (cg->source);
    cg->source = NULL;

    if (!cg->handle) {
        CGerror error = cgGetError ();
        x_critical ("program `%s' compile: %s, %s",
                    program->shader->name,
                    cgGetErrorString (error),
                    cgGetLastListing (_context));
        return;
    }
    else {
        CHECK_CG_ERROR (_context);
    }

    cg->vindex = x_array_new (sizeof(struct vsepc_index));
    recurse_register (cg,cgGetFirstParameter(cg->handle, CG_PROGRAM),0,1);
    recurse_register (cg,cgGetFirstParameter(cg->handle, CG_GLOBAL),0,1);

    if (!_d3dDevice9) {
        HRESULT hr;
        g_render_get ("device", &_d3dDevice9, NULL);
        hr = cgD3D9SetDevice(_d3dDevice9);
        if FAILED (hr)
            x_error ("cgD3D9SetDevice:%s",cgD3D9TranslateHRESULT(hr));
    }
    cg->parent.supported = TRUE;
}

static void
cg_program_init         (cgProgram       *shader)
{
    G_PROGRAM (shader)->supported = TRUE;
}
/* cg program property */
enum { 
    PROP_0,
    PROP_FILE,
    PROP_ENTRY,
    PROP_PROFILES,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    cgProgram *cg = (cgProgram*)object;

    switch(property_id) {
    case PROP_FILE:
        x_free (cg->file);
        cg->file = x_value_dup_str (value);
        break;
    case PROP_ENTRY:
        x_free (cg->entry);
        cg->entry = x_value_dup_str (value);
        break;
    case PROP_PROFILES:
        x_strv_free(cg->profiles);
        cg->profiles = x_strv_dup ((xcstrv)x_value_get_boxed (value));
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    cgProgram *cg = (cgProgram*)object;
    switch(property_id) {
    case PROP_FILE:
        x_value_set_str (value, cg->file);
        break;
    case PROP_ENTRY:
        x_value_set_str (value, cg->entry);
        break;
    case PROP_PROFILES:
        x_value_set_boxed (value, cg->profiles);
        break;
    }
}
static void
finalize                (xObject        *object)
{
    cgProgram *cg = (cgProgram*)object;

    x_free (cg->file);
    cg->file = NULL;
    x_free (cg->entry);
    cg->entry = NULL;
    x_strv_free (cg->profiles);
    cg->profiles = NULL;
}
static void
cg_program_bind         (gProgram        *program)
{
    HRESULT hr;
    cgProgram *cg = (cgProgram*)program;

    if (!cg->loaded) {
        hr = cgD3D9LoadProgram(cg->handle, FALSE, 0);
        if (FAILED(hr))
            x_critical ("program `%s': cgD3D9LoadProgram:%s",
                        program->shader->name, cgD3D9TranslateHRESULT(hr));
        else
            cg->loaded = TRUE;
    }
    if (cg->loaded) {
        hr = cgD3D9BindProgram(cg->handle);
        if(FAILED(hr))
            x_critical ("program `%s': cgD3D9BindProgram:%s",
                        program->shader->name, cgD3D9TranslateHRESULT(hr));
    }
}
static void
cg_program_unbind       (gProgram        *program)
{
    cgProgram *cg = (cgProgram*)program;
    HRESULT hr = cgD3D9UnbindProgram(cg->handle);
    if(FAILED(hr))
        x_critical ("cgD3D9UnbindProgram:%s",cgD3D9TranslateHRESULT(hr));
}
static void
cg_program_upload       (gProgram        *program,
                         const gUniform  *uniform,
                         xcptr           data)
{
   cgProgram *cg = (cgProgram*)program;
   HRESULT hr;

   hr =cgD3D9SetUniform((CGparameter)uniform->index, (const void*)data);
   if(FAILED(hr))
       x_critical("cgD3D9SetUniform:%s", cgD3D9TranslateHRESULT(hr));
}

static void
cg_program_class_init   (cgProgramClass  *klass)
{
    xParam *param;
    xObjectClass *oclass;
    gProgramClass *pclass;


    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = finalize;

    pclass = G_PROGRAM_CLASS (klass);
    pclass->prepare = cg_program_prepare;
    pclass->load = cg_program_load;
    pclass->bind = cg_program_bind;
    pclass->unbind = cg_program_unbind;
    pclass->upload = cg_program_upload;

    param = x_param_str_new ("file","file","file", NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_FILE,param);

    param = x_param_str_new ("entry","entry","entry", NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_ENTRY,param);

    param = x_param_strv_new ("profiles","profiles","profiles",
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_PROFILES,param);
}
static void
cg_program_class_finalize(cgProgramClass  *klass)
{

}
