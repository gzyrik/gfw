#include "config.h"
#include "xarchive.h"
#include <string.h>
#include <ctype.h>
struct _xCache
{
    xbyte               *buf;
    xsize               pos;
    xsize               len;
    xbool               eof;
    xbool               flip_endian;
    xsize               count;
    xsize               mark;
};

X_DEFINE_TYPE_EXTENDED (xFile, x_file, X_TYPE_OBJECT, X_TYPE_ABSTRACT, );
static void
x_file_init             (xFile          *archive)
{
}
static void
x_file_class_init       (xFileClass     *klass)
{
}
xsize
x_file_get              (xFile          *file,
                         xptr           buf,
                         xsize          size,
                         xsize          count)
{
    xsize len;
    xFileClass  *klass;

    x_return_val_if_fail (X_IS_FILE (file), 0);
    x_return_val_if_fail (size>=1 && count>0, 0);

    klass = X_FILE_GET_CLASS (file);

    len = klass->read (file, buf, size*count);
    if (file->flip_endian && size>1 && len>size)
        x_flip_endian (buf,size,len/size);
    return len;
}
void
x_file_set_flip_endian  (xFile      *file,
                         xbool      flip)
{
    x_return_if_fail (X_IS_FILE (file));

    file->flip_endian = flip;
}
xbool
x_file_get_flip_endian  (xFile          *file)
{
    x_return_val_if_fail (X_IS_FILE (file), FALSE);

    return file->flip_endian;
}
xsize
x_file_put              (xFile          *file,
                         xptr           buf,
                         xsize          size,
                         xsize          count)
{
    xFileClass  *klass;

    x_return_val_if_fail (X_IS_FILE (file), 0);
    x_return_val_if_fail (size>= 1 && count>0, 0);

    klass = X_FILE_GET_CLASS (file);

    if (file->flip_endian && size>1)
        x_flip_endian (buf,size,count);
    return klass->write (file, buf, size*count);
}
xsize
x_file_putc             (xFile          *file,
                         xchar          data)
{
    return x_file_put (file, &data, sizeof(xchar), 1);
}
xsize
x_file_puti16           (xFile          *file,
                         xint16         data)
{
    return x_file_put (file, &data, sizeof(xint16), 1);
}
xsize
x_file_puti32           (xFile          *file,
                         xint32         data)
{
    return x_file_put (file, &data, sizeof(xint32), 1);
}
xsize
x_file_putf32           (xFile          *file,
                         xfloat         data)
{
    return x_file_put (file, &data, sizeof(xfloat32), 1);
}
xsize
x_file_puts             (xFile          *file,
                         xcstr          str,
                         xbool          zero)
{
    xchar c = '\0';
    xsize count = zero ? 1 : 0;
    if (str) {
        count += strlen (str);
        count = x_file_put (file, (xptr)str, 1, count);
    }
    else if (zero) {
        count = x_file_put (file, &c, 1, 1);
    }
    return count;
}

xsize
x_file_print            (xFile          *file,
                         xbool          zero,
                         xcstr          format,
                         ...)
{
    va_list args;
    xsize ret;

    va_start (args, format);
    ret = x_file_print_valist(file, zero, format, args);
    va_end(args);

    return ret;
}

xsize
x_file_print_valist     (xFile          *file,
                         xbool          zero,
                         xcstr          format,
                         va_list        args)
{
    xchar c = '\0';
    xsize count = zero ? 1 : 0;
    if (format) {
        xstr str = x_strdup_vprintf (format, args);
        count += strlen (str);
        count = x_file_put (file, (xptr)str, 1, count);
        x_free(str);
    }
    else if (zero) {
        count = x_file_put (file, &c, 1, 1);
    }
    return count;
}

xbool
x_file_seek             (xFile          *file,
                         xint           offset,
                         xSeekOrigin    origin)
{
    xFileClass  *klass;

    x_return_val_if_fail (X_IS_FILE (file), FALSE);

    klass = X_FILE_GET_CLASS (file);

    return klass->seek (file, offset, origin);
}

