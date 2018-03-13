#include "config.h"
#include "xinit-prv.h"
xLogLevelFlags x_log_always_fatal = X_LOG_FATAL_MASK;
xLogLevelFlags x_log_msg_prefix = X_LOG_LEVEL_ERROR | X_LOG_LEVEL_WARNING |
                                  X_LOG_LEVEL_CRITICAL | X_LOG_LEVEL_DEBUG;
#ifdef ENABLE_GC_FRIENDLY_DEFAULT
xbool x_mem_gc_friendly = TRUE;
#else
xbool x_mem_gc_friendly = FALSE;
#endif

#if defined X_OS_WINDOWS
#include <windows.h>
xptr _x_os_dll;

BOOL WINAPI
DllMain                 (HINSTANCE      hinstDLL,
                         DWORD          fdwReason,
                         LPVOID         lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        _x_os_dll = (xptr) hinstDLL;
        _x_clock_win32_init ();
        _x_thread_win32_init ();
        break;
    case DLL_THREAD_DETACH:
        _x_thread_win32_detach ();
        break;
    default:
        ;/* do nothing */
    }
    return TRUE;
}
#endif
