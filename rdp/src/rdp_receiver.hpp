#ifndef __RDP_RECEIVER_HPP__
#define __RDP_RECEIVER_HPP__
#include "rdp_bwe.hpp"
namespace rdp {
struct AckFeedbackIFace;

/**
 * ���ݰ������
 */
struct OutputerIFace
{
    virtual ~OutputerIFace(){}
    //��ʱ,�޷���ԭ�Ĳ�ְ�
    virtual void onSplitPackets_W(PacketPtr packet[], const int num) = 0;
    //���а�
    virtual void onOrderPackets_W(PacketPtr packet[], const int num, const bool ordered) = 0;
    //���ɿ���
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
    //R�߳�, ��ȡ����������
    virtual void onReceived_R(BitStream& bs, const Time& nowNS) = 0;
    //W�߳����Ը���
    virtual void update_W(const Time& nowNS) = 0;
    //д������Ack,�����ackRanges
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) = 0;

    //���ط��ʹ���bps
    virtual int tmmbrKbps_W(void) const = 0;
};

}
#endif
