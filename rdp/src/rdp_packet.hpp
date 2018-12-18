#ifndef __RDP_PACKET_HPP__
#define __RDP_PACKET_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_time.hpp"
#include <string>
namespace rdp {
struct Packet
{
    enum Reliability
    {
        // 没有次序, 也不重发, 等价于普通UDP
        kUnreliable     = 0, 
        // 保证次序, 但不重发, 晚到包会丢弃
        kSequence       = 1, 
        // 没有次序, 会重发, 保证到达
        kReliable       = 2,
        // 保证次序, 会重发, 默认是保证到达,由OrderingFlag进行精细控制
        kOrderable      = kSequence | kReliable
    };
    /**
     * 针对kOrderable的特殊标记,
     * 可进行精细的控制.
     */
    enum OrderingFlag
    {
        // 类似于视频关键帧,刷新缓存,强制复位次序.
        // 发送端将不重发该次序前的包.
        kResetable      = 1,
        // 强制复位次序后,允许该包晚到
        kDelayable      = 2,
    };

    uint16_t messageNumber;
    uint8_t reliability;
    uint8_t orderingFlags;
    uint8_t orderingChannel;
    uint32_t orderingIndex;
    uint16_t splitPacketId;
    uint16_t splitPacketIndex;
    uint16_t splitPacketCount;
    uint16_t bitLength;
    uint8_t* payload;
    Time   timeStamp;

    //转换为描述性的字符串
    std::string toString() const;

    //释放
    void release();

    //是否完整
    bool compelted() const
    {
        return this->splitPacketCount == 0 || this->splitPacketId != 0;
    }

    //写入流
    bool writeToStream(BitStream& bs) const;

    //估计报文头大小,实际可能会小些
    int headerBitSize(bool splitting = false) const;

    //估计报文大小,实际可能会小些
    int bitSize() const
    {
        return headerBitSize() + this->bitLength;
    }
    //
    PacketPtr clone(int byteOffset, int bitLength) const;
    //
    PacketPtr clone(const uint8_t *buf, int bitLength) const;

    //尝试流中是否含有数据包
    static bool maybePacket(const BitStream& bs);
    //
    static PacketPtr merge(PacketPtr packets[], int num);
    //
    static PacketPtr readFromStream(BitStream& bs);
    static PacketPtr Reliable(const void* data, int bitLength);
};
}
#endif
