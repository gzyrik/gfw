#include "config.h"
#include "xarchive.h"
#include "xresource.h"
#include "xscript.h"
#include "xinit-prv.h"
#include <string.h>
enum GroupState {
    GROUP_UNLOADED,
    GROUP_PARSING,
    GROUP_PARSED,
    GROUP_PREPARING,
    GROUP_PREPARED,
    GROUP_LOADING,
    GROUP_LOADED,
    GROUP_UNLOADING
};
typedef struct {
    xstr        filename;
    xArchive    *archive;
    xsize       priority;
} FileInfo;

typedef struct {
    xHashTable      *archives; /* mount path -> archive ptr array */
    xHashTable      *files;    /* file path -> file info */
    xPtrArray       *resources;/* all resouce instance */
    xRWLock         lock;
    xint            state;
} ResourceGroup;

X_LOCK_DEFINE_STATIC    (_groups);
static xHashTable       *_groups;
static xHashTable       *_scripts;
X_DEFINE_TYPE (xResource, x_resource, X_TYPE_OBJECT);

static ResourceGroup*
res_group_get           (xcstr          group)
{
    xQuark  quark;

    quark  = x_quark_try (group);
    if (quark && _groups) {
        ResourceGroup* resGrp;

        X_LOCK (_groups);
        resGrp = x_hash_table_lookup (_groups, X_SIZE_TO_PTR(quark));
        X_UNLOCK (_groups);

        return   resGrp;
    }

    return NULL;
}
static void
res_group_free          (ResourceGroup  *resGrp)
{
}
static void
file_info_free (FileInfo *info)
{
    x_free(info->filename);
    x_free(info);
}
inline static ResourceGroup* res_group_new (xint state)
{
    ResourceGroup* resGrp;
    resGrp = x_slice_new0 (ResourceGroup);
    resGrp->archives = x_hash_table_new_full (x_str_hash, (xCmpFunc)x_str_cmp,
                                              x_free, (xFreeFunc)x_ptr_array_unref);
    resGrp->resources= x_ptr_array_new ();
    resGrp->files = x_hash_table_new_full (x_str_hash, (xCmpFunc)x_str_cmp,
                                           x_free, (xFreeFunc)file_info_free);
    resGrp->state = GROUP_UNLOADED;
    return resGrp;
}
static ResourceGroup* res_group_sure (xcstr group)
{
    ResourceGroup* resGrp;
    xQuark  quark;

    quark  = x_quark_from (group);

    X_LOCK (_groups);
    if (_groups)
        resGrp = x_hash_table_lookup (_groups, X_SIZE_TO_PTR(quark));
    else {
        _groups = x_hash_table_new_full (x_direct_hash, x_direct_cmp,
                                         NULL, (xFreeFunc)res_group_free);
        x_hash_table_insert (_groups, X_SIZE_TO_PTR(x_quark_from_static ("")),
                             res_group_new (GROUP_LOADED));
        resGrp = NULL;

    }
    if (!resGrp) {
        resGrp = res_group_new (GROUP_UNLOADED);
        x_hash_table_insert (_groups, X_SIZE_TO_PTR(quark), resGrp);
    }
    X_UNLOCK (_groups);

    return resGrp;
}
static xint
resource_order_cmp      (xcptr          a,
                         xcptr          b)
{
    xsize order1 = X_RESOURCE_GET_CLASS(a)->order;
    xsize order2 = X_RESOURCE_GET_CLASS(b)->order;
    if (order1 < order2)
        return -1;
    else if (order1 > order2)
        return 1;
    return X_PTR_TO_SIZE(a) - X_PTR_TO_SIZE(b);
}
static void
res_group_add_resource  (xcstr          group,
                         xResource      *resource)
{
    ResourceGroup   *resGrp;

    resGrp = group[0] == '0' ? res_group_sure(group) : res_group_get (group);
    x_return_if_fail (resGrp != NULL);

    x_rwlock_wlock (&resGrp->lock);
    x_ptr_array_insert_sorted (resGrp->resources, resource, resource,
                               (xCmpFunc)resource_order_cmp, FALSE);
    x_rwlock_unwlock (&resGrp->lock);
}

static void
res_group_remove_resource(xcstr         group,
                          xResource     *resource)
{
    ResourceGroup   *resGrp;

    resGrp = res_group_get (group);
    if (!resGrp) return;

    x_rwlock_wlock (&resGrp->lock);
    x_ptr_array_remove_data (resGrp->resources, resource);
    x_rwlock_unwlock (&resGrp->lock);
}

