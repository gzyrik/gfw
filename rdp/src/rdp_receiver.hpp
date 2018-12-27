#ifndef __RDP_RECEIVER_HPP__
#define __RDP_RECEIVER_HPP__
#include "rdp_bwe.hpp"
#include "rdp_transport.hpp"
namespace rdp {
struct AckFeedbackIFace;

/**
 * ���ݰ������
 */
struct OutputerIFace
{
    virtual ~OutputerIFace(){}
    virtual void onPackets_W(PacketPtr packet[], const int num) = 0;
    //��ʱ�޷���ԭ�Ĳ�ְ�
    virtual void onExpiredPackets_W(PacketPtr packet[], const int num) = 0;
    //�����������а�
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

        int ackBitSent; //�ѷ��͵�ACK����
        int nackBitSent; //�ѷ��͵�NACK����
        int ackTotalBitSent;//����ķ�������(ACK+NACK+RTT+TMMBR)

        int receivedPackets;
        int duplicateReceivedPackets;

        //���ʹ���
        int bwKbps;
    };

    virtual void stats(Statistics& stats) const = 0;

    //W�߳����Ը���
    virtual void update_W(const Time& nowNS) = 0;

    //д������Ack,�����ackRanges
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) = 0;

    static ReceiverIFace* create(AckFeedbackIFace& ack, OutputerIFace& out, BWEIFace& bwe);
    virtual ~ReceiverIFace(){}
};

}
#endif
