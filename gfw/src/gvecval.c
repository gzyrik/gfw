#include "config.h"
#include "gvecval.h"
#include "gfw-prv.h"
#include <string.h>
static xType *_param_types;
static void
value_init_vec          (xValue         *val)
{
    val->data[0].v_pointer = NULL;
}
static void
value_clear_vec         (xValue         *val)
{
    if (!(val->data[1].v_uint & X_VALUE_NOCOPY_CONTENTS))
        x_align_free (val->data[0].v_pointer);
}
static void
value_copy_vec          (const xValue   *src,
                         xValue         *dst)
{
    if (src->data[0].v_pointer) {
        dst->data[0].v_pointer = x_align_malloc0 (sizeof(gvec), G_VEC_ALIGN);
        memcpy (dst->data[0].v_pointer,
                src->data[0].v_pointer,
                sizeof(gvec));
    }
}
static xptr
value_peek_vec          (const xValue   *val)
{
    return val->data[0].v_pointer;
}
static xstr
value_collect_vec       (xValue         *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xsize i;
    xfloat *vec_f;

    if (n_collect_values > 4)
        return x_strdup_printf ("value location for `%s' must floats",
                                X_VALUE_TYPE_NAME (val)); 

    vec_f = x_align_malloc0 (sizeof(gvec), G_VEC_ALIGN);
    for (i=0; i<n_collect_values; ++i) {
        vec_f[i] = (xfloat)collect_values[i].v_double;
    }

    val->data[0].v_pointer = vec_f;
    return NULL;
}

static xstr
value_lcopy_vec         (const xValue   *val,
                         xsize          n_collect_values,
                         xCValue        *collect_values,
                         xuint          collect_flags)
{
    xsize i;
    const xfloat *vec_f;


    if (n_collect_values > 4)
        return x_strdup_printf ("value location for `%s' must floats",
                                X_VALUE_TYPE_NAME (val)); 

    vec_f = (xfloat*) val->data[0].v_pointer;

    for (i=0; i<n_collect_values; ++i) {
        xfloat *float_p = collect_values[i].v_pointer;
        if (!float_p)
            return x_strdup_printf ("value location for `%s' passed as NULL",
                                    X_VALUE_TYPE_NAME (val));
        *float_p = vec_f[i];
    }
    return NULL;
}

static void
param_vec_init          (xParam         *pspec)
{
    xsize i;
    gParamVec *vspec = (gParamVec*)pspec;

    for (i=0;i<4;++i) {
        vspec->minimum[i] = 0;
        vspec->maximum[i] = 0;
        vspec->default_value[i] = 0;
    }
}
#define param_vec_final NULL
static void
param_vec_default       (const xParam   *pspec,
                         xValue	        *value)
{
    xfloat *vec_f;
    gParamVec  *vspec;

    vspec = (gParamVec*)pspec;
    vec_f = (xfloat*) value->data[0].v_pointer;

    memcpy (vec_f, vspec->default_value, sizeof(xfloat)*4);
}

#define param_vec_validate  NULL
#define param_vec_cmp NULL

void
_g_value_types_init     (void)
{
    xType type;
    xTypeInfo info = {0,};
    xType *param_types;
    const xuint n_types = 23;
    xFundamentalInfo finfo = {X_TYPE_DERIVABLE, };
    xValueTable value_table = {
        value_init_vec,
        value_clear_vec,
        value_copy_vec,
        value_peek_vec,
        NULL,
        value_collect_vec,
        NULL,
        value_lcopy_vec,
    };
    xParamInfo pspec_info = {
        sizeof (gParamVec),
        param_vec_init,
        X_TYPE_INVALID,
        param_vec_final,
        param_vec_default,
        param_vec_validate,
        param_vec_cmp,
    };
    _param_types =  param_types = x_new0 (xType, n_types);
    /* G_TYPE_VEC3 */
    {
        value_table.collect_format = "ddd";
        value_table.lcopy_format = "ppp";
        info.value_table = &value_table;
        x_type_register_fundamental (G_TYPE_VEC3,
                                     x_intern_static_str ("gvec3"),
                                     &info, &finfo, 0);
    }
    /* G_TYPE_VEC4 */
    {
        value_table.collect_format = "dddd";
        value_table.lcopy_format = "pppp";
        info.value_table = &value_table;
        x_type_register_fundamental (G_TYPE_VEC4,
                                     x_intern_static_str ("gvec4"),
                                     &info, &finfo, 0);

    }
    /* G_TYPE_PARAM_VEC3 */
    {
        pspec_info.value_type = G_TYPE_VEC3;
        type = x_param_register_static (x_intern_static_str("gParamVec3"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == G_TYPE_PARAM_VEC3);
    }
    /* G_TYPE_PARAM_VEC4 */
    {
        pspec_info.value_type = G_TYPE_VEC4;
        type = x_param_register_static (x_intern_static_str("gParamVec4"),
                                        &pspec_info);
        *param_types++ = type;
        x_assert (type == G_TYPE_PARAM_VEC4);
    }
}
const xType*
g_param_types           (void)
{
    return _param_types;
}
xParam*
g_param_vec3_new        (xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         gvec           minimum,
                         gvec           maximum,
                         gvec           default_value,
                         xuint          flags)
{
    gParamVec  *vspec;

    x_return_val_if_fail (g_vec_ge_s (default_value, minimum)
                          && g_vec_le_s (default_value, maximum), NULL);

    vspec = x_param_new	(G_TYPE_PARAM_VEC3,
                         name, nick, blurb, flags);

    g_vec_store (minimum, vspec->minimum);
    g_vec_store (maximum, vspec->maximum);
    g_vec_store (default_value, vspec->default_value);

    return X_PARAM (vspec);
}
xParam*
g_param_vec4_new        (xcstr          name,
                         xcstr          nick,
                         xcstr          blurb,
                         gvec           minimum,
                         gvec           maximum,
                         gvec           default_value,
                         xuint          flags)
{
    gParamVec  *vspec;

    x_return_val_if_fail (g_vec_ge_s (default_value, minimum)
                          && g_vec_le_s (default_value, maximum), NULL);

    vspec = x_param_new	(G_TYPE_PARAM_VEC4,
                         name, nick, blurb, flags);

    g_vec_store (minimum, vspec->minimum);
    g_vec_store (maximum, vspec->maximum);
    g_vec_store (default_value, vspec->default_value);

    return X_PARAM (vspec);
}

gvec
g_value_get_vec         (const xValue   *val)
{
    gvec *ret;

    x_return_val_if_fail (G_VALUE_HOLDS_VEC3(val)
                          || G_VALUE_HOLDS_VEC4(val), G_VEC_0);

    ret = val->data[0].v_pointer;
    return  *ret;
}

void
g_value_set_vec         (xValue         *val,
                         const gvec     vec)
{
    gvec *ret;
    x_return_if_fail (G_VALUE_HOLDS_VEC3(val)
                      || G_VALUE_HOLDS_VEC4(val));

    ret = val->data[0].v_pointer;
    *ret = vec;
}
