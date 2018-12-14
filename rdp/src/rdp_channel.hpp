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
class Channel : public Thread,
    public transport::ReceiverIFace,
    public OutputerIFace,
    public AckFeedbackIFace,
    public BWEIFace
{
public:
    Channel(transport::SenderIFace& socket);
    ~Channel();
    //I线程, 发送数据
    void sendPacket(const PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    //设置固定接收码率
    void fixBandwidth(const bool fixed, const int kbps);

    //返回发送带宽
    int kbps(void) const;
private:
    //W线程
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
    //超时,无法复原的拆分包
    virtual void onSplitPackets_W(PacketPtr packet[], const int num);
    //序列包
    virtual void onOrderPackets_W(PacketPtr packet[], const int num, const bool ordered);
    //不可靠包
    virtual void onPacket_W(PacketPtr packet, const bool sequence);

private://AckFeedbackIFace
    virtual void onAcks_R(AckRangeList& acks, const Time& rttAverage);
private://BwEstimatorIFace
    virtual void onReceived_R(const int bitLength, const Time& peerStampMS, const uint32_t messageNumber, const Time& now);
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now);
};

}
#endif
