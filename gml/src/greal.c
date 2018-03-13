#define __G_REAL_C__
#define X_IMPLEMENT_INLINES 1
#include "config.h"
#include "gtype.h"
#include "greal.h"
static xsize    _n_trigtable;
static greal    _trigtable_factor;
static greal    *_sin_table;
static greal    *_tan_table;
void
g_trigtable              (xsize          n_trigtable)
{
    if (_n_trigtable < n_trigtable) {
        xsize i;
        _trigtable_factor = n_trigtable/G_2PI;
        _n_trigtable = n_trigtable;
        _sin_table = x_realloc (_sin_table, sizeof(greal)*n_trigtable);
        _tan_table = x_realloc (_tan_table, sizeof(greal)*n_trigtable);
        for (i = 0; i < n_trigtable; ++i) {
            grad angle = G_2PI * i / n_trigtable;
            _sin_table[i] = sinf(angle);
            _tan_table[i] = tanf(angle);
        }
    }
}
greal
g_real_tan              (const grad     r)
{
    if (_tan_table) {
        int idx = (int)(r * _trigtable_factor) % _n_trigtable;
        return _tan_table[idx];
    }
    return tanf(r);
}
greal
g_real_sin                   (const grad     r)
{
    if (_sin_table) {
        int idx;
        if (r >= 0)
            idx = (int)(r * _trigtable_factor) % _n_trigtable;
        else
            idx = _n_trigtable - ((int)(-r * _trigtable_factor) % _n_trigtable) - 1;

        return _sin_table[idx];
    }
    return sinf(r);
}
