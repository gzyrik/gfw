#include "xiconv.h"
#include "xwin.h"
#include <stdio.h>
#include <windows.h>

#include <tlhelp32.h>

#ifdef X_WITH_CYGWIN
#include <sys/cygwin.h>
#endif
static xint dummy;
static xptr null_module_handle = &dummy;
static xptr
module_open             (xcstr          file_name,
                         xbool          bind_lazy,
                         xbool          bind_local,
                         xError         **error)
{
    HINSTANCE handle;
    wchar_t *wfilename;
#ifdef X_WITH_CYGWIN
    xchar tmp[MAX_PATH];

    cygwin_conv_to_win32_path(file_name, tmp);
    file_name = tmp;
#endif
    wfilename = x_str_to_utf16 (file_name, -1, NULL, NULL,NULL);

    handle = LoadLibraryW (wfilename);
    x_free (wfilename);

    if (!handle) {
        int ecode = GetLastError ();
        xstr estr  = x_win_error_msg(ecode);
        x_set_error(error,x_module_domain(), ecode,
                    "`%s': %s", file_name, estr);
        x_free (estr);
    }
    return handle;
}
static xptr
find_in_any_module_using_toolhelp (xcstr    symbol_name)
{
    HANDLE snapshot; 
    MODULEENTRY32 me32;

    xptr p;

    if ((snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE, 0)) == (HANDLE) -1)
        return NULL;

    me32.dwSize = sizeof (me32);
    p = NULL;
    if (Module32First (snapshot, &me32)) {
        do {
            if ((p = GetProcAddress (me32.hModule, symbol_name)) != NULL)
                break;
        } while (Module32Next (snapshot, &me32));
    }

    CloseHandle (snapshot);

    return p;
}
static xptr
find_in_any_module      (xcstr          symbol_name)
{
    xptr result;

    if ((result = find_in_any_module_using_toolhelp (symbol_name)) == NULL)
        return NULL;
    else
        return result;
}
static xptr
module_symbol           (xptr           handle,
                         xcstr          symbol_name,
                         xError         **error)
{
    xptr p;

    if (handle == null_module_handle) {
        if ((p = GetProcAddress (GetModuleHandle (NULL), symbol_name)) == NULL)
            p = find_in_any_module (symbol_name);
    }
    else
        p = GetProcAddress (handle, symbol_name);

    return p;
}
static void
module_close            (xptr           handle,
                         xbool          is_unref)
{
    if (handle != null_module_handle)
        FreeLibrary (handle);
}

static xptr
module_self             (void)
{
    return null_module_handle;
}
