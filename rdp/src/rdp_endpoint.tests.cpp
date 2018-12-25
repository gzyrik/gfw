#include "rdp_endpoint.hpp"
#include "rdp_rwqueue.hpp"
#include "rdp_packet.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_bitstream.hpp"
#include <gtest/gtest.h>

using namespace rdp;
namespace {
struct EchoPacket : public OutputerIFace
{
    void echo(PacketPtr packet[], int num)
    {
        for(int i=0;i<num;++i) {
            //printf("%s\n",packet[i]->toString().c_str());
            packet[i]->release();
        }
    }
    virtual void onSplitPackets_W(PacketPtr packet[], const int num){ echo(packet, num); }
    virtual void onOrderPackets_W(PacketPtr packet[], const int num, const bool ordered){ echo(packet, num);}
    virtual void onPacket_W(PacketPtr packet, const bool sequence){echo(&packet, 1);}
};
struct PipeLine : public transport::SenderIFace
{
    transport::ReceiverIFace* peer_;
    virtual void send_W(BitStream& bs, const Time& now) override{
        peer_->onReceived_R(bs, now);
    }
};
}

TEST(EndPoint, FixedBandWidth)
{
    EchoPacket echo;
    PipeLine L1, L2;
    EndPoint P1(L1, echo), P2(L2, echo);
    L1.peer_ = &P2;
    L2.peer_ = &P1;
    P1.start();
    P2.start();

    Time now;
    char buf[15000];
    const int fixKbps = 400;
    P2.fixBandwidth(0, fixKbps);
    for(int i=0;i<100;++i) {
        now = Time::now();
        P1.sendPacket(Packet::Reliable(buf, fixKbps*1000/100), now);
        Time::sleep(Time::MS(10));
    }
    SenderIFace::Statistics sendStats;
    rdp::ReceiverIFace::Statistics recvStats;
    Time::sleep(Time::MS(100));
    P1.join();
    P2.join();
    P1.sendStats(sendStats);
    P2.recvStats(recvStats);
}

