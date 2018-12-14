#include "rdp_config.hpp"
#include <cstdio>
#include <cstdarg>
#ifdef ANDROID
#include <android/log.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#endif
namespace rdp {
void log(const char* format, ...)
{
    va_list argv;
    va_start(argv, format);
#ifdef ANDROID
    int prio;
    switch(format[0]){
    case 'E':
        prio = ANDROID_LOG_ERROR;
        break;
    case 'W':
        prio = ANDROID_LOG_WARN;
        break;
    case 'I':
        prio = ANDROID_LOG_INFO;
        break;
    default:
        prio = ANDROID_LOG_DEBUG;
    }
    __android_log_vprint(prio, "RDP", format, argv);
#else
    char str[2048];
    int len = vsprintf(str,format,argv);
#ifdef _WIN32
    OutputDebugStringA(str);
    if (str[len-1] != '\n') OutputDebugStringA("\n");
#else
    fputs(str, stderr);
    if (str[len-1] != '\n') fputc('\n', stderr);
#endif
#endif
    va_end(argv);
}
}
