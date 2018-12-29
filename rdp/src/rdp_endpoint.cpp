#include "rdp_config.hpp"
#include "rdp_endpoint.hpp"
#include "rdp_bitstream.hpp"
#include "rdp_sender.hpp"
#include "rdp_packet.hpp"
namespace rdp {
EndPoint::EndPoint(transport::SenderIFace& socket, OutputerIFace& output, const int mtu) :
    mtu_(mtu > 0 ? mtu : kMaxMtuBytes),
    bwe_(*BWEIFace::create()),
    sender_(*SenderIFace::create(mtu_*8)),
    receiver_(*rdp::ReceiverIFace::create(sender_, output, bwe_)),
    fec_(*FECIFace::create(socket, receiver_))
{}
EndPoint::~EndPoint()
{
    join();
    delete &fec_;
    delete &receiver_;
    delete &sender_;
    delete &bwe_;
}
void EndPoint::sendPacket(PacketPtr packet, const Time& now, const int priority)
{
    if (state() == kJoining) return;
    sender_.sendPacket_I(packet, (SenderIFace::Priority)priority, now);
}
/*
 * �������� 
 * - I�߳�, ���÷��ͽӿ� Sender::sendPacket_I, ���뷢�Ͷ��� Sender::sendPacketSet
 * - R�߳�, ���ý��սӿ� Receiver::onReceived_R, ������ն��� Receiver::receivedQueue
 * - W�߳�, �ڲ��߳�
 *   #- �����Ͷ��в�д������ Sender::writeStream_W, Receiver::writeStream_W
 *   #- ������ն��в��������� Receiver::update_W, Receiver::Outputer::onPackets_W
 */
void EndPoint::run()
{//W�߳�
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
        receiver_.update_W(now);
        memset(buf, 0, sizeof(buf));
        BitStream bs(buf, bitsof(buf));
        receiver_.stats(stats);

        const Time deltaNS(now - lastNS);
        int64_t bitLength = stats.bwKbps * deltaNS.millisec();
        if (bitLength < bs.bitSize)
            bs.bitSize = (int)bitLength;

        fec_.markFec_W(bs, now);
        receiver_.writeAcks_W(bs, now);
        sender_.writePackets_W(bs, now);
        if (bs.bitWrite > 20) //������С��: SeqNo[16]+NoACK+NoNACK+NoRTT
            fec_.send_W(bs, now);
        else if(state()==kJoining)
            break;//�ȴ���շ��Ͳ��˳�
        lastNS = now;
    }
}
}
