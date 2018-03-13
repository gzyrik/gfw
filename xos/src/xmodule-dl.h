#include <dlfcn.h>
#ifndef	RTLD_LAZY
#define	RTLD_LAZY	1
#endif	/* RTLD_LAZY */
#ifndef	RTLD_NOW
#define	RTLD_NOW	0
#endif	/* RTLD_NOW */
#ifndef	RTLD_GLOBAL
#define	RTLD_GLOBAL	0
#endif	/* RTLD_GLOBAL */

static void
fetch_dlerror (xError         **error,
               xbool replace_null)
{
    xcstr msg = dlerror ();

    /* make sure we always return an error message != NULL, if
     * expected to do so. */
    if (!msg && replace_null)
        msg  = "unknown dl-error";
    if (msg)
        x_set_error (error, x_module_domain(), -1, "%s", msg);
}

static xptr
module_open             (xcstr          file_name,
                         xbool          bind_lazy,
                         xbool          bind_local,
                         xError         **error)
{
    xptr handle;

    handle = dlopen (file_name,
                     (bind_local ? 0 : RTLD_GLOBAL)
                     | (bind_lazy ? RTLD_LAZY : RTLD_NOW));
    if (!handle)
        fetch_dlerror (error, TRUE);

    return handle;

}

static xptr
module_symbol           (xptr           handle,
                         xcstr          symbol_name,
                         xError         **error)
{
    xptr p;

    dlerror();
    p = dlsym (handle, symbol_name);
    fetch_dlerror (error, FALSE);

    return p;
}

static void
module_close            (xptr           handle,
                         xbool          is_unref)
{
    dlclose (handle);
}

static xptr
module_self             (void)
{
    return dlopen (NULL, RTLD_GLOBAL | RTLD_LAZY);
}
