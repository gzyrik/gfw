#ifndef __X_ARCHIVE_H__
#define __X_ARCHIVE_H__
#include "xtype.h"
X_BEGIN_DECLS

#define X_TYPE_FILE                 (x_file_type())
#define X_FILE(object)              (X_INSTANCE_CAST((object), X_TYPE_FILE, xFile))
#define X_FILE_CLASS(klass)         (X_CLASS_CAST((klass), X_TYPE_FILE, xFileClass))
#define X_IS_FILE(object)           (X_INSTANCE_IS_TYPE((object), X_TYPE_FILE))
#define X_IS_FILE_CLASS(klass)      (X_CLASS_IS_TYPE((klass), X_TYPE_FILE))
#define X_FILE_GET_CLASS(object)    (X_INSTANCE_GET_CLASS((object), X_TYPE_FILE, xFileClass))

xbyte*      x_file_read_all         (xFile          *file,
                                     xsize          *outlen);

xsize       x_file_get              (xFile          *file,
                                     xptr           buf,
                                     xsize          size,
                                     xsize          count);

xsize       x_file_put              (xFile          *file,
                                     xptr           buf,
                                     xsize          size,
                                     xsize          count);

void        x_file_set_flip_endian  (xFile          *file,
                                     xbool          flip);

xbool       x_file_get_flip_endian  (xFile          *file);

xsize       x_file_putc             (xFile          *file,
                                     xchar          data);

xsize       x_file_puti16           (xFile          *file,
                                     xint16         data);

xsize       x_file_puti32           (xFile          *file,
                                     xint32         data);

xsize       x_file_putf32           (xFile          *file,
                                     xfloat32       data);

xsize       x_file_puts             (xFile          *file,
                                     xcstr          str,
                                     xbool          zero);

xsize       x_file_print            (xFile          *file,
                                     xbool          zero,
                                     xcstr          format,
                                     ...) X_PRINTF (3, 4);

xsize       x_file_print_valist     (xFile          *file,
                                     xbool          zero,
                                     xcstr          format,
                                     va_list        args);

xbool       x_file_seek             (xFile          *file,
                                     xint           offset,
                                     xSeekOrigin    origin);

xsize       x_file_tell             (xFile          *file);


#define X_TYPE_ARCHIVE              (x_archive_type())
#define X_ARCHIVE(object)           (X_INSTANCE_CAST((object), X_TYPE_ARCHIVE, xArchive))
#define X_ARCHIVE_CLASS(klass)      (X_CLASS_CAST((klass), X_TYPE_ARCHIVE, xArchiveClass))
#define X_IS_ARCHIVE(object)        (X_INSTANCE_IS_TYPE((object), X_TYPE_ARCHIVE))
#define X_IS_ARCHIVE_CLASS(klass)   (X_CLASS_IS_TYPE((klass), X_TYPE_ARCHIVE))
#define X_ARCHIVE_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), X_TYPE_ARCHIVE, xArchiveClass))

xArchive*   x_archive_load          (xcstr          path,
                                     xcstr          first_property,
                                     ...);

xSList*     x_archive_list          (xArchive       *archive);

xFile*      x_archive_open          (xArchive       *archive,
                                     xcstr          file_name,
                                     xcstr          first_property,
                                     ...);

xFile*      x_archive_open_valist   (xArchive       *archive,
                                     xcstr          file_name,
                                     xcstr          first_property,
                                     va_list        argv);

xCache*     x_file_cache_new        (xFile          *file,
                                     xsize          buf_size);

xCache*     x_cache_new             (xbyte          *data,
                                     xsize          len);

void        x_cache_delete          (xCache         *cache);

xsize       x_cache_get             (xCache         *cache,
                                     xptr           buf,
                                     xsize          size,
                                     xsize          count);

void        x_cache_set_flip_endian (xCache         *cache,
                                     xbool          flip);

xbool       x_cache_get_flip_endian (xCache         *cache);

xcstr       x_cache_gets            (xCache         *cache);

xint        x_cache_getc            (xCache         *cache);

xint16      x_cache_geti16          (xCache         *cache);

xint32      x_cache_geti32          (xCache         *cache);

xfloat32    x_cache_getf32          (xCache         *cache);

xsize       x_cache_tell            (xCache         *cache);

void        x_cache_skip            (xCache         *cache,
                                     xsize          offset);

void        x_cache_back            (xCache         *cache,
                                     xsize          offset);

/** sets this mark at offset of its position. */
void        x_cache_mark            (xCache         *cache,
                                     xssize         offset);

/** return the bytes between the position and the mark */
xssize      x_cache_offset          (xCache         *cache);

void        x_flip_endian           (xptr           buf,
                                     xsize          size,
                                     xsize          count);

enum _xSeekOrigin{
    X_SEEK_SET,
    X_SEEK_CUR,
    X_SEEK_END
};

struct _xFile
{
    xObject             parent;
    xbool               flip_endian;
};

struct _xFileClass
{
    xObjectClass        parent;
    xsize   (*read)     (xFile*, xchar*, xsize);
    xsize   (*write)    (xFile*, xchar*, xsize);
    xbool   (*seek)     (xFile*, xint, xSeekOrigin);
    xsize   (*tell)     (xFile*);
};

struct _xArchive
{
    xObject             parent;
};

struct _xArchiveClass
{
    xObjectClass        parent;
    xbool   (*load)     (xArchive*, xUri*, xError **);
    xFile*  (*open)     (xArchive*, xcstr, xcstr, va_list argv);
    xSList* (*list)     (xArchive*);
};

xType       x_file_type             (void);


xType       x_archive_type          (void);


X_END_DECLS
#endif /* __X_ARCHIVE_H__ */


