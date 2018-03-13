#ifndef __X_RESOURCE_H__
#define __X_RESOURCE_H__
#include "xtype.h"
X_BEGIN_DECLS

#define X_TYPE_RESOURCE              (x_resource_type())
#define X_RESOURCE(object)           (X_INSTANCE_CAST((object), X_TYPE_RESOURCE, xResource))
#define X_RESOURCE_CLASS(klass)      (X_CLASS_CAST((klass), X_TYPE_RESOURCE, xResourceClass))
#define X_IS_RESOURCE(object)        (X_INSTANCE_IS_TYPE((object), X_TYPE_RESOURCE))
#define X_IS_RESOURCE_CLASS(klass)   (X_CLASS_IS_TYPE((klass), X_TYPE_RESOURCE))
#define X_RESOURCE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), X_TYPE_RESOURCE, xResourceClass))

void        x_resource_prepare      (xptr           resource,
                                     xbool          backThread);

void        x_resource_load         (xptr           resource,
                                     xbool          backThread);

void        x_resource_unload       (xptr           resource,
                                     xbool          backThread);


void        x_repository_mount      (xcstr          group,
                                     xcstr          path,
                                     xArchive       *archive);

void        x_repository_unmount    (xcstr          group,
                                     xcstr          path);

void        x_repository_parse      (xcstr          group);

void        x_repository_prepare    (xcstr          group,
                                     xbool          backThread);

void        x_repository_load       (xcstr          group,
                                     xbool          backThread);

void        x_repository_unload     (xcstr          group,
                                     xbool          backThread);

xFile*      x_repository_open       (xcstr          group,
                                     xcstr          filepath,
                                     xcstr          first_property,
                                     ...);

void        x_set_script_priority   (xcstr          suffix,
                                     xint           priority);

typedef enum {
    X_RESOURCE_UNLOADED,
    X_RESOURCE_PREPARING,
    X_RESOURCE_PREPARED,
    X_RESOURCE_LOADING,
    X_RESOURCE_LOADED,
    X_RESOURCE_UNLOADING
} xResourceState;

struct _xResource
{
    xObject         parent;
    xstr            group;
    xint            state;
};
struct _xResourceClass
{
    xObjectClass    parent;
    xsize           order;
    void    (*prepare)  (xResource  *resource, xbool backThread);
    void    (*load)     (xResource  *resource, xbool backThread);
    void    (*unload)   (xResource  *resource, xbool backThread);
};
xType       x_resource_type         (void);

X_END_DECLS
#endif /* __X_RESOURCE_H__ */

