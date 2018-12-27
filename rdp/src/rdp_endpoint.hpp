#ifndef __RDP_CHANNEL_HPP__
#define __RDP_CHANNEL_HPP__
#include "rdp_transport.hpp"
#include "rdp_sender.hpp"
#include "rdp_receiver.hpp"
#include "rdp_thread.hpp"
#include "rdp_fec.hpp"
namespace rdp {
class EndPoint final: public Thread, public transport::ReceiverIFace
{
    typedef rdp::ReceiverIFace ReceiverIFace;
public:

    /** �������ݶ˵�
     * @note  socket �� output ��������ͬ�Ķ����߳���
     *
     * @param[in] socket ���緢����
     * @param[in] output ���ݽ�����
     * @param[in] mtu    �����������ֽ���
     */
    EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu=0);
    ~EndPoint();


    /** �����跢�͵İ�,���봦��ĳ���̶��̻߳�ͬ������ */
    void sendPacket(PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    /** ������������, ���봦��ĳ���̶��̻߳�ͬ������ */
    virtual void onReceived_R(BitStream& bs, const Time& now) override;

    //���ý������ʷ�Χ
    void fixBandwidth(const int minKbps, const int maxKbps);

    void sendStats(SenderIFace::Statistics& stats) const { sender_->stats(stats); }
    void recvStats(ReceiverIFace::Statistics& stats) const { receiver_->stats(stats); }//TODO
private:
    //W�߳�
    virtual void run();
    const int mtu_;
private:
    SenderIFace* sender_;
    ReceiverIFace* receiver_;
    FecIFace*  fec_;
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
