#ifndef __FEC_CONFIG_H__
#define __FEC_CONFIG_H__
#if defined _MSC_VER && _MSC_VER < 1600
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef __int32 int32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#define IP_PACKET_SIZE 1500    // we assume ethernet
#define ASSERT(exp)
#define LOG_E(id, format, ...)\
    rdp::log("E[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_D(id, format, ...)\
    rdp::log("D[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_I(id, format, ...)\
    rdp::log("I[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_W(id, format, ...)\
    rdp::log("W[%s:%d]"format, __FILE__, __LINE__, ##__VA_ARGS__)
namespace rdp {
    void log(const char* format, ...);
}
#endif