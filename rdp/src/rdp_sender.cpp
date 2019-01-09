#include "rdp_config.hpp"
#include "rdp_sender.hpp"
#include "rdp_packet.hpp"
#include "rdp_rwqueue.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_shared.hpp"
#include "rdp_thread.hpp"
#include <list>
namespace rdp {
namespace {

class Sender : public SenderIFace
{
    typedef RWQueue<PacketPtr> PacketRWQueue;
    typedef RWQueue<AckRangeList> AckRangeRWQueue;
    typedef std::list<PacketPtr> PacketQueue;
private:
    //�����߳�I->W
    PacketRWQueue       sendPacketSet_[kNumberOfPriority];
    //�����߳�R->W
    AckRangeRWQueue     ackRangeQueue_, nackRangeQueue_;
private:
    PacketQueue         resendQueue_;//��ʱ��������ط�����[ACK ����]

    uint16_t            splitPacketId_;
    Statistics          statistics_;
    const int           bitMTU_;
    //I�߳�,ֻ�ǽӿڲ�payload��������,��������ͷ,�ط�,ACK,FEC��
    BitrateIFace        *bitrate_I_;
    Time                betweenResendTime_;//ACK�ط����

    uint32_t messageNumber_;

    // kOrderable ���м���
    uint32_t waitingForOrderedPacketWriteIndex_[0x100];
    //kSequence ���м���
    uint32_t waitingForSequencedPacketWriteIndex_[0x100];

    virtual void stats(Statistics& stats) const override
    {
        stats = statistics_;
    }
    enum { kResendMark = 8 };
    void markResend_W(AckRangeList& acks, PacketQueue& queue)
    {
        PacketQueue::const_iterator piter = queue.begin(); 
        for(AckRangeList::iterator iter = acks.begin();
            iter != acks.end() && piter != queue.end(); ++iter) {
            //�Ƶ���Χ
            while(piter != queue.end()
                && (*piter)->messageNumber < iter->first)
                ++piter;
            //�����ȷ�ϵİ�
            while(piter != queue.end()
                && (*piter)->messageNumber <= iter->second)
                (*piter++)->reliability |= kResendMark;
        } 
    }
    //SenderIFace
    virtual void writePackets_W(BitStream& bs, const Time& nowNS) override
    {
        AckRangeList acks;

        while (ackRangeQueue_.pop(acks)) //�ȸ��� ACK, ����ط���
            markResend_W(acks, resendQueue_);

        while (nackRangeQueue_.pop(acks)) //���� nack ���ǿ���ط���
            markResend_W(acks, resendQueue_);

        {//�ط�������
            PacketQueue::iterator iter = resendQueue_.begin();
            while (iter != resendQueue_.end()) {
                bool toErase, bResend;
                PacketQueue::iterator cur = iter++;
                PacketPtr packet = *cur;
                if (packet->reliability & Packet::kReliable) {
                    toErase = (packet->reliability & kResendMark) != 0;//ACK,�����
                    bResend = (packet->timeStamp <= nowNS);//��ʱ,���ط�
                }
                else {
                    bResend = (packet->reliability & kResendMark) != 0;//NACK,���ط�
                    toErase = (packet->timeStamp <= nowNS); //��ʱ,�����
                }

                if (toErase) {
                    resendQueue_.erase(cur);
                    packet->release();
                }
                else if(bResend) {
                    if (!packet->writeToStream(bs))
                        return;
                    packet->reliability &= ~kResendMark;
                    if (packet->reliability & Packet::kReliable)
                        packet->timeStamp = nowNS + betweenResendTime_;
                    statistics_.resentPackets++;
                    statistics_.bitResent += packet->bitLength;
                }
            }
        }

        // �����ȼ����Ͷ����еİ�
        for(int i=0; i<kNumberOfPriority; ++i) {
            PacketPtr* ppacket;
            PacketRWQueue& queue = sendPacketSet_[i];
            while ((ppacket = queue.readLock())) {
                PacketPtr packet = *ppacket;
                packet->messageNumber = messageNumber_;
                if (!packet->writeToStream(bs)){
                    queue.cancelRead(ppacket);
                    return;
                }
                queue.readUnlock();
                messageNumber_++;
                statistics_.sentPackets[i]++;
                statistics_.bitSent[i] += packet->bitLength;

                if (packet->reliability & Packet::kReliable)
                    packet->timeStamp = nowNS + betweenResendTime_;
                else
                    packet->timeStamp = nowNS + Time::MS(kExpiredMs);

                resendQueue_.push_back(packet);
            }
        }
    }
    void markPacket_I(PacketPtr packet) 
    {
        if (packet->reliability & Packet::kSequence) {
            if (packet->reliability == Packet::kOrderable)
                packet->orderingIndex = waitingForOrderedPacketWriteIndex_[packet->orderingChannel] ++;
            else
                packet->orderingIndex = waitingForSequencedPacketWriteIndex_[packet->orderingChannel] ++;
        }
    }
    virtual void sendPacket_I(PacketPtr packet, const enum Priority priority, const Time& nowNS) override
    {
        bitrate_I_->update(packet->bitLength, nowNS);
        markPacket_I(packet);
        if (packet->bitSize() < bitMTU_) {
            sendPacketSet_[priority].push(packet);
        }
        else {
            statistics_.splitPackets++;
            const uint16_t splitPacketId = splitPacketId_++;
            const int maxSendBitBlock = bitMTU_ - packet->headerBitSize(true);
            const int splitPacketCount = (packet->bitLength - 1)/maxSendBitBlock + 1;
            int bitLength = packet->bitLength;
            for(int i=0; i<splitPacketCount; ++i, bitLength -= maxSendBitBlock) {
                PacketPtr splitPacket = packet->clone(i*maxSendBitBlock, bitLength > maxSendBitBlock ? maxSendBitBlock : bitLength);
                splitPacket->splitPacketId = splitPacketId;
                splitPacket->splitPacketIndex = i;
                splitPacket->splitPacketCount = splitPacketCount;
                sendPacketSet_[priority].push(splitPacket);
            }
            packet->release();
        }
    }
    //AckFeedbackIFace
    void onAcks_R(AckRangeList& acks, AckRangeList& nacks, const Time& rttAverage) override
    {
        if (!acks.empty()) {
            ackRangeQueue_.writeLock()->swap(acks);
            ackRangeQueue_.writeUnlock();
        }
        if (!nacks.empty()) {
            nackRangeQueue_.writeLock()->swap(nacks);
            nackRangeQueue_.writeUnlock();
        }
        if (rttAverage > Time::MS(kAckMinMs))
            betweenResendTime_ = Time::MS(kAckMinMs);
        else if (rttAverage < Time::MS(kAckMaxMs))
            betweenResendTime_ = Time::MS(kAckMaxMs);
        else
            betweenResendTime_ = rttAverage;
    }
public:
    Sender(const int bitMTU) :
        bitrate_I_(BitrateIFace::create()),
        bitMTU_(bitMTU),
        messageNumber_(0),
        betweenResendTime_(Time::S(1))
    {
        memset(&statistics_, 0, sizeof(statistics_));
        memset(waitingForOrderedPacketWriteIndex_, 0, sizeof(waitingForOrderedPacketWriteIndex_));
        memset(waitingForSequencedPacketWriteIndex_, 0, sizeof(waitingForSequencedPacketWriteIndex_));
    }
    ~Sender()
    {
        delete bitrate_I_;
    }
};
}//namespace

SenderIFace* SenderIFace::create(const int bitMTU)
{
    return new Sender(bitMTU);
}
}
