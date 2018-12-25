#ifndef __RDP_RECEIVER_HPP__
#define __RDP_RECEIVER_HPP__
#include "rdp_bwe.hpp"
#include "rdp_transport.hpp"
namespace rdp {
struct AckFeedbackIFace;

/**
 * 数据包输出器
 */
struct OutputerIFace
{
    virtual ~OutputerIFace(){}
    virtual void onPackets_W(PacketPtr packet[], const int num) = 0;
    //超时无法复原的拆分包
    virtual void onExpiredPackets_W(PacketPtr packet[], const int num) = 0;
    //丢弃的乱序列包
    virtual void onDisorderPacket_W(PacketPtr packet) = 0;
};


/**
 * TODO
 */
struct ReceiverIFace : public transport::ReceiverIFace
{
     struct Statistics
    {
        int orderedInOrder;
        int orderedOutOfOrder;
        int sequencedInOrder;
        int sequencedOutOfOrder;

        int ackBitSent; //已发送的ACK码流
        int nackBitSent; //已发送的NACK码流
        int ackTotalBitSent;//额外的反馈码流(ACK+NACK+RTT+TMMBR)

        int receivedPackets;
        int duplicateReceivedPackets;

        //发送带宽
        int bwKbps;
    };

    virtual void stats(Statistics& stats) const = 0;

    //W线程中自更新
    virtual void update_W(const Time& nowNS) = 0;

    //写入所有Ack,并清除ackRanges
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) = 0;

    static ReceiverIFace* create(AckFeedbackIFace& ack, OutputerIFace& out, BWEIFace& bwe);
    virtual ~ReceiverIFace(){}
};

}
#endif
