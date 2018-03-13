#include "config.h"
#include "xdir.h"
#include "xmsg.h"
#include "xiconv.h"
#include "xmem.h"
#include "xerror.h"
#include "xatomic.h"
#include "xquark.h"
#include <errno.h>
#include <string.h>
xQuark
x_file_domain           (void)
{
    static xQuark quark = 0;
    if (!x_atomic_ptr_get(&quark) && x_once_init_enter(&quark)) {
        quark = x_quark_from_static("xFile");
        x_once_init_leave (&quark);
    }
    return quark;
}
#ifdef X_OS_WINDOWS
#include "dirent/dirent.h"
#include "dirent/wdirent.c"
struct _xDir
{
    _WDIR   *wdirp;
    xchar   utf8_buf[FILENAME_MAX*4];
};
xDir*
x_dir_open              (xcstr          path,
                         xError         **error)
{
    xDir    *dir;
    int     errsv;
    wchar_t *wpath;

    x_return_val_if_fail (path != NULL, NULL);

    wpath = x_str_to_utf16 (path, -1, NULL, NULL, error);

    if (wpath == NULL)
        return NULL;

    dir = x_new (xDir, 1);

    dir->wdirp = _wopendir (wpath);
    x_free (wpath);

    if (dir->wdirp)
        return dir;

    /* error case */
    errsv = errno;

    x_set_error (error,
                 x_file_domain(),
                 errsv,
                 "Error opening directory '%s': %s",
                 path, strerror (errsv));

    x_free (dir);

    return NULL;

}  

xcstr
x_dir_read              (xDir           *dir)
{
    xstr    utf8_name;
    struct _wdirent *wentry;

    x_return_val_if_fail (dir != NULL, NULL);
    while (1) {
        wentry = _wreaddir (dir->wdirp);
        while (wentry 
               && (0 == wcscmp (wentry->d_name, L"./") ||
                   0 == wcscmp (wentry->d_name, L"../")))
            wentry = _wreaddir (dir->wdirp);

        if (wentry == NULL)
            return NULL;

        utf8_name = x_utf16_to_str (wentry->d_name, -1, NULL, NULL, NULL);

        if (utf8_name == NULL)
            continue;		/* Huh, impossible? Skip it anyway */

        strcpy (dir->utf8_buf, utf8_name);
        x_free (utf8_name);

        return dir->utf8_buf;
    }

}

void
x_dir_rewind            (xDir           *dir)
{
    x_return_if_fail (dir != NULL);
    _wrewinddir (dir->wdirp);
}

void
x_dir_close             (xDir           *dir)
{
    x_return_if_fail (dir != NULL);
    _wclosedir (dir->wdirp);
}

#else

#include <sys/types.h>
#include <dirent.h>
#include <limits.h> /* PATH_MAX */
struct _xDir
{
    DIR *dirp;
    char d_name [PATH_MAX+1];
};
xDir*
x_dir_open              (xcstr          path,
                         xError         **error)
{
    xDir    *dir;
    int     errsv;

    dir = x_new (xDir, 1);

    dir->dirp = opendir (path);

    if (dir->dirp)
        return dir;

    /* error case */
    errsv = errno;

    x_set_error (error,
                 x_file_domain(),
                 errsv,
                 "Error opening directory '%s': %s",
                 path, strerror (errsv));
    x_free (dir);

    return NULL;
}

xcstr
x_dir_read              (xDir           *dir)
{
    struct dirent *entry;
    x_return_val_if_fail (dir != NULL, NULL);
    entry = readdir (dir->dirp);
    while (entry 
           && (0 == strcmp (entry->d_name, ".") ||
               0 == strcmp (entry->d_name, "..")))
        entry = readdir (dir->dirp);

    if (entry) {
        strcpy(dir->d_name, entry->d_name);
        if (entry->d_type == DT_DIR)
            strcat(dir->d_name, "/");
        return dir->d_name;
    }
    return NULL;
}

void
x_dir_rewind            (xDir           *dir)
{
    x_return_if_fail (dir != NULL);
    rewinddir (dir->dirp);
}

void
x_dir_close             (xDir           *dir)
{
    x_return_if_fail (dir != NULL);
    closedir (dir->dirp);
}
#endif
void
x_dir_foreach           (xcstr          path,
                         xVisitFunc     visit,
                         xptr           user_data)
{
    xcstr name;
    xDir *dir = x_dir_open (path, NULL);
    x_return_if_fail(dir != NULL);

    while ((name = x_dir_read (dir)) != NULL)
        visit((xptr)name, user_data);
    x_dir_close(dir);
}