static void
resource_dispose        (xObject        *object)
{
    xResource       *res = (xResource*)object;

    if (res->group) {
        res_group_remove_resource (res->group, res);
        x_free (res->group);
        res->group = NULL;
    }

    X_OBJECT_CLASS(x_resource_parent_class)->dispose (object);
}
enum { PROP_0, PROP_GROUP, N_PROPERTIES};
static void
set_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    xResource       *res = (xResource*)object;
    xResourceClass  *klass = X_RESOURCE_GET_CLASS(res);

    switch(property_id) {
    case PROP_GROUP:
        if (res->group) {
            res_group_remove_resource (res->group, res);
            x_free (res->group);
        }
        res->group = x_value_dup_str (value);
        if (res->group) {
            res_group_add_resource (res->group, res);
        }
        break;
    }
}

static void
get_property            (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    xResource       *res = (xResource*)object;

    switch(property_id) {
    case PROP_GROUP:
        x_value_set_str (value, res->group);
        break;
    }
}
static void
x_resource_init         (xResource      *archive)
{
}

static void
x_resource_class_init   (xResourceClass *klass)
{
    xParam         *param;
    xObjectClass   *oclass = X_OBJECT_CLASS (klass);

    oclass->dispose      = resource_dispose;
    oclass->get_property = get_property;
    oclass->set_property = set_property;

    param = x_param_str_new ("group","group","group",NULL,
                             X_PARAM_STATIC_STR | X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_GROUP,param);
}
void
x_resource_prepare      (xptr           object,
                         xbool          backThread)
{
    xResource *resource;
    xResourceClass *klass;

    x_return_if_fail (X_IS_RESOURCE(object));
    resource = (xResource*)object;
    klass = X_RESOURCE_GET_CLASS(resource);
    x_return_if_fail (klass != NULL);

    if (!x_atomic_int_cas (&resource->state,
                           X_RESOURCE_UNLOADED,
                           X_RESOURCE_PREPARING))
        return;
    klass->prepare (resource, backThread);
    x_atomic_int_set (&resource->state, X_RESOURCE_PREPARED);
}

void
x_resource_load         (xptr           object,
                         xbool          backThread)
{
    xint state;
    xResource *resource;
    xResourceClass *klass;

    x_return_if_fail (X_IS_RESOURCE(object));
    resource = (xResource*)object;
    klass = X_RESOURCE_GET_CLASS(resource);
    x_return_if_fail (klass != NULL);

    state = x_atomic_int_get (&resource->state);
    if (state != X_RESOURCE_UNLOADED
        && state != X_RESOURCE_PREPARED)
        return;
    if (!x_atomic_int_cas (&resource->state,
                           state,
                           X_RESOURCE_LOADING))
        return;
    if (state == X_RESOURCE_UNLOADED)
        klass->prepare (resource, backThread);
    klass->load (resource, backThread);
    x_atomic_int_set (&resource->state, X_RESOURCE_LOADED);
}

void
x_resource_unload       (xptr           object,
                         xbool          backThread)
{
    xint state;
    xResource *resource;
    xResourceClass   *klass;

    x_return_if_fail (X_IS_RESOURCE(object));
    resource = (xResource*)object;
    klass = X_RESOURCE_GET_CLASS(resource);
    x_return_if_fail (klass != NULL);

    state = x_atomic_int_get (&resource->state);
    if (state != X_RESOURCE_LOADED
        && state != X_RESOURCE_PREPARED)
        return;
    if (!x_atomic_int_cas (&resource->state,
                           state,
                           X_RESOURCE_UNLOADING))
        return;
    klass->unload (resource, backThread);
    x_atomic_int_set (&resource->state, X_RESOURCE_UNLOADED);
}

void
x_repository_mount      (xcstr          group,
                         xcstr          path,
                         xArchive       *archive)
{
    xSList *files;
    xPtrArray *archivelist;
    ResourceGroup* resGrp;

    x_return_if_fail (path != NULL);
    x_return_if_fail (archive != NULL);

    resGrp = res_group_sure (group);
    x_return_if_fail (resGrp->state == GROUP_UNLOADED);

    x_rwlock_wlock (&resGrp->lock);
    archivelist = x_hash_table_lookup (resGrp->archives, path);
    if (!archivelist) {
        archivelist = x_ptr_array_new_full(4, (xFreeFunc)x_object_unref);
        x_hash_table_insert (resGrp->archives, x_strdup(path), archivelist);
    }
    x_ptr_array_append1(archivelist, archive);
    x_object_sink(archive);

    files = x_archive_list (archive);
    while (files) {
        FileInfo *fileinfo = x_slice_new0(FileInfo);
        xSList *next = files->next;

        fileinfo->filename =  files->data;
        fileinfo->archive = archive;

        x_hash_table_insert (resGrp->files, x_strcat(path,fileinfo->filename,NULL), fileinfo);
        x_slist_free1 (files);
        files = next;
    }
    x_rwlock_unwlock (&resGrp->lock);
}

