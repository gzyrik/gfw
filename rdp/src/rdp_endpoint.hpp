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

    /** 构造数据端点
     * @note  socket 和 output 工作在相同的独立线程中
     *
     * @param[in] socket 网络发送者
     * @param[in] output 数据接收者
     * @param[in] mtu    网络包的最大字节数
     */
    EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu=0);
    ~EndPoint();


    /** 放入需发送的包,必须处于某个固定线程或同步处理 */
    void sendPacket(PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    /** 接收网络数据, 必须处于某个固定线程或同步处理 */
    virtual void onReceived_R(BitStream& bs, const Time& now) override;

    //设置接收码率范围
    void fixBandwidth(const int minKbps, const int maxKbps);

    void sendStats(SenderIFace::Statistics& stats) const { sender_->stats(stats); }
    void recvStats(ReceiverIFace::Statistics& stats) const { receiver_->stats(stats); }//TODO
private:
    //W线程
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
