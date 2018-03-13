#include "config.h"
#include "xmodule.h"
#include "xthread.h"
#include "xmem.h"
#include "xutil.h"
#include "xstr.h"
#include "xerror.h"
#include "xatomic.h"
#include "xquark.h"
#include "xmsg.h"
#include "xfile.h"
#include <string.h>
enum
{
    X_MODULE_BIND_LAZY          = 1 << 0,
    X_MODULE_BIND_LOCAL         = 1 << 1,
    X_MODULE_RESIDENT           = 1 << 2,
};

typedef xcstr   (*xModuleInit)      (xModule        *module);
typedef void    (*xModuleUnload)    (void);

struct _xModule
{
    xstr                file_name;
    xptr                handle;
    xuint               ref_count   :31;
    xuint               is_resident : 1;
    xModuleUnload       unload;
    xModule             *next;
};

static xModule          *modules     = NULL;
static xModule          *main_module = NULL;
static xRecMutex        module_recmutex;
static xuint	        module_flags = 0;
static xptr
module_open             (xcstr          file_name,
                         xbool          bind_lazy,
                         xbool          bind_local,
                         xError         **error);
static xptr
module_symbol           (xptr           handle,
                         xcstr          symbol_name,
                         xError         **error);
static void
module_close            (xptr           handle,
                         xbool          is_unref);

static xptr
module_self             (void);

static inline xModule*
x_module_find_by_handle (xptr           handle)
{
    xModule *module;
    xModule *retval = NULL;

    if (main_module && main_module->handle == handle)
        retval = main_module;
    else
        for (module = modules; module; module = module->next)
            if (handle == module->handle) {
                retval = module;
                break;
            }

    return retval;
}

static inline xModule*
module_find_by_name (xcstr          name)
{
    xModule *module;
    xModule *retval = NULL;

    for (module = modules; module; module = module->next)
        if (strcmp (name, module->file_name) == 0)
        {
            retval = module;
            break;
        }

    return retval;
}

static xstr
parse_libtool_archive   (xcstr          libtool_name)
{
    return NULL;
}

#ifdef X_OS_WINDOWS
#include "xmodule-win32.h"
#elif defined (HAVE_DLFCN_H)
#include "xmodule-dl.h"
#else
static xptr
module_open             (xcstr          file_name,
                         xbool          bind_lazy,
                         xbool          bind_local,
                         xError         **error)
{
    return NULL;
}
static xptr
module_symbol           (xptr           handle,
                         xcstr          symbol_name,
                         xError         **error)
{
    return NULL;
}
static void
module_close            (xptr           handle,
                         xbool          is_unref)
{
}
static xptr
module_self             (void)
{
    return NULL;
}
#endif

xQuark
x_module_domain         (void)
{
    static xQuark quark = 0;
    if (!x_atomic_ptr_get(&quark) && x_once_init_enter(&quark)) {
        quark = x_quark_from_static("xModule");
        x_once_init_leave (&quark);
    }
    return quark;
}

xModule*
x_module_open           (xcstr          file_name,
                         xError         **error)
{
    xModule *module;
    xptr    handle = NULL;
    xstr    name = NULL;

    x_recmutex_lock (&module_recmutex);

    if (!file_name) {      
        if (!main_module) {
            handle = module_self ();
            if (handle) {
                main_module = x_new (xModule, 1);
                main_module->file_name = NULL;
                main_module->handle = handle;
                main_module->ref_count = 1;
                main_module->is_resident = TRUE;
                main_module->unload = NULL;
                main_module->next = NULL;
            }
        }
        else
            main_module->ref_count++;

        x_recmutex_unlock (&module_recmutex);
        return main_module;
    }

    /* we first search the module list by name */
    module = module_find_by_name (file_name);
    if (module) {
        module->ref_count++;

        x_recmutex_unlock (&module_recmutex);
        return module;
    }

    /* check whether we have a readable file right away */
    if (x_path_test (file_name, 'f'))
        name = x_strdup (file_name);
    /* try completing file name with standard library suffix */
    if (!name) {
        name = x_strcat (file_name, "." X_MODULE_SUFFIX, NULL);
        if (!x_path_test (name, 'f')) {
            x_free (name);
            name = NULL;
        }
    }
    /* try completing by appending libtool suffix */
    if (!name) {
        name = x_strcat (file_name, ".la", NULL);
        if (!x_path_test (name, 'f')) {
            x_free (name);
            name = NULL;
        }
    }
    /* we can't access() the file, lets hope the platform backends finds
     * it via library paths
     */
    if (!name) {
        xcstr dot = strrchr (file_name, '.');
        xcstr slash = x_strrstr (file_name, X_DIR_SEPARATOR);

        /* make sure the name has a suffix */
        if (!dot || dot < slash)
            name = x_strcat (file_name, "." X_MODULE_SUFFIX, NULL);
        else
            name = x_strdup (file_name);
    }

    /* ok, try loading the module */
    if (name) {
        /* if it's a libtool archive, figure library file to load */
        if (x_str_suffix (name, ".la")) {/* libtool archive? */
            xchar *real_name = parse_libtool_archive (name);

            /* real_name might be NULL, but then module error is already set */
            if (real_name) {
                x_free (name);
                name = real_name;
            }
        }
        if (name)
            handle = module_open (name,
                                  (module_flags & X_MODULE_BIND_LAZY) != 0,
                                  (module_flags & X_MODULE_BIND_LOCAL) != 0,
                                  error);
    } else  {
        x_set_error (error, x_module_domain(), -1,
                     "unable to access file \"%s\"", file_name);
    }
    x_free (name);

    if (handle) {
        xModuleInit     check_init;
        xcstr           check_failed = NULL;

        /* search the module list by handle, since file names are not unique */
        module = x_module_find_by_handle (handle);
        if (module) {
            module_close (module->handle, TRUE);
            module->ref_count++;

            x_recmutex_unlock (&module_recmutex);
            return module;
        }

        module = x_new (xModule, 1);
        module->file_name   = x_strdup (file_name);
        module->handle      = handle;
        module->ref_count   = 1;
        module->is_resident = FALSE;
        module->unload      = NULL;
        module->next        = modules;
        modules = module;

        /* check initialization */
        check_init = x_module_symbol (module,
                                      "x_module_init",
                                      NULL);
        if (check_init)
            check_failed = check_init (module);

        /* we don't call unload() if the initialization check failed. */
        if (!check_failed)
            module->unload = x_module_symbol (module,
                                              "x_module_unload",
                                              NULL);

        if (check_failed ) {
            x_set_error (error, x_module_domain(), -1,
                         "xModule (%s)  initialization failed: %s",
                         file_name, check_failed);
            x_module_close (module);
            module = NULL;
        }
    }

    if (module != NULL && (module_flags & X_MODULE_RESIDENT) != 0)
        x_module_resident (module);

    x_recmutex_unlock (&module_recmutex);
    return module;
}

void
x_module_close          (xModule        *module)
{
}

void
x_module_resident       (xModule        *module)
{
}

xptr
x_module_symbol         (xModule        *module,
                         xcstr          symbol_name,
                         xError         **error)
{
    xptr symbol;
    x_return_val_if_fail (module != NULL, NULL);
    x_return_val_if_fail (symbol_name != NULL, NULL);

    x_recmutex_lock (&module_recmutex);
    symbol = module_symbol (module->handle, symbol_name, error);
    x_recmutex_unlock (&module_recmutex);

    return symbol;
}

/* vim: set et sw=4 ts=4 cino=g0,\:0,l1,t0,(0:  */