void
x_repository_unmount    (xcstr          group,
                         xcstr          path)
{
    xPtrArray *archivelist;
    ResourceGroup   *resGrp;

    resGrp = res_group_get (group);
    x_return_if_fail (resGrp != NULL);
    x_return_if_fail (resGrp->state == GROUP_UNLOADED);

    x_rwlock_wlock (&resGrp->lock);
    archivelist = x_hash_table_lookup(resGrp->archives, path);
    if (archivelist) {
        xsize i;
        xstr filepath = x_malloc(1024*1024);
        for (i=0;i<archivelist->len;++i) {
            xSList *files;
            files = x_archive_list (archivelist->data[i]);
            while (files) {
                xSList *next = files->next;
                sprintf(filepath, "%s/%s", path, (xcstr)(files->data));
                x_hash_table_remove (resGrp->files, filepath);
                x_slist_free1 (files);
                files = next;
            }
        }
        x_free(filepath);
        x_hash_table_remove(resGrp->archives, (xptr)path);
    }
    x_rwlock_unwlock (&resGrp->lock);
}
static xint
file_info_cmp (FileInfo *pA, FileInfo *pB)
{
    if (pA->priority < pB->priority)
        return -1;
    else if (pA->priority > pB->priority)
        return 1;
    else
        return strcmp (pA->filename, pB->filename);
}
static void
find_script_files       (xcstr          filenpath,
                         FileInfo       *fileinfo,
                         xPtrArray      *scripts)
{
    xcstr suffix;
    xptr priority;

    if ((suffix = strchr (fileinfo->filename, '.')) == NULL)
        return;
    if ((priority = x_hash_table_lookup(_scripts,suffix+1)) == NULL)
        return;
    fileinfo->priority = X_PTR_TO_SIZE(priority);
    x_ptr_array_insert_sorted (scripts, fileinfo, fileinfo,
                               (xCmpFunc)file_info_cmp, FALSE);
}
static void
group_parse             (ResourceGroup  *resGrp,
                         xcstr          group)
{
    xPtrArray       *files;
    xsize           i;

    x_rwlock_rlock (&resGrp->lock);
    files = x_ptr_array_new_full (x_hash_table_size(resGrp->files), NULL);
    x_hash_table_foreach (resGrp->files,
                          X_CALLBACK(find_script_files),
                          files);
    x_rwlock_unrlock (&resGrp->lock);

    for (i = 0; i < files->len; ++i) {
        xFile *file;
        FileInfo *info;

        info = files->data[i];
        x_debug("parsing '%s'", info->filename);
        file = x_archive_open (info->archive, info->filename, NULL);
        if (file)
            x_script_import_file (file, group, NULL);
        else
            x_warning ("file '%s' has removed",info->filename);
        x_object_unref (file);
    }
    x_ptr_array_unref (files);
}

void
x_set_script_priority   (xcstr          suffix,
                         xint           priority)
{
    x_return_if_fail (suffix != 0);
    x_return_if_fail (priority != 0);

    if ( !x_atomic_ptr_get (&_scripts)
         && x_once_init_enter (&_scripts)) {
        _scripts = x_hash_table_new_full (x_str_hash, (xCmpFunc)x_str_cmp,
                                          x_free, NULL);
        x_once_init_leave (&_scripts); 
    }

    x_hash_table_insert (_scripts,
                         x_strdup (suffix),
                         X_SIZE_TO_PTR(priority));
}

