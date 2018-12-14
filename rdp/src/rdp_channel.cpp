#include "rdp_config.hpp"
#include "rdp_channel.hpp"
#include "rdp_bitstream.hpp"
#include "rdp_sender.hpp"
#include "rdp_packet.hpp"
namespace rdp {
static void echo(PacketPtr packet[], int num)
{
    for(int i=0;i<num;++i) {
        //printf("%s\n",packet[i]->toString().c_str());
        packet[i]->release();
    }
}
void Channel::onSplitPackets_W(PacketPtr packet[], const int num)
{
    echo(packet, num);
}
void Channel::onOrderPackets_W(PacketPtr packet[], const int num, const bool ordered)
{
    echo(packet, num);
}
void Channel::onPacket_W(PacketPtr packet, const bool sequence)
{
    echo(&packet, 1);
}
void Channel::onReceived_R(BitStream& bs, const Time& now)
{
    receiver_->onReceived_R(bs, now);
}
void Channel::onAcks_R(AckRangeList& acks, const Time& rttAverage)
{
    sender_->onAcks_R(acks, rttAverage);
}
void Channel::onReceived_R(const int bitLength, const Time& peerStampMS, const uint32_t messageNumber, const Time& now)
{
    bwe_->onReceived_R(bitLength, peerStampMS, messageNumber, now);
}
int Channel::tmmbrKbps_W(const Time& rtt, const Time& now)
{
    if (fixedBw_)
        return fixedBwKbps_;
    return bwe_->tmmbrKbps_W(rtt, now);
}
Channel::Channel(transport::SenderIFace& socket) :
    bitMTU_(8*kMaxMtuBytes),
    transport_(socket),
    fixedBw_(false),
    tmmbrKbps_(kMaxBandwidthKbps),
    bwe_(BWEIFace::create())
{
	sender_.reset(SenderIFace::create(bitMTU_));
    receiver_.reset(rdp::ReceiverIFace::create(*this, *this, *this));
}
Channel::~Channel()
{
    join();
}
void Channel::sendPacket(const PacketPtr packet, const Time& now, const int priority)
{
    sender_->sendPacket_I(packet, (SenderIFace::Priority)priority, now);
}
void Channel::fixBandwidth(const bool fixed, const int kbps)
{
    fixedBw_ = fixed;
    fixedBwKbps_ = kbps;
}
int Channel::kbps(void) const
{
    return tmmbrKbps_;
}
void Channel::run()
{
    uint8_t buf[kMaxMtuBytes];
    Time lastNS = Time::now();
    while(state() == kRunning) {
        //kbitMTU = bitMTU_/1000;
        //Hz = tmmbrKbps/kbitMTU;
        //Ms = 1000/Hz = bitMTU_/tmmbrKbps;
		if (tmmbrKbps_ <= 0)
			Time::sleep(Time::MS(1000));
        else if (bitMTU_ >= tmmbrKbps_)
			Time::sleep(Time::MS(bitMTU_/tmmbrKbps_));
        else
            Time::sleep(Time::MS(1));

        const Time& now = Time::now();
        receiver_->update_W(now);
        memset(buf, 0, sizeof(buf));
        BitStream bs(buf, bitsof(buf));
        tmmbrKbps_ = receiver_->tmmbrKbps_W();

        const Time deltaNS(now - lastNS);
        int64_t bitLength = tmmbrKbps_ * deltaNS.millisec();
        if (bitLength < bs.bitSize)
            bs.bitSize = bitLength;

        receiver_->writeAcks_W(bs, now);
        sender_->writePackets_W(bs, now);
        transport_.send_W(bs, now);
        lastNS = now;
    }
}
}
