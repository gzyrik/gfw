#ifndef __RDP_SENDER_HPP__
#define __RDP_SENDER_HPP__
#include "rdp_rangelist.hpp"
#include "rdp_basictypes.hpp"
namespace rdp {

/** ACK ��Χ���� */
typedef RangeList<uint16_t> AckRangeList;

/**
 * ACK������
 */
struct AckFeedbackIFace
{
    virtual ~AckFeedbackIFace(){}
    //R�߳�,�յ�ACK, ��Ҫ��������ط���
    virtual void onAcks_R(AckRangeList& acks, const Time& rttAverage) = 0;
};

/**
 * TODO
 * ���Ͷ˿��ܷ��͵Ĳ���
 * - PH �ɱ��ͷ
 * - PD payload ����
 * - RH �ط��Ŀɱ��ͷ
 * - RD �ط���payload����
 * - AK Ack����
 * �����Ч���� = TMMBR - (PH + RH + RD) - AK
 * TMMBR ��ReceiverIFace::tmmbrKbps_W()����
 * AK ��ReceiverIFace::ackKbps_W()����
 *
 */
struct SenderIFace : public AckFeedbackIFace
{
    enum Priority
    {
        kSystem = 0,
        kHigh,
        kMedium,
        kLow,
        kNumberOfPriority
    };
    struct Statistics
    {
        int splitPackets;
        int unsplitPackets;

        int resentPackets;
        int bitResent;

        ///���ķ��͸���
        int sentPackets[kNumberOfPriority];

        ///payload�ķ���λ��
        int bitSent[kNumberOfPriority];
    };

    virtual void stats(Statistics& stats) const = 0;
    //W�߳�,д���(�ط�������),�������緢��
    virtual void writePackets_W(BitStream& bs, const Time& nowNS) = 0;
    //I�߳�,�������ݱ�,�û���ӿ�.
    virtual void sendPacket_I(PacketPtr packet, const enum Priority priority, const Time& nowNS) = 0;
    //��־
    virtual void markSplitPacket_I(PacketPtr packet) = 0;

    static SenderIFace* create(const int bitMTU);
};

}
#endif
