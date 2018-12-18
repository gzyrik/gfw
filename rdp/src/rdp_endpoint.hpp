#ifndef __RDP_CHANNEL_HPP__
#define __RDP_CHANNEL_HPP__
#include "rdp_transport.hpp"
#include "rdp_sender.hpp"
#include "rdp_receiver.hpp"
#include "rdp_thread.hpp"
namespace rdp {
/**
 * 处理流程 
 * - I线程, 调用发送接口 Sender::sendPacket_I, 放入发送队列 Sender::sendPacketSet
 * - R线程, 调用接收接口 Receiver::onReceived_R, 放入接收队列 Receiver::receivedQueue
 * -
 * - W线程, 内部线程
 *   #- 处理发送队列并写入网络 Sender::writeStream_W, Receiver::writeStream_W
 *   #- 处理接收队列并导出数据 Receiver::update_W, Receiver::Outputer::onPackets_W
 * -
 * - O线程, 从输出者中取出数据
 */
class EndPoint final: public Thread,
    public transport::ReceiverIFace
{
public:
    EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu=0);
    ~EndPoint();
    //I线程, 发送数据
    void sendPacket(const PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    //设置固定接收码率
    void fixBandwidth(const bool fixed, const int kbps);

    void sendStats(SenderIFace::Statistics& stats) const { sender_->stats(stats); }
    void recvStats(rdp::ReceiverIFace::Statistics& stats) const { receiver_->stats(stats); }
public://transport::ReceiverIFace
    virtual void onReceived_R(BitStream& bs, const Time& now);
private:
    //W线程
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
