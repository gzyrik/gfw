#ifndef __RDP_CHANNEL_HPP__
#define __RDP_CHANNEL_HPP__
#include "rdp_transport.hpp"
#include "rdp_sender.hpp"
#include "rdp_receiver.hpp"
#include "rdp_thread.hpp"
#include "rdp_fec.hpp"
namespace rdp {
class EndPoint final: public Runnable, public transport::ReceiverIFace
{
    typedef rdp::ReceiverIFace ReceiverIFace;
public:

    /** 构造通信端点
     * @note  socket 和 output 将工作在相同的独立线程(_W)中
     *
     * @param[in] socket 网络发送者
     * @param[in] output 数据接收者
     * @param[in] mtu    网络包的最大字节数
     */
    EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu=0);
    ~EndPoint();


    /** 放入需发送的包
     * @note 注意非线程安全, 必须处于某个固定线程或同步处理
     */
    void postPacket(PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    /** 接收到网络数据, 
     * @note 注意非线程安全, 必须处于某个固定线程或同步处理
     */
    virtual void onReceived_R(BitStream& bs, const Time& now) override
    { fec_.onReceived_R(bs, now); }

    //固定接收码率的范围
    void fixBandwidth(const int minBwKbps, const int maxBwKbps)
    { bwe_.fixKbps(minBwKbps, maxBwKbps); }

    //固定FEC比例的范围
    void fixProtection(const uint8_t minFecRate, const uint8_t maxFecRate, const bool enableDecode = true)
    { fec_.fixRate(minFecRate, maxFecRate, enableDecode); }

    void sendStats(SenderIFace::Statistics& stats) const
    { sender_.stats(stats); }

    void recvStats(ReceiverIFace::Statistics& stats) const
    { receiver_.stats(stats); }
private:
    const int mtu_;
    BWEIFace& bwe_;
    SenderIFace& sender_;
    ReceiverIFace& receiver_;
    FECIFace& fec_;
private:
    //W线程
    virtual void run() override;
};

}
#endif
