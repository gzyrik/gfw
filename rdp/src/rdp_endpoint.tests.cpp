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
    virtual void onPackets_W(PacketPtr packet[], const int num){ echo(packet, num);}
    virtual void onExpiredPackets_W(PacketPtr packet[], const int num){ echo(packet, num); }
    virtual void onDisorderPacket_W(PacketPtr packet){ echo(&packet, 1);}
};
struct PipeLine : public transport::SenderIFace
{
    transport::ReceiverIFace* receriver_;
    int lostRate_;
    PipeLine(int lostRate):lostRate_(lostRate){}
    virtual void send_W(BitStream& bs, const Time& now) override{
        if (rand()%100 >= lostRate_)
            receriver_->onReceived_R(bs, now);
    }
};
}

TEST(EndPoint, FixedBandWidth)
{
    EchoPacket echo;
    PipeLine L1(50), L2(0); //P1发送丢包
    EndPoint P1(L1, echo), P2(L2, echo);
    L1.receriver_ = &P2;
    L2.receriver_ = &P1;

    P1.fixProtection(0,0xFF,false);//P1关闭FEC接收解码
    Thread T1, T2;
    T1.start(SharedPtr<Runnable>::ever(&P1));
    T2.start(SharedPtr<Runnable>::ever(&P2));

    Time now;
    char buf[15000];
    const int fixKbps = 400;
    P2.fixBandwidth(0, fixKbps);
    for(int i=0;i<100;++i) {
        now = Time::now();
        P1.postPacket(Packet::create(buf, fixKbps*1000/100, Packet::kUnreliable), now);
        Time::sleep(Time::MS(10));
    }
    SenderIFace::Statistics sendStats;
    ReceiverIFace::Statistics recvStats;
    Time::sleep(Time::MS(100));
    T1.join();
    T2.join();
    P1.sendStats(sendStats);
    P2.recvStats(recvStats);
}

