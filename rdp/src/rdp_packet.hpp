#ifndef __RDP_PACKET_HPP__
#define __RDP_PACKET_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_time.hpp"
#include <string>
namespace rdp {
struct Packet final
{
    enum Reliability
    {
        // 没有次序, 也不主动重发, 等价于普通UDP
        kUnreliable     = 0,

        // 保证次序, 但不主动重发, 丢弃乱序的包
        // orderingChannel 为次序通道
        kSequence       = 1,

        // 没有次序, 会主动重发(ACK 机制), 可能乱序
        kReliable       = 2,

        // 保证次序, 会主动重发(ACK 机制), 丢弃乱序的包
        // orderingChannel 为次序通道
        kRealtime       = kSequence | kReliable,

        // 保证次序, 会主动重发(ACK 机制), 等价于普通TCP
        // orderingChannel 为独立的排序通道
        // 次序与排序是不同的通道
        kOrderable      = kReliable | kSequence | 4 ,
    };

    uint32_t messageNumber;
    uint8_t reliability;
    uint8_t orderingChannel;
    uint32_t orderingIndex;
    uint16_t splitPacketId;
    uint16_t splitPacketIndex;
    uint16_t splitPacketCount;
    uint16_t bitLength;
    uint8_t* payload;
    Time   timeStamp;

    //转换为描述性的字符串
    //格式
    // "%X%c%x",     messageNumber,"USRO"[reliability], bitLength
    // "(%x/%x)",    orderingIndex, orderingChannel,
    // "[%x/%x|%x]", splitPacketIndex, splitPacketCount, splitPacketId,
    // ": %s",       payload
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
    static PacketPtr create(const void* data, int bitLength,
        enum Reliability reliability, uint8_t orderingChannel=0);
};
}
#endif
