#include "rdp_config.hpp"
#include "rdp_endpoint.hpp"
#include "rdp_bitstream.hpp"
#include "rdp_sender.hpp"
#include "rdp_packet.hpp"
namespace rdp {
void EndPoint::onReceived_R(BitStream& bs, const Time& now)
{
    if (fec_) fec_->onReceived_R(bs, now);
}
EndPoint::EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu) :
    mtu_(mtu > 0 ? mtu : kMaxMtuBytes),
    sender_(SenderIFace::create(mtu_)),
    receiver_(rdp::ReceiverIFace::create(*sender_, output, bwe_)),
    fec_(FecIFace::create(socket, *receiver_, mtu_))
{
}
EndPoint::~EndPoint()
{
    join();
    delete fec_;
    delete receiver_;
    delete sender_;
}
void EndPoint::sendPacket(PacketPtr packet, const Time& now, const int priority)
{
    if (state() == kJoining) return;
    sender_->sendPacket_I(packet, (SenderIFace::Priority)priority, now);
}
void EndPoint::fixBandwidth(const int minKbps, const int maxKbps)
{
    bwe_.minBwKbps_ = minKbps;
    bwe_.maxBwKbps_ = maxKbps;
}
/*
 * 处理流程 
 * - I线程, 调用发送接口 Sender::sendPacket_I, 放入发送队列 Sender::sendPacketSet
 * - R线程, 调用接收接口 Receiver::onReceived_R, 放入接收队列 Receiver::receivedQueue
 * - W线程, 内部线程
 *   #- 处理发送队列并写入网络 Sender::writeStream_W, Receiver::writeStream_W
 *   #- 处理接收队列并导出数据 Receiver::update_W, Receiver::Outputer::onPackets_W
 */
void EndPoint::run()
{//W线程
    uint8_t buf[kMaxMtuBytes];
    Time lastNS = Time::now();
    ReceiverIFace::Statistics stats={0};
    const int bitMTU = mtu_*8;
    while(1) {
        //kbitMTU = bitMTU_/1000;
        //Hz = tmmbrKbps/kbitMTU;
        //Ms = 1000/Hz = bitMTU_/tmmbrKbps;
        if (stats.bwKbps <= 0)
            Time::sleep(Time::MS(1000));
        else if (bitMTU >= stats.bwKbps)
            Time::sleep(Time::MS(bitMTU/stats.bwKbps));
        else
            Time::sleep(Time::MS(1));

        const Time& now = Time::now();
        receiver_->update_W(now);
        memset(buf, 0, sizeof(buf));
        BitStream bs(buf, bitsof(buf));
        receiver_->stats(stats);

        const Time deltaNS(now - lastNS);
        int64_t bitLength = stats.bwKbps * deltaNS.millisec();
        if (bitLength < bs.bitSize)
            bs.bitSize = (int)bitLength;

        receiver_->writeAcks_W(bs, now);
        sender_->writePackets_W(bs, now);
        if (bs.bitWrite > 5) //理论最小包: kVersion[2]+NoACK+NoNACK+NoRTT
            fec_->send_W(bs, now);
        else if(state()==kJoining)
            break;//等待清空发送才退出
        lastNS = now;
    }
}
}
