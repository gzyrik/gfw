#ifndef __RDP_SENDER_HPP__
#define __RDP_SENDER_HPP__
#include "rdp_rangelist.hpp"
#include "rdp_basictypes.hpp"
namespace rdp {

/** ACK 范围数组 */
typedef RangeList<uint16_t> AckRangeList;

/**
 * ACK反馈者
 */
struct AckFeedbackIFace
{
    virtual ~AckFeedbackIFace(){}
    //R线程,收到ACK, 主要用于清除重发包
    virtual void onAcks_R(AckRangeList& acks, const Time& rttAverage) = 0;
};

/**
 * TODO
 * 发送端可能发送的部分
 * - PH 可变包头
 * - PD payload 数据
 * - RH 重发的可变包头
 * - RD 重发的payload数据
 * - AK Ack数据
 * 因此有效带宽 = TMMBR - (PH + RH + RD) - AK
 * TMMBR 由ReceiverIFace::tmmbrKbps_W()返回
 * AK 由ReceiverIFace::ackKbps_W()返回
 *
 */
struct SenderIFace : public AckFeedbackIFace
{
    enum Priority
    {
        kSystem = 0,
        kHigh,
        kMedium,
        kLow,
        kNumberOfPriority
    };
    struct Statistics
    {
        int splitPackets;
        int unsplitPackets;

        int resentPackets;
        int bitResent;

        ///包的发送个数
        int sentPackets[kNumberOfPriority];

        ///payload的发送位数
        int bitSent[kNumberOfPriority];
    };

    virtual void stats(Statistics& stats) const = 0;
    //W线程,写入包(重发包优先),用于网络发送
    virtual void writePackets_W(BitStream& bs, const Time& nowNS) = 0;
    //I线程,发送数据报,用户层接口.
    virtual void sendPacket_I(PacketPtr packet, const enum Priority priority, const Time& nowNS) = 0;
    //标志
    virtual void markSplitPacket_I(PacketPtr packet) = 0;

    static SenderIFace* create(const int bitMTU);
};

}
#endif
