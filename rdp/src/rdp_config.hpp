#ifndef __RDP_CONFIG_HPP__
#define __RDP_CONFIG_HPP__
#include "rdp_basictypes.hpp"
#include <assert.h>
#ifdef _WIN32
#define HAVE_TIMEGETTIME        1
#define HAVE_WINDOWS_THREAD     1
#endif
#if defined(DARWIN) || defined(IOS)
#define HAVE_MACH_TIMEBASE_INFO 1
#endif

#define JIF(_exp, ...) do { \
    if (!(_exp)) { \
        rdp::log("W[%s:%d]"#_exp, __FILE__, __LINE__, ##__VA_ARGS__);\
        goto clean; \
    } \
} while (0)
#define ASSERT(exp) assert(exp)
#define LOG_E(format, ...)\
    rdp::log("E[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_D(format, ...)\
    rdp::log("D[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_I(format, ...)\
    rdp::log("I[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_W(format, ...)\
    rdp::log("W[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
namespace rdp {
enum {
    kMaxMtuBytes = 1500,
    kStartBwKbps = 1200,
};
/** 返回是否是大端序(高位在字节序前部) */
inline bool bigEndian()
{
    return *(uint16_t*)("\0\1") == (uint16_t)1;
}
void log(const char* format, ...);
}
#endif
