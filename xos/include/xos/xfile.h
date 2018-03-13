#ifndef __X_FILE_H__
#define __X_FILE_H__
#include <stdarg.h>
#include <sys/stat.h>
#include <stdio.h>
#include "xtype.h"
X_BEGIN_DECLS
#if defined (X_OS_WIN32)
typedef struct _stat32 xStatBuf;
#elif defined (X_OS_WIN64)
typedef struct _stat32 xStatBuf;
#else
typedef struct stat xStatBuf;
#endif

struct _xUri
{
    xstr        scheme;
    xstr        user;
    xstr        host;
    xstr        port;
    xstr        path;
    xstr        query;
    xstr        fragment;
};

/** test file attribute
 * @param[in] test  can be 'e', 'f', 'x'
 */
xbool       x_path_test             (xcstr          filename,
                                     xchar          test);
xbool       x_path_is_absolute      (xcstr          path);

xstr        x_path_get_basename     (xcstr          path);

xstr        x_path_get_dirname      (xcstr          path);

xstr        x_build_path            (xcstr          separator,
                                     ...)  X_MALLOC X_NULL_TERMINATED;

xstr        x_build_patha           (xcstr          separator,
                                     xcstr          args[]);

xstr        x_build_pathv           (xcstr          separator,
                                     va_list        argv);

xUri*       x_uri_parse             (xcstr          str,
                                     xError         **error);

xstr        x_uri_build             (xUri           *uri,
                                     xbool          allow_utf8);

xint        x_access                (xcstr          filename,
                                     xint           mode);

xint        x_chmod                 (xcstr          filename,
                                     xint           mode);

xint        x_open                  (xcstr          filename,
                                     xint           flags,
                                     xint           mode);

xint        x_creat                 (xcstr          filename,
                                     xint           mode);

xint        x_rename                (xcstr          oldfilename,
                                     xcstr          newfilename);

xint        x_mkdir                 (xcstr          filename,
                                     xint           mode);

xint        x_chdir                 (xcstr          path);

xint        x_stat                  (xcstr          filename,
                                     xStatBuf       *buf);

xint        x_lstat                 (xcstr          filename,
                                     xStatBuf       *buf);

xint        x_unlink                (xcstr          filename);

xint        x_remove                (xcstr          filename);

xint        x_rmdir                 (xcstr          filename);

FILE*       x_fopen                 (xcstr          filename,
                                     xcstr          mode);

FILE*       x_freopen               (xcstr          filename,
                                     xcstr          mode,
                                     FILE           *stream);
X_END_DECLS
#endif /* __X_FILE_H__ */
