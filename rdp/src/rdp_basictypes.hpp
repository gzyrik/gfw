#ifndef __RDP_BASICTYPES_HPP__
#define __RDP_BASICTYPES_HPP__

#if defined _MSC_VER && _MSC_VER < 1600
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef __int32 int32_t;
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif

namespace rdp {
struct BitStream;
struct Packet;
struct Time;
typedef struct Packet* PacketPtr;
template<typename T> class WeakPtr;
template<typename T> class SharedPtr;
}

#endif