void
x_repository_parse      (xcstr          group)
{
    ResourceGroup   *resGrp;

    resGrp = res_group_get (group);
    x_return_if_fail (resGrp != NULL);

    if (!x_atomic_int_cas (&resGrp->state,
                           GROUP_UNLOADED,
                           GROUP_PARSING))
        return;

    if (_scripts)
        group_parse (resGrp, group);
    resGrp->state = GROUP_PARSED;
}
void
x_repository_prepare    (xcstr          group,
                         xbool          backThread)
{
    xint state;
    xPtrArray   *resources;
    ResourceGroup   *resGrp;

    resGrp = res_group_get (group);
    x_return_if_fail (resGrp != NULL);

    state = x_atomic_int_get (&resGrp->state);
    if (state != GROUP_UNLOADED
        && state != GROUP_PARSED)
        return;
    if (!x_atomic_int_cas (&resGrp->state,
                           state,
                           GROUP_PREPARING))
        return;
    if (state == GROUP_UNLOADED)
        group_parse (resGrp, group);

    x_rwlock_rlock (&resGrp->lock);
    resources = x_ptr_array_dup(resGrp->resources);
    x_rwlock_unrlock (&resGrp->lock);

    x_ptr_array_foreach (resources,
                         X_CALLBACK(x_resource_prepare),
                         X_SIZE_TO_PTR(backThread));

    x_ptr_array_unref (resources);
    resGrp->state = GROUP_PREPARED;
}

void
x_repository_load       (xcstr          group,
                         xbool          backThread)
{
    xint state;
    xPtrArray   *resources;
    ResourceGroup   *resGrp;

    resGrp = res_group_get (group);
    x_return_if_fail (resGrp != NULL);

    state = x_atomic_int_get (&resGrp->state);
    if (state != GROUP_UNLOADED
        && state != GROUP_PARSED
        && state != GROUP_PREPARED)
        return;
    if (!x_atomic_int_cas (&resGrp->state,
                           state,
                           GROUP_LOADING))
        return;
    if (state <= GROUP_UNLOADED)
        group_parse (resGrp, group);


    x_rwlock_rlock (&resGrp->lock);
    resources = x_ptr_array_dup(resGrp->resources);
    x_rwlock_unrlock (&resGrp->lock);


    if (state <= GROUP_PARSED) 
        x_ptr_array_foreach (resources,
                             X_CALLBACK(x_resource_prepare),
                             X_SIZE_TO_PTR(backThread));

    x_ptr_array_foreach (resources,
                         X_CALLBACK(x_resource_load),
                         X_SIZE_TO_PTR(backThread));

    x_ptr_array_unref (resources);
    resGrp->state = GROUP_LOADED;
}

void
x_repository_unload     (xcstr          group,
                         xbool          backThread)
{
    xint state;
    xPtrArray   *resources;
    ResourceGroup   *resGrp;

    resGrp = res_group_get (group);
    x_return_if_fail (resGrp != NULL);

    state = x_atomic_int_get (&resGrp->state);
    if (state != GROUP_LOADED
        && state != GROUP_PARSED
        && state != GROUP_PREPARED)
        return;
    if (!x_atomic_int_cas (&resGrp->state,
                           state,
                           GROUP_UNLOADING))
        return;

    x_rwlock_rlock (&resGrp->lock);
    resources = x_ptr_array_dup(resGrp->resources);
    x_rwlock_unrlock (&resGrp->lock);

    x_ptr_array_foreach (resources,
                         X_CALLBACK(x_resource_unload),
                         X_SIZE_TO_PTR(backThread));

    x_ptr_array_unref (resources);
    resGrp->state = GROUP_UNLOADED;
}
static xbool
find_file(xcstr filepath, FileInfo *info, xptr result[2])
{
    if (x_str_suffix(filepath, result[0])) {
        result[1] = info;
        return FALSE;
    }
    return TRUE;
}
xFile*
x_repository_open       (xcstr          group,
                         xcstr          filepath,
                         xcstr          first_property,
                         ...)
{
    ResourceGroup   *resGrp;
    FileInfo        *fileinfo = NULL;
    xFile           *file = NULL;

    resGrp = res_group_get (group);
    x_return_val_if_fail (resGrp != NULL, NULL);


    x_rwlock_rlock (&resGrp->lock);
    if (filepath[0] == '/')
        fileinfo = x_hash_table_lookup (resGrp->files, filepath);
    else {
        xptr result[2] = { (xptr)filepath, NULL };
        x_hash_table_walk (resGrp->files, X_WALKFUNC(find_file), result);
        fileinfo = result[1];
    }
    x_rwlock_unrlock (&resGrp->lock);

    if (fileinfo) {
        va_list         argv;
        va_start (argv, first_property);
        file = x_archive_open_valist (fileinfo->archive, fileinfo->filename, first_property, argv);
        va_end (argv);
    }
    else {
        x_warning ("can't open file '%s' at `%s'",filepath, group);
    }

    return file;
}
