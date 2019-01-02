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

    /** ����ͨ�Ŷ˵�
     * @note  socket �� output ����������ͬ�Ķ����߳�(_W)��
     *
     * @param[in] socket ���緢����
     * @param[in] output ���ݽ�����
     * @param[in] mtu    �����������ֽ���
     */
    EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu=0);
    ~EndPoint();


    /** �����跢�͵İ�
     * @note ע����̰߳�ȫ, ���봦��ĳ���̶��̻߳�ͬ������
     */
    void postPacket(PacketPtr packet, const Time& now, const int priority = SenderIFace::kMedium);

    /** ���յ���������, 
     * @note ע����̰߳�ȫ, ���봦��ĳ���̶��̻߳�ͬ������
     */
    virtual void onReceived_R(BitStream& bs, const Time& now) override
    { fec_.onReceived_R(bs, now); }

    //�̶��������ʵķ�Χ
    void fixBandwidth(const int minBwKbps, const int maxBwKbps)
    { bwe_.fixKbps(minBwKbps, maxBwKbps); }

    //�̶�FEC�����ķ�Χ
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
    //W�߳�
    virtual void run() override;
};

}
#endif
