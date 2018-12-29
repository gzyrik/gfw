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
        // û�д���, Ҳ�������ط�, �ȼ�����ͨUDP
        kUnreliable     = 0,

        // ��֤����, ���������ط�, ��������İ�
        // orderingChannel Ϊ����ͨ��
        kSequence       = 1,

        // û�д���, �������ط�(ACK ����), ��������
        kReliable       = 2,

        // ��֤����, �������ط�(ACK ����), ��������İ�
        // orderingChannel Ϊ����ͨ��
        kRealtime       = kSequence | kReliable,

        // ��֤����, �������ط�(ACK ����), �ȼ�����ͨTCP
        // orderingChannel Ϊ����������ͨ��
        // �����������ǲ�ͬ��ͨ��
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

    //ת��Ϊ�����Ե��ַ���
    //��ʽ
    // "%X%c%x",     messageNumber,"USRO"[reliability], bitLength
    // "(%x/%x)",    orderingIndex, orderingChannel,
    // "[%x/%x|%x]", splitPacketIndex, splitPacketCount, splitPacketId,
    // ": %s",       payload
    std::string toString() const;

    //�ͷ�
    void release();

    //�Ƿ�����
    bool compelted() const
    {
        return this->splitPacketCount == 0 || this->splitPacketId != 0;
    }

    //д����
    bool writeToStream(BitStream& bs) const;

    //���Ʊ���ͷ��С,ʵ�ʿ��ܻ�СЩ
    int headerBitSize(bool splitting = false) const;

    //���Ʊ��Ĵ�С,ʵ�ʿ��ܻ�СЩ
    int bitSize() const
    {
        return headerBitSize() + this->bitLength;
    }
    //
    PacketPtr clone(int byteOffset, int bitLength) const;
    //
    PacketPtr clone(const uint8_t *buf, int bitLength) const;

    //���������Ƿ������ݰ�
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
