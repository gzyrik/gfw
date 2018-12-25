#include "rdp_config.hpp"
#include "rdp_sender.hpp"
#include "rdp_packet.hpp"
#include "rdp_rwqueue.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_shared.hpp"
#include "rdp_thread.hpp"
namespace rdp {
namespace {

class Sender : public SenderIFace
{
    typedef RWQueue<PacketPtr> PacketRWQueue;
    typedef RWQueue<AckRangeList> AckRangeRWQueue;
    typedef std::deque<PacketPtr> PacketDeQueue;
private:
    //连接线程I->W
    PacketRWQueue       sendPacketSet_[kNumberOfPriority];
    //连接线程R->W
    AckRangeRWQueue     ackRangeQueue_;
private:
    PacketDeQueue       resendQueue_;

    uint16_t            splitPacketId_;
    Statistics          statistics_;
    const int           bitMTU_;
    //I线程,只是接口层payload发送码率,不包括包头,重发,ACK,FEC等
    SharedPtr<BitrateIFace> bitrate_I_;
    Time                betweenResendTime_;//重发间隔

    uint16_t messageNumber_;

    // kOrderable 序列计数
    uint16_t waitingForOrderedPacketWriteIndex_[0x100];
    volatile uint16_t resetingForOrderedPacketWriteIndex_[0x100];

    //kSequence 序列计数
    uint16_t waitingForSequencedPacketWriteIndex_[0x100];

    virtual void stats(Statistics& stats) const
    {
        stats = statistics_;
    }
    void markResend_W(AckRangeList& acks)
    {
        PacketDeQueue::const_iterator piter = resendQueue_.begin(); 
        for(AckRangeList::iterator iter = acks.begin();
            iter != acks.end() && piter != resendQueue_.end(); ++iter) {
            //移到范围
            while(piter != resendQueue_.end()
                && (*piter)->messageNumber < iter->first)
                ++piter;
            //标记已确认的包
            while(piter != resendQueue_.end()
                && (*piter)->messageNumber <= iter->second) {
                (*piter)->timeStamp = Time::zero;
                ++piter;
            }
        } 
    }
    //SenderIFace
    virtual void writePackets_W(BitStream& bs, const Time& nowNS)
    {
        {
            AckRangeList acks;
            while (ackRangeQueue_.pop(acks))
                markResend_W(acks); //ack 标记已确认包
        }
        {//按时间次序,重发过期包,时间0表示不再重发
            while (resendQueue_.size() > 0) {
                PacketPtr packet = resendQueue_.front();
                if (packet->reliability & Packet::kSequence) {//小于该值都无须重发
                    if (packet->orderingIndex < resetingForOrderedPacketWriteIndex_[packet->orderingChannel])
                        packet->timeStamp  = Time::zero;
                }

                if (packet->timeStamp == Time::zero) {
                    resendQueue_.pop_front();
                    packet->release();
                }
                else if(packet->timeStamp < nowNS) {
                    //超过重发时间，且没有收到ack,所以重发
                    if (!packet->writeToStream(bs))
                        return;
                    resendQueue_.pop_front();
                    statistics_.resentPackets++;
                    statistics_.bitResent += packet->bitLength;
                    packet->timeStamp = nowNS + betweenResendTime_;
                    resendQueue_.push_back(packet);
                }
                else
                    break;
            }
        }

        // 按优先级发送队列中的包
        for(int i=0; i<kNumberOfPriority; ++i)
        {
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

                if (packet->reliability & Packet::kReliable) {
                    packet->timeStamp = nowNS + betweenResendTime_;
                    if (packet->reliability & Packet::kSequence) {
                        if (packet->orderingIndex < resetingForOrderedPacketWriteIndex_[packet->orderingChannel])
                            packet->timeStamp  = Time::zero;
                    }
                    if (packet->timeStamp > nowNS)
                        resendQueue_.push_back(packet);
                }
            }
        }
    }
    void markPacket_I(PacketPtr packet) 
    {
        if (packet->reliability & Packet::kSequence) {
            if (packet->reliability & Packet::kReliable){
                packet->orderingIndex = waitingForOrderedPacketWriteIndex_[packet->orderingChannel] ++;
                if (packet->controlFlags & Packet::kResetable)
                    resetingForOrderedPacketWriteIndex_[packet->orderingChannel] = packet->orderingIndex;
            }
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
    void onAcks_R(AckRangeList& acks, const Time& rttAverage) override
    {
        if (acks.size()) {
            ackRangeQueue_.writeLock()->swap(acks);
            ackRangeQueue_.writeUnlock();
        }
        if (rttAverage > Time::MS(2500))
            betweenResendTime_ = Time::MS(2500);
        else if (rttAverage < Time::MS(500))
            betweenResendTime_ = Time::MS(500);
        else
            betweenResendTime_ = rttAverage;
    }
public:
    Sender(const int bitMTU):
        bitrate_I_(BitrateIFace::create()),
        bitMTU_(bitMTU),
        messageNumber_(0x7fff+(rand()%0x8000)),//至少从0x7fff开始
        betweenResendTime_(Time::S(1))
    {
        memset(&statistics_, 0, sizeof(statistics_));
        memset(waitingForOrderedPacketWriteIndex_, 0, sizeof(waitingForOrderedPacketWriteIndex_));
        memset((void*)resetingForOrderedPacketWriteIndex_, 0, sizeof(resetingForOrderedPacketWriteIndex_));
        memset(waitingForSequencedPacketWriteIndex_, 0, sizeof(waitingForSequencedPacketWriteIndex_));
    }
};
}//namespace

SenderIFace* SenderIFace::create(const int bitMTU)
{
    return new Sender(bitMTU);
}
}