xsize
x_file_tell            (xFile          *file)
{
    xFileClass  *klass;

    x_return_val_if_fail (X_IS_FILE (file), 0);

    klass = X_FILE_GET_CLASS (file);

    return klass->tell (file);
}

xbyte*
x_file_read_all         (xFile          *file,
                         xsize          *outlen)
{
    xbyte *ret = NULL;
    xsize alloc=512, offset=0, count=0;

    x_return_val_if_fail (file != NULL, 0);
    do {
        alloc *= 2;
        offset += count;
        ret = x_realloc (ret, alloc);
        count = x_file_get (file, ret+offset, 1, alloc-offset);
    } while (count == alloc-offset);

    if (outlen)
        *outlen = offset + count;
    ret[offset + count] = '\0';
    return ret;
}
struct  RealCache
{
    xbyte               *buf;
    xsize               pos;
    xsize               len;
    xbool               eof;
    xbool               flip_endian;
    xsize               count;
    void  (*fill)       (xCache*);
    void  (*clear)      (xCache*);
};
struct FileCache
{
    struct RealCache    cache;
    xFile               *file;
    xsize               buf_size;
} ;

static void
mem_cache_fill          (xCache         *cache)
{
    cache->eof = TRUE;
}
static void
file_cache_fill         (xCache         *cache)
{
    struct FileCache  *fc = (struct FileCache*)cache;

    xsize avail = cache->len - cache->pos;
    /* move avail to the head */
    if (avail != 0)
        memcpy (cache->buf, cache->buf+cache->pos, avail);
    cache->pos = 0;
    cache->len = avail;

    cache->len += x_file_get (fc->file,
                              cache->buf + avail,
                              1,
                              fc->buf_size - avail);
    if (cache->len == avail)
        cache->eof = TRUE;
}
static void
file_cache_clear        (xCache         *cache)
{
    struct FileCache  *fc = (struct FileCache*)cache;

    x_object_unref (fc->file);
    x_free (cache->buf);
}

xCache*
x_cache_new             (xbyte          *data,
                         xsize          len)
{
    struct RealCache  *cache;

    cache = x_new0 (struct RealCache,1);
    cache->len = len;
    cache->buf = data;
    cache->fill = mem_cache_fill;
    return (xCache*)cache;
}

xCache*
x_file_cache_new        (xFile          *file,
                         xsize          buf_size)
{
    struct FileCache  *fc;
    struct RealCache  *cache;

    fc = x_new0 (struct FileCache, 1);

    fc->buf_size = buf_size;
    fc->file = x_object_ref (file);

    cache = (struct RealCache*)fc;
    cache->buf = x_malloc (buf_size);
    cache->fill = file_cache_fill;
    cache->clear= file_cache_clear;
    return (xCache*)cache;
}

void
x_cache_delete          (xCache         *cache)
{
    struct RealCache  *rc = (struct RealCache*)cache;
    if (rc->clear)
        rc->clear (cache);
    x_free (cache);
}

void
x_flip_endian           (xptr           buf,
                         xsize          size,
                         xsize          count)
{
    xbyte *p = buf;
    if(size == 8){
        while(count--) {
            p[0] = p[0] ^ p[7];
            p[7] = p[0] ^ p[7];
            p[0] = p[0] ^ p[7];

            p[1] = p[1] ^ p[6];
            p[6] = p[1] ^ p[6];
            p[1] = p[1] ^ p[6];

            p[2] = p[2] ^ p[5];
            p[5] = p[2] ^ p[5];
            p[2] = p[2] ^ p[5];

            p[3] = p[3] ^ p[4];
            p[4] = p[3] ^ p[4];
            p[3] = p[3] ^ p[4];
            p += 8;
        }
    }
    else if(size == 4) {
        while(count--) {
            p[0] = p[0] ^ p[3];
            p[3] = p[0] ^ p[3];
            p[0] = p[0] ^ p[3];
            p[1] = p[1] ^ p[2];
            p[2] = p[1] ^ p[2];
            p[1] = p[1] ^ p[2];
            p += 4;
        }
    }
    else if(size == 2) {
        while (count--) {
            p[0] = p[0] ^ p[1];
            p[1] = p[0] ^ p[1];
            p[0] = p[0] ^ p[1];
            p += 2;
        }
    }
    else {
        while(count--) {
            int i=0,j=size-1;
            while(i<j) {
                p[i] = p[i] ^ p[j];
                p[j] = p[i] ^ p[j];
                p[i] = p[i] ^ p[j];
                ++i,--j;
            }
            p += size;
        }
    }
}

