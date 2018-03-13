#include "gfw-d3d9.h"
#include <string.h>
typedef struct _D3d9Hlsl            D3d9Hlsl;
typedef struct _D3d9HlslClass       D3d9HlslClass;
#ifndef ID3DXBuffer_GetBufferPointer
#define ID3DXBuffer_GetBufferPointer(p) (p)->lpVtbl->GetBufferPointer(p)
#endif
#ifndef ID3DXConstantTable_GetDesc
#define ID3DXConstantTable_GetDesc(p, d) (p)->lpVtbl->GetDesc(p, d)
#define ID3DXConstantTable_GetConstant(p, d, i) (p)->lpVtbl->GetConstant(p, d, i)
#define ID3DXConstantTable_GetConstantDesc(p, h, d, i) (p)->lpVtbl->GetConstantDesc(p, h, d, i)
#endif

struct _D3d9Hlsl
{
    gProgram            parent;
    D3DXMACRO           *defines;
    LPD3DXBUFFER        micro_code;

    /* LPDIRECT3DVERTEXSHADER9, LPDIRECT3DPIXELSHADER9 */
    void                *handle;

    xstr                file;
    xstr                source;
    xsize               source_len;
    xstr                profile;
    xstr                entry;
    xbool               column_major;
    xbool               binary_source;
};
struct _D3d9HlslClass
{
    gProgramClass       parent;
};

X_DEFINE_DYNAMIC_TYPE (D3d9Hlsl, d3d9_hlsl,G_TYPE_PROGRAM);

xType
_d3d9_hlsl_register     (xTypeModule    *module)
{
    return d3d9_hlsl_register (module);
}
static void
d3d9_hlsl_preprocess    (D3d9Hlsl       *hlsl)
{

}

