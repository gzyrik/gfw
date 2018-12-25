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
    kVersion = 0, //版本号,占2bit
    kMaxMtuBytes = 1500,
    kStartBwKbps = 1200,
};
/** 返回是否是大端序(高位在字节序前部) */
inline bool bigEndian()
{
    return *(uint16_t*)("\0\1") == (uint16_t)1;
}
/** 返回 seq1与seq0 的序列差 */
inline int seqDelta(const uint16_t seq1,  const uint16_t seq0, const uint16_t maxSize=0xffff)
{
    if (seq1 == seq0)
        return 0;
    else if (seq0 > seq1) {
        if (!(seq0 > 0xff00 && seq1 < 0xff) && seq0 - seq1 < 5 * maxSize) 
            return - (seq0 - seq1); //没回滚且在合理范围内
        return 0x10000 + seq1 - seq0;
    } else {
        if (seq1 > 0xff00 && seq0 < 0xff) {//可能回滚
            if (seq0 < uint16_t(seq1 + maxSize))
                return - (0x10000 + seq0 - seq1);//范围内的合法回滚
        }
        else if (seq1 >= seq0 + 5 * maxSize)//远超范围,认为已回滚
            return - (0x10000 + seq0 - seq1);
        return seq1 - seq0;
    }
}
void log(const char* format, ...);
}
#endif