void
x_cache_set_flip_endian (xCache         *cache,
                         xbool          flip)
{
    cache->flip_endian = flip;
}

xbool
x_cache_get_flip_endian (xCache         *cache)
{
    return cache->flip_endian;
}
xsize
x_cache_get            (xCache         *cache,
                        xptr           buf,
                        xsize          size,
                        xsize          count)
{
    xsize   rd = 0;
    xsize  avail;
    struct RealCache  *rc = (struct RealCache*)cache;

    count *= size;
    while (TRUE) {
        avail = cache->len - cache->pos;
        if (avail > 0) {
            avail = MIN (avail, count);
            memcpy (buf, cache->buf+ cache->pos, avail);
            cache->pos += avail;
            rd += avail;
        }
        if (cache->eof || rd >= count)
            break;
        else if (rd < count) 
            rc->fill (cache);
    }
    cache->count += rd;
    if (size > 1) {
        rd /= size;
        if(cache->flip_endian)
            x_flip_endian (buf, size, rd);
    }
    return rd;
}

xcstr
x_cache_gets            (xCache         *cache)
{
    xbyte *ret = NULL;
    struct RealCache  *rc = (struct RealCache*)cache;

    while (TRUE) {
        if (cache->len > cache->pos) {
            if (!cache->buf[cache->pos]  ||  isspace (cache->buf[cache->pos])) {
                if (ret)
                    break;
            }
            else {
                if (!ret)
                    ret = cache->buf + cache->pos;
                cache->pos++;
                cache->count++;
            }
        }
        else if (cache->eof)
            break;
        else {
            rc->fill (cache);
            if (ret)
                ret = cache->buf;
        }
    }
    cache->count++;
    cache->buf[cache->pos++] = '\0';
    return (xcstr)ret;
}
xint
x_cache_getc           (xCache         *cache)
{
    struct RealCache  *rc = (struct RealCache*)cache;

    while (TRUE) {
        if (cache->len > cache->pos) {
            cache->count++;
            return cache->buf[cache->pos++];
        }
        else if (cache->eof)
            return -1;
        else
            rc->fill (cache);
    }
}
xint16
x_cache_geti16          (xCache         *cache)
{
    xint16 ret;
    x_cache_get (cache, &ret, sizeof(ret), 1);
    return ret;
}

xint32
x_cache_geti32          (xCache         *cache)
{
    xint32 ret;
    x_cache_get (cache, &ret, sizeof(ret), 1);
    return ret;
}

xfloat32
x_cache_getf32          (xCache         *cache)
{
    xfloat32 ret;
    x_cache_get (cache, &ret, sizeof(ret), 1);
    return ret;
}

xsize
x_cache_tell            (xCache         *cache)
{
    return cache->count;
}

void
x_cache_skip            (xCache         *cache,
                         xsize          offset)
{
    struct RealCache  *rc = (struct RealCache*)cache;

    while (offset > 0) {
        if (cache->len > cache->pos) {
            xsize avail = cache->len - cache->pos;
            avail = MIN(avail, offset);
            cache->pos += avail;
            cache->count += avail;
            offset -= avail;
        }
        else if (cache->eof)
            return;
        else
            rc->fill (cache);
    }
}

