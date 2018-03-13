#ifndef __G_MATERIAL_H__
#define __G_MATERIAL_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_MATERIAL             (g_material_type())
#define G_MATERIAL(object)          (X_INSTANCE_CAST((object), G_TYPE_MATERIAL, gMaterial))
#define G_MATERIAL_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_MATERIAL, gMaterialClass))
#define G_IS_MATERIAL(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_MATERIAL))
#define G_MATERIAL_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_MATERIAL, gMaterialClass))

#define G_TYPE_MATERIAL_TEMPLET             (g_material_type())
#define G_MATERIAL_TEMPLET(object)          (X_INSTANCE_CAST((object), G_TYPE_MATERIAL_TEMPLET, gMaterialTemplet))
#define G_MATERIAL_TEMPLET_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_MATERIAL_TEMPLET, gMaterialTempletClass))
#define G_IS_MATERIAL_TEMPLET(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_MATERIAL_TEMPLET))
#define G_MATERIAL_TEMPLET_GET_CLASS(object)(X_INSTANCE_GET_CLASS((object), G_TYPE_MATERIAL_TEMPLET, gMaterialTempletClass))


gMaterial*  g_material_attach       (xcstr          name);

void        g_material_detach       (gMaterial      *material);


gTechnique* g_material_technique    (gMaterial      *material);

xbool       g_material_is_3d        (gMaterial      *material);

gMaterial*  g_material_new          (xcstr          templet,
                                     xHashTable     *textures);

gTechnique* g_material_templet_add  (gMaterial      *material,
                                     xcstr          techniques_name);


struct _gTextureSourceIFace
{
    xIFace                      parent;
    gTexture* (*bind)    (gTextureSource *source, xptr user_data);

};
struct _gMaterialTemplet
{
    xResource           parent;
    /*< private >*/
    xstr                name;
    xbool               dirty;
    xPtrArray           *techniques;
};
struct _gMaterialTempletClass
{
    xResourceClass      parent;
};
struct _gMaterial
{
    xResource           parent;
    /*< private >*/
    xstr                name;
    gMaterialTamplate   *templet;
    xHashTable          *textures;
};
struct _gMaterialClass
{
    xResourceClass      parent;
};

xType       g_material_type         (void);
xType       g_material_templet_type (void);

X_END_DECLS
#endif /* __G_MATERIAL_H__ */
