#ifndef __RDP_CHANNEL_HPP__
#define __RDP_CHANNEL_HPP__
#include "rdp_transport.hpp"
#include "rdp_sender.hpp"
#include "rdp_receiver.hpp"
#include "rdp_thread.hpp"
namespace rdp {
/**
 * �������� 
 * - I�߳�, ���÷��ͽӿ� Sender::sendPacket_I, ���뷢�Ͷ��� Sender::sendPacketSet
 * - R�߳�, ���ý��սӿ� Receiver::onReceived_R, ������ն��� Receiver::receivedQueue
 * -
 * - W�߳�, �ڲ��߳�
 *   #- �����Ͷ��в�д������ Sender::writeStream_W, Receiver::writeStream_W
 *   #- ������ն��в��������� Receiver::update_W, Receiver::Outputer::onPackets_W
 * -
 * - O�߳�, ���������ȡ������
 */
class Channel : public Thread,
    public transport::ReceiverIFace,
    public OutputerIFace,
    public AckFeedbackIFace,
    public BWEIFace
{
public:
    Channel(transport::SenderIFace& socket);
    ~Channel();
    //I�߳�, ��������
    void sendPacket(const PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    //���ù̶���������
    void fixBandwidth(const bool fixed, const int kbps);

    //���ط��ʹ���
    int kbps(void) const;
private:
    //W�߳�
    virtual void run();
private:
    SharedPtr<SenderIFace>      sender_;
    SharedPtr<rdp::ReceiverIFace>    receiver_;
    transport::SenderIFace&     transport_;
    int                         bitMTU_;
private:
    bool                        fixedBw_;
    int                         fixedBwKbps_;
    SharedPtr<BWEIFace>         bwe_;
    int                         tmmbrKbps_;

private://ReceiveSocketIFace
    virtual void onReceived_R(BitStream& bs, const Time& now);
private://OutputerIFace
    //��ʱ,�޷���ԭ�Ĳ�ְ�
    virtual void onSplitPackets_W(PacketPtr packet[], const int num);
    //���а�
    virtual void onOrderPackets_W(PacketPtr packet[], const int num, const bool ordered);
    //���ɿ���
    virtual void onPacket_W(PacketPtr packet, const bool sequence);

private://AckFeedbackIFace
    virtual void onAcks_R(AckRangeList& acks, const Time& rttAverage);
private://BwEstimatorIFace
    virtual void onReceived_R(const int bitLength, const Time& peerStampMS, const uint32_t messageNumber, const Time& now);
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now);
};

}
#endif
