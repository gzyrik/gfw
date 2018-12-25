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
    typedef rdp::ReceiverIFace ReceiverIFace;
public:
    EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu=0);
    ~EndPoint();
    //I�߳�, ��������
    void sendPacket(const PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    //���ý������ʷ�Χ
    void fixBandwidth(const int minKbps, const int maxKbps);

    void sendStats(SenderIFace::Statistics& stats) const { sender_->stats(stats); }
    void recvStats(ReceiverIFace::Statistics& stats) const { receiver_->stats(stats); }//TODO
public://transport::ReceiverIFace
    virtual void onReceived_R(BitStream& bs, const Time& now);
private:
    //W�߳�
    virtual void run();
    const int bitMTU_;
private:
    SharedPtr<SenderIFace> sender_;
    SharedPtr<ReceiverIFace> receiver_;
    transport::SenderIFace& transport_;
    struct BWECtrl final: public BWEIFace {
        BWEIFace* bwe_;
        int  minBwKbps_, maxBwKbps_;
        BWECtrl() : bwe_(BWEIFace::create()), minBwKbps_(0), maxBwKbps_(0)
        {}
        virtual ~BWECtrl() { delete bwe_; }
        virtual void onReceived_R(const int bitLength, const Time& peerStampMS,
            const int recvKbps, const Time& now) override {
            bwe_->onReceived_R(bitLength, peerStampMS, recvKbps, now);
        }
        virtual int tmmbrKbps_W(const Time& rtt, const Time& now) override {
            int kbps = bwe_->tmmbrKbps_W(rtt, now);
            if (kbps < minBwKbps_) kbps = minBwKbps_ ;
            if (maxBwKbps_ && kbps > maxBwKbps_) kbps = maxBwKbps_;
            return kbps;
        }
    } bwe_;
};

}
#endif
