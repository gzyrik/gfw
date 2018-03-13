#ifndef __G_TERRAIN_H__
#define __G_TERRAIN_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_TERRAIN              (g_terrain_type())
#define G_TERRAIN(object)           (X_INSTANCE_CAST((object), G_TYPE_TERRAIN, gTerrain))
#define G_TERRAIN_CLASS(klass)      (X_CLASS_CAST((klass), G_TYPE_TERRAIN, gTerrainClass))
#define G_IS_TERRAIN(object)        (X_INSTANCE_IS_TYPE((object), G_TYPE_TERRAIN))
#define G_TERRAIN_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_TERRAIN, gTerrainClass))

struct _gTerrain
{
    xObject         parent;
};

struct _gTerrainClass
{
    xObjectClass    parent;
}

xType       g_terrain_type          (void);

gTerrain*   g_terrain_get           (xcstr          name);

gTerrain*   g_terrain_new           (gScene         *scene,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

void        g_terrain_define        (gTerrain       *terrain,
                                     xint           x,
                                     xint           y,
                                     xcstr          first_property,
                                     ...) X_NULL_TERMINATED;

#define     g_terrain_build(terrain, x, y, height_data)                 \
    g_terrain_define (terrain, x, y, "height-data", height_data, NULL)

#define     g_terrain_import(terrain, x, y, height_map)                 \
    g_terrain_define (terrain, x, y, "height-map", height_map, NULL)

void        g_terrain_remove        (gTerrain       *terrain,
                                     xint           x,
                                     xint           y);
X_END_DECLS
#endif /* __G_TERRAIN_H__ */
