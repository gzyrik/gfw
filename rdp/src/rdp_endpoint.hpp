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
class EndPoint final: public Thread,
    public transport::ReceiverIFace
{
public:
    EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu=0);
    ~EndPoint();
    //I�߳�, ��������
    void sendPacket(const PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    //���ù̶���������
    void fixBandwidth(const bool fixed, const int kbps);

    void sendStats(SenderIFace::Statistics& stats) const { sender_->stats(stats); }
    void recvStats(rdp::ReceiverIFace::Statistics& stats) const { receiver_->stats(stats); }
public://transport::ReceiverIFace
    virtual void onReceived_R(BitStream& bs, const Time& now);
private:
    //W�߳�
    virtual void run();
    const int bitMTU_;
private:
    SharedPtr<SenderIFace> sender_;
    SharedPtr<rdp::ReceiverIFace> receiver_;
    transport::SenderIFace& transport_;
    struct BWECtrl final: public BWEIFace {
        BWECtrl() : bwe_(BWEIFace::create()), fixedBw_(false){}
        virtual ~BWECtrl() { delete bwe_; }
        BWEIFace* bwe_;
        bool fixedBw_;
        int  fixedBwKbps_;
        virtual void onReceived_R(const int bitLength, const Time& peerStampMS,
            const uint32_t messageNumber, const Time& now) override {
            bwe_->onReceived_R(bitLength, peerStampMS, messageNumber, now);
        }
        virtual int tmmbrKbps_W(const Time& rtt, const Time& now) override {
            return fixedBw_ ? fixedBwKbps_ : bwe_->tmmbrKbps_W(rtt, now);
        }
    } bwe_;
};

}
#endif
