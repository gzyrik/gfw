#include "rdp_config.hpp"
#include "rdp_endpoint.hpp"
#include "rdp_bitstream.hpp"
#include "rdp_sender.hpp"
#include "rdp_packet.hpp"
namespace rdp {
void EndPoint::onReceived_R(BitStream& bs, const Time& now)
{
    receiver_->onReceived_R(bs, now);
}
EndPoint::EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu) :
    bitMTU_(8*(mtu ? mtu : kMaxMtuBytes)),
    transport_(socket)
{
    sender_.reset(SenderIFace::create(bitMTU_));
    receiver_.reset(rdp::ReceiverIFace::create(*sender_, output, bwe_));
}
EndPoint::~EndPoint()
{
    join();
}
void EndPoint::sendPacket(const PacketPtr packet, const Time& now, const int priority)
{
    if (state() == kJoining) return;
    sender_->sendPacket_I(packet, (SenderIFace::Priority)priority, now);
}
void EndPoint::fixBandwidth(const int minKbps, const int maxKbps)
{
    bwe_.minBwKbps_ = minKbps;
    bwe_.maxBwKbps_ = maxKbps;
}
void EndPoint::run()
{//W线程
    uint8_t buf[kMaxMtuBytes];
    Time lastNS = Time::now();
    ReceiverIFace::Statistics stats={0};
    while(1) {
        //kbitMTU = bitMTU_/1000;
        //Hz = tmmbrKbps/kbitMTU;
        //Ms = 1000/Hz = bitMTU_/tmmbrKbps;
        if (stats.bwKbps <= 0)
            Time::sleep(Time::MS(1000));
        else if (bitMTU_ >= stats.bwKbps)
            Time::sleep(Time::MS(bitMTU_/stats.bwKbps));
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
        int bitWrite = bs.bitWrite;
        sender_->writePackets_W(bs, now);
        transport_.send_W(bs, now);
        if (bitWrite == bs.bitWrite && state()==kJoining)
            break;//等待清空发送才退出
        lastNS = now;
    }
}
}
