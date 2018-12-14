#ifndef __RDP_RECEIVER_HPP__
#define __RDP_RECEIVER_HPP__
#include "rdp_bwe.hpp"
namespace rdp {
struct AckFeedbackIFace;

/**
 * 数据包输出器
 */
struct OutputerIFace
{
    virtual ~OutputerIFace(){}
    //超时,无法复原的拆分包
    virtual void onSplitPackets_W(PacketPtr packet[], const int num) = 0;
    //序列包
    virtual void onOrderPackets_W(PacketPtr packet[], const int num, const bool ordered) = 0;
    //不可靠包
    virtual void onPacket_W(PacketPtr packet, const bool sequence) = 0;
};


/**
 * TODO
 */
struct ReceiverIFace
{
     struct Statistics
    {

        int orderedInOrder;
        int orderedOutOfOrder;
        int sequencedInOrder;
        int sequencedOutOfOrder;

        int ackSent;
        int ackTotalBitsSent;

        int receivedPackets;
        int duplicateReceivedPackets;
    };
    static ReceiverIFace* create(AckFeedbackIFace& ackFeedback,
                                 OutputerIFace& outputer,
                                 BWEIFace& estimator);
    virtual ~ReceiverIFace(){}
    virtual void stats(Statistics& stats) const = 0;
    //R线程, 读取网络数据流
    virtual void onReceived_R(BitStream& bs, const Time& nowNS) = 0;
    //W线程中自更新
    virtual void update_W(const Time& nowNS) = 0;
    //写入所有Ack,并清除ackRanges
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) = 0;

    //返回发送带宽bps
    virtual int tmmbrKbps_W(void) const = 0;
};

}
#endif