void
x_cache_back            (xCache         *cache,
                         xsize          offset)
{
    if (cache->pos < offset)
        x_error ("can't rewind");
    else {
        cache->pos -= offset;
        cache->count -= offset;
    }
}
void
x_cache_mark            (xCache         *cache,
                         xssize         offset)
{
    cache->mark = cache->count + offset;
}
xssize
x_cache_offset          (xCache         *cache)
{
    return cache->count - cache->mark;
}

X_DEFINE_TYPE_EXTENDED (xArchive, x_archive, X_TYPE_OBJECT, X_TYPE_ABSTRACT,);
static void
x_archive_init          (xArchive       *archive)
{
}

static void
x_archive_class_init    (xArchiveClass  *klass)
{
}

static xType file_archive_type (void);

xArchive*
x_archive_load          (xcstr          path,
                         xcstr          first_property,
                         ...)
{
    xUri        *uri;
    xArchive    *ar;
    xError      *error = NULL;
    xType       type;
    xArchiveClass *klass;
    va_list     argv;

    x_return_val_if_fail (path != NULL, NULL);

    uri = x_uri_parse (path, &error);
    if (!uri) {
        x_warning ("invalid uri for archive:%s", error->message);
        x_error_delete (error);
        return NULL;
    }

    type = file_archive_type();
    if (uri->scheme) {
        type = x_type_from_mime (X_TYPE_ARCHIVE, uri->scheme);
        x_return_val_if_fail (type != X_TYPE_INVALID, NULL);
    }

    va_start (argv, first_property);
    ar = x_object_new_valist (type, first_property, argv);
    va_end (argv);

    x_return_val_if_fail (ar != NULL, NULL);

    x_object_unsink (ar);
    klass = X_ARCHIVE_GET_CLASS (ar);
    x_return_val_if_fail (klass != NULL, NULL);

    if (!klass->load (ar, uri, &error)) {
        x_warning ("%s",error->message);
        x_object_unref (ar);
        ar = NULL;
    }

    x_error_delete (error);
    x_free (uri);

    return ar;
}

xSList*
x_archive_list          (xArchive       *archive)
{
    xArchiveClass *klass;
    x_return_val_if_fail (archive != NULL, NULL);

    klass = X_ARCHIVE_GET_CLASS(archive);
    x_return_val_if_fail (klass != NULL, NULL);

    return klass->list(archive);
}
xFile*
x_archive_open_valist   (xArchive       *archive,
                         xcstr          file_name,
                         xcstr          first_property,
                         va_list        argv)
{
    xArchiveClass   *klass;
    xFile           *file;

    x_return_val_if_fail (archive != NULL, NULL);
    x_return_val_if_fail (file_name != NULL, NULL);

    klass = X_ARCHIVE_GET_CLASS(archive);

    x_return_val_if_fail (klass != NULL, NULL);

    file = klass->open(archive, file_name, first_property, argv);

    return file;
}
xFile*
x_archive_open          (xArchive       *archive,
                         xcstr          file_name,
                         xcstr          first_property,
                         ...)
{
    xFile           *file;
    va_list         argv;

    va_start (argv, first_property);
    file = x_archive_open_valist (archive, file_name,
                                  first_property, argv);
    va_end (argv);

    return file;
}

/* dir archive realize */
typedef struct {
    xFile           parent;
    FILE            *fp;
    xstr            mode;
} File;
typedef struct {
    xFileClass      parent;
} FileClass;
typedef struct {
    xArchive        parent;
    xstr            path;
    xDir            *dir;
} FileArchive;
typedef struct {
    xArchiveClass   parent;
} FileArchiveClass;
X_DEFINE_TYPE (File, file, X_TYPE_FILE);
X_DEFINE_TYPE (FileArchive, file_archive, X_TYPE_ARCHIVE);