static void
d3d9_hlsl_prepare       (gProgram       *program,
                         xbool          backThread)
{
    D3d9Hlsl *hlsl = (D3d9Hlsl*)program;

    x_assert (program->shader != NULL);
    x_return_if_fail (program->supported);
    program->supported = FALSE;

    if (!hlsl->profile) {
        x_critical("program `%s' no hlsl profile", program->shader->name);
        return;
    }

    if (hlsl->file) {
        xFile *file = x_repository_open (X_RESOURCE(program->shader)->group, hlsl->file, NULL);
        if (!file) {
            x_critical("program `%s' can't open file %s",
                       program->shader->name, hlsl->file);
            return;
        }
        hlsl->source = x_file_read_all (file, &hlsl->source_len);
        x_object_unref (file);
    }
    if (!hlsl->source) {
        x_critical("program `%s' no hlsl source", program->shader->name);
        return;
    }

    if (!hlsl->binary_source) {
        d3d9_hlsl_preprocess (hlsl);
    }
    program->supported = TRUE;
}
/* D3D9 pads to 4 elements */
static 	void
populate_uniform        (D3DXCONSTANT_DESC* desc,
                         gUniform       *def)
{
    def->array_count = desc->Elements;
    switch(desc->Type) {
    case D3DXPT_INT:
        switch(desc->Columns) {
        case 1:
            def->type = G_UNIFORM_INT1;
            def->elem_size = 4*sizeof(int);
            break;
        case 2:
            def->type = G_UNIFORM_INT2;
            def->elem_size = 4*sizeof(int);
            break;
        case 3:
            def->type = G_UNIFORM_INT3;
            def->elem_size = 4*sizeof(int);
            break;
        case 4:
            def->type = G_UNIFORM_INT4;
            def->elem_size = 4*sizeof(int);
            break;
        }
        break;
    case D3DXPT_FLOAT:
        switch(desc->Class) {
        case D3DXPC_MATRIX_COLUMNS:
        case D3DXPC_MATRIX_ROWS: {
            int firstDim, secondDim;
            firstDim = desc->RegisterCount / desc->Elements;
            if (desc->Class == D3DXPC_MATRIX_ROWS)
                secondDim = desc->Columns;
            else
                secondDim = desc->Rows;
            switch(firstDim) {
            case 2:
                switch(secondDim) {
                case 2:
                    def->type = G_UNIFORM_MATRIX_2X2;
                    def->elem_size = 8*sizeof(float); /* HLSL always packs */
                    break;
                case 3:
                    def->type = G_UNIFORM_MATRIX_2X3;
                    def->elem_size = 8*sizeof(float); /* HLSL always packs */
                    break;
                case 4:
                    def->type = G_UNIFORM_MATRIX_2X4;
                    def->elem_size = 8*sizeof(float); 
                    break;
                } /* columns */
                break;
            case 3:
                switch(secondDim) {
                case 2:
                    def->type = G_UNIFORM_MATRIX_3X2;
                    def->elem_size = 12*sizeof(float); /* HLSL always packs */
                    break;
                case 3:
                    def->type = G_UNIFORM_MATRIX_3X3;
                    def->elem_size = 12*sizeof(float); /* HLSL always packs */
                    break;
                case 4:
                    def->type = G_UNIFORM_MATRIX_3X4;
                    def->elem_size = 12*sizeof(float); 
                    break;
                } /* columns */
                break;
            case 4:
                switch(secondDim) {
                case 2:
                    def->type = G_UNIFORM_MATRIX_4X2;
                    def->elem_size = 16*sizeof(float); /* HLSL always packs */
                    break;
                case 3:
                    def->type = G_UNIFORM_MATRIX_4X3;
                    def->elem_size = 16*sizeof(float); /* HLSL always packs */
                    break;
                case 4:
                    def->type = G_UNIFORM_MATRIX_4X4;
                    def->elem_size = 16*sizeof(float); 
                    break;
                } /* secondDim */
                break;

            } /* firstDim */
        }
            break;
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
            switch(desc->Columns) {
            case 1:
                def->type = G_UNIFORM_FLOAT1;
                def->elem_size = 4*sizeof(float);
                break;
            case 2:
                def->type = G_UNIFORM_FLOAT2;
                def->elem_size = 4*sizeof(float);
                break;
            case 3:
                def->type = G_UNIFORM_FLOAT3;
                def->elem_size = 4*sizeof(float);
                break;
            case 4:
                def->type = G_UNIFORM_FLOAT4;
                def->elem_size = 4*sizeof(float);
                break;
            } /* columns */
        }
        break;
    default:
        /* not mapping samplers, don't need to take the space */
        break;
    };
}
static void
recurse_register   (D3d9Hlsl       *hlsl,
                    LPD3DXCONSTANTTABLE pConstTable,
                    D3DXHANDLE     parent,
                    xcstr          prefix,
                    unsigned       index)
{
    D3DXHANDLE hConstant;
    D3DXCONSTANT_DESC desc = {0,};
    unsigned int numParams = 1;
    xchar *pStr, *pLocalName;
    xchar name[256];

    hConstant = ID3DXConstantTable_GetConstant (pConstTable, parent, index);
    TRACE_CALL (ID3DXConstantTable_GetConstantDesc,
                "%p, %p, %p, %p",
                (pConstTable, hConstant, &desc, &numParams));

    pLocalName = x_stpcpy (name, prefix);
    /* trim the odd '$' which appears at the start of the names in HLSL */
    if (desc.Name[0] == '$')
        strcpy (pLocalName, desc.Name+1);
    else
        strcpy (pLocalName, desc.Name);

    /* trim the '[0]' suffix, we will add our own indexing later */
    pStr = x_str_suffix (pLocalName, "[0]");
    if (pStr) *pStr = '\0';

    if (desc.Class == D3DXPC_STRUCT) {
        xsize i;
        strcat (pLocalName, ".");
        for (i=0; i<desc.StructMembers; ++i)
            recurse_register (hlsl, pConstTable, hConstant, name, i);
    }
    else {
        if (desc.Type == D3DXPT_FLOAT
            || desc.Type == D3DXPT_INT
            || desc.Type == D3DXPT_BOOL)
        {
            gUniform def;
            def.index = (xint)(desc.RegisterIndex);
            def.name = name;
            def.semantic = G_UNIFORM_MANUAL;
            def.extra = 0;
            populate_uniform(&desc, &def);
            g_shader_register (hlsl->parent.shader, &def);
        }
    }

}
static void
d3d9_hlsl_load          (gProgram       *program,
                         xbool          backThread)
{
    HRESULT hr;
    LPD3DXCONSTANTTABLE pConstTable = NULL;
    LPD3DXBUFFER errors = NULL;
    DWORD compileFlags = 0;
    ID3DXInclude includeHandler;

    D3d9Hlsl *hlsl = (D3d9Hlsl*)program;

    x_assert (program->shader != NULL);
    x_return_if_fail (program->supported);
    program->supported = FALSE;

#ifdef DEBUG
    compileFlags |= D3DXSHADER_DEBUG;
#endif
    if (hlsl->column_major)
        compileFlags |= D3DXSHADER_PACKMATRIX_COLUMNMAJOR;
    else
        compileFlags |= D3DXSHADER_PACKMATRIX_ROWMAJOR;

    x_assert (hlsl->source != NULL && hlsl->source_len > 0);
    if (hlsl->binary_source) {
        hr = D3DXAssembleShader(hlsl->source, hlsl->source_len,
                                NULL, NULL, 0,
                                &hlsl->micro_code, &errors);
    }
    else {
        hr = D3DXCompileShader (hlsl->source, hlsl->source_len,
                                hlsl->defines, NULL, hlsl->entry,
                                hlsl->profile, compileFlags,
                                &hlsl->micro_code, &errors,
                                &pConstTable);
    }
    if (FAILED(hr) || hlsl->micro_code == NULL || errors != NULL){
        xcstr str = "error";
        if(errors)
            str = ID3DXBuffer_GetBufferPointer (errors);
        x_critical ("program `%s' compile: %s",
                    program->shader->name, str);
        SAFE_RELEASE (errors);
        return;
    }
    x_free (hlsl->source);
    hlsl->source = NULL;
    hlsl->source_len = 0;
    if (pConstTable != NULL) {
        xsize i;
        D3DXCONSTANTTABLE_DESC desc = {0,};
        TRACE_CALL (ID3DXConstantTable_GetDesc,
                    "%p, %p", 
                    (pConstTable, &desc));

        for (i=0; i<desc.Constants; ++i)
            recurse_register (hlsl, pConstTable, NULL, "", i);

        SAFE_RELEASE (pConstTable);
    }

    switch (program->shader->type) {
    case G_SHADER_VERTEX:
        TRACE_CALL (IDirect3DDevice9_CreateVertexShader,
                    "%p, %p, %p",
                    (_d3d9_render->device,
                     ID3DXBuffer_GetBufferPointer (hlsl->micro_code),
                     (LPDIRECT3DVERTEXSHADER9*)&hlsl->handle));
        break;
    case G_SHADER_PIXEL:
        TRACE_CALL (IDirect3DDevice9_CreatePixelShader,
                    "%p, %p, %p",
                    (_d3d9_render->device,
                     ID3DXBuffer_GetBufferPointer (hlsl->micro_code),
                     (LPDIRECT3DPIXELSHADER9*)&hlsl->handle));
        break;
    }
    if (hlsl->handle)
        hlsl->parent.supported = TRUE;
}