static xsize
file_read               (xFile          *object,
                         xchar          *buf,
                         xsize          count)
{
    File* file = (File*)object;
    return fread (buf, 1, count, file->fp);
}
static xsize
file_write              (xFile          *object,
                         xchar          *buf,
                         xsize          count)
{
    File* file = (File*)object;
    return fwrite (buf, 1, count, file->fp);
}
static xbool
file_seek               (xFile          *object,
                         xint           offset,
                         xSeekOrigin    origin)
{
    File* file = (File*)object;
    return fseek (file->fp, offset, origin) == 0;
}
static xsize
file_tell               (xFile          *object)
{
    File* file = (File*)object;
    return ftell (file->fp);
}

enum { PROP_0, PROP_MODE};
static void
file_set_property       (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
    File *file = (File*)object;
    switch(property_id){
    case PROP_MODE:
        file->mode =  x_value_dup_str (value);
        break;
    }
}

static void
file_get_property       (xObject            *object,
                         xuint              property_id,
                         xValue             *value,
                         xParam             *pspec)
{
}
static void
file_finalize           (xObject        *object)
{
    File *file = (File*)object;
    fclose (file->fp);
    x_free (file->mode);
    X_OBJECT_CLASS(file_parent_class)->finalize (object);
}
static void
file_init               (File        *file)
{
}
static void
file_class_init         (FileClass      *klass)
{
    xObjectClass   *oclass;
    xFileClass     *fclass;
    xParam         *param;
    oclass = X_OBJECT_CLASS(klass);

    oclass->get_property = file_get_property;
    oclass->set_property = file_set_property;
    oclass->finalize     = file_finalize;

    fclass = X_FILE_CLASS (klass);
    fclass->read  = file_read;
    fclass->write = file_write;
    fclass->tell  = file_tell;
    fclass->seek  = file_seek;

    param = x_param_str_new ("mode","mode","mode", "rb",
                             X_PARAM_STATIC_STR|X_PARAM_CONSTRUCT_ONLY|X_PARAM_READWRITE);
    x_oclass_install_param(oclass, PROP_MODE,param);
}

static xbool
file_archive_load       (xArchive       *ar,
                         xUri           *uri,
                         xError         **error)
{
    FileArchive *dir_ar = (FileArchive*)ar;
#ifdef X_OS_WINDOWS
    /* /c: -> c: */
    if (uri->path[0] == '/' && isalpha(uri->path[1]) && uri->path[2]==':')
        dir_ar->path = x_strdup (uri->path+1);
    else
#endif
        dir_ar->path = x_strdup (uri->path);
    dir_ar->dir = x_dir_open (dir_ar->path, error);
    return dir_ar->dir != NULL;
}
static xFile*
file_archive_open       (xArchive       *ar,
                         xcstr          file_name,
                         xcstr          first_property,
                         va_list        argv)
{
    FileArchive  *dir_ar = (FileArchive*)ar;
    File *file;
    xstr filename;

    file = x_object_new_valist (file_type(), first_property, argv);
    x_return_val_if_fail (file != NULL, NULL);

    filename = x_build_path (NULL, dir_ar->path, file_name, NULL);
    file->fp =fopen (filename , file->mode);
    x_free (filename);

    return (xFile*)file;
}
static xSList*
file_archive_list       (xArchive       *ar)
{
    FileArchive  *dir_ar;
    xSList      *result;
    xcstr       path;

    dir_ar = (FileArchive*)ar;
    x_dir_rewind (dir_ar->dir);
    result = NULL;
    while ((path = x_dir_read (dir_ar->dir)) != NULL) {
        xsize len = strlen(path);
        if (len > 0 && path[len-1] != '/')
            result = x_slist_append (result, x_strdup (path));
    }
    return result;
}
static void
file_archive_init       (FileArchive    *archive)
{
}
static void
file_archive_class_init (FileArchiveClass *klass)
{
    xArchiveClass* ar_class = (xArchiveClass*) klass;
    ar_class->list = file_archive_list;
    ar_class->load = file_archive_load;
    ar_class->open = file_archive_open;
}