static void
d3d9_hlsl_init          (D3d9Hlsl       *shader)
{
    G_PROGRAM(shader)->supported = TRUE;
}
/* hlsl program property */
enum { 
    PROP_0,
    PROP_FILE,
    PROP_ENTRY,
    PROP_PROFILE,
    N_PROPERTIES
};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         const xValue       *value,
                         xParam             *pspec)
{
    D3d9Hlsl *hlsl = (D3d9Hlsl*)object;

    switch(property_id) {
    case PROP_FILE:
        x_free (hlsl->file);
        hlsl->file = x_value_dup_str (value);
        break;
    case PROP_ENTRY:
        x_free (hlsl->entry);
        hlsl->entry = x_value_dup_str (value);
        break;
    case PROP_PROFILE:
        x_free (hlsl->profile);
        hlsl->profile = x_value_dup_str (value);
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    D3d9Hlsl *hlsl = (D3d9Hlsl*)object;
    switch(property_id) {
    case PROP_FILE:
        x_value_set_str (value, hlsl->file);
        break;
    case PROP_ENTRY:
        x_value_set_str (value, hlsl->entry);
        break;
    case PROP_PROFILE:
        x_value_set_str (value, hlsl->profile);
        break;
    }
}
static void
finalize                (xObject        *object)
{
    D3d9Hlsl *hlsl = (D3d9Hlsl*)object;

    x_free (hlsl->file);
    hlsl->file = NULL;
    x_free (hlsl->entry);
    hlsl->entry = NULL;
    x_free (hlsl->profile);
    hlsl->profile = NULL;
}
static void
d3d9_hlsl_bind         (gProgram        *program)
{
    D3d9Hlsl *hlsl = (D3d9Hlsl*)program;
    switch (program->shader->type) {
    case G_SHADER_VERTEX:
        TRACE_CALL (IDirect3DDevice9_SetVertexShader,
                    "%p, %p",
                    (_d3d9_render->device, hlsl->handle));
        break;
    case G_SHADER_PIXEL:
        TRACE_CALL (IDirect3DDevice9_SetPixelShader,
                    "%p, %p",
                    (_d3d9_render->device, hlsl->handle));
        break;
    }
}
static void
d3d9_hlsl_unbind       (gProgram        *program)
{
    D3d9Hlsl *hlsl = (D3d9Hlsl*)program;
    switch (program->shader->type) {
    case G_SHADER_VERTEX:
        TRACE_CALL (IDirect3DDevice9_SetVertexShader,
                    "%p, %p",
                    (_d3d9_render->device, NULL));
        break;
    case G_SHADER_PIXEL:
        TRACE_CALL (IDirect3DDevice9_SetPixelShader,
                    "%p, %p",
                    (_d3d9_render->device, NULL));
        break;
    }
}
static void
d3d9_hlsl_upload       (gProgram        *program,
                        const gUniform  *uniform,
                        xcptr           data)
{
    D3d9Hlsl *hlsl = (D3d9Hlsl*)program;
    switch (program->shader->type) {
    case G_SHADER_VERTEX:
        TRACE_CALL (IDirect3DDevice9_SetVertexShaderConstantF,
                    "%p, %p, %p, %d",
                    (_d3d9_render->device, (UINT)uniform->index,
                     data, uniform->elem_size*uniform->array_count/(4*sizeof(float))));
        break;
    case G_SHADER_PIXEL:
        TRACE_CALL (IDirect3DDevice9_SetPixelShaderConstantF,
                    "%p, %p, %p, %d",
                    (_d3d9_render->device, (UINT)uniform->index,
                     data, uniform->elem_size*uniform->array_count/(4*sizeof(float))));
        break;
    }
}

static void
d3d9_hlsl_class_init   (D3d9HlslClass  *klass)
{
    xParam *param;
    xObjectClass *oclass;
    gProgramClass *pclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->get_property = get_property;
    oclass->set_property = set_property;
    oclass->finalize     = finalize;

    pclass = G_PROGRAM_CLASS (klass);
    pclass->prepare = d3d9_hlsl_prepare;
    pclass->load = d3d9_hlsl_load;
    pclass->bind = d3d9_hlsl_bind;
    pclass->unbind = d3d9_hlsl_unbind;
    pclass->upload = d3d9_hlsl_upload;

    param = x_param_str_new ("file","file","file", NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_FILE,param);

    param = x_param_str_new ("entry","entry","entry", NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_ENTRY,param);

    param = x_param_str_new ("profile","profile","profile", NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_PROFILE,param);
}
static void
d3d9_hlsl_class_finalize(D3d9HlslClass  *klass)
{

}

