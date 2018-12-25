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
    //连接线程I->W
    PacketRWQueue       sendPacketSet_[kNumberOfPriority];
    //连接线程R->W
    AckRangeRWQueue     ackRangeQueue_, nackRangeQueue_;
private:
    PacketQueue         resendQueue_;//按时间的主动重发队列[ACK 机制]
    PacketQueue         histroyQueue_;//收到请求而重发队列[NACK 机制]

    uint16_t            splitPacketId_;
    Statistics          statistics_;
    const int           bitMTU_;
    //I线程,只是接口层payload发送码率,不包括包头,重发,ACK,FEC等
    BitrateIFace        *bitrate_I_;
    Time                betweenResendTime_;//重发间隔

    uint32_t messageNumber_;

    // kOrderable 序列计数
    uint32_t waitingForOrderedPacketWriteIndex_[0x100];
    //kSequence 序列计数
    uint32_t waitingForSequencedPacketWriteIndex_[0x100];

    virtual void stats(Statistics& stats) const override
    {
        stats = statistics_;
    }
    void markResend_W(AckRangeList& acks, PacketQueue& queue)
    {
        PacketQueue::const_iterator piter = queue.begin(); 
        for(AckRangeList::iterator iter = acks.begin();
            iter != acks.end() && piter != queue.end(); ++iter) {
            //移到范围
            while(piter != queue.end()
                && (*piter)->messageNumber < iter->first)
                ++piter;
            //标记已确认的包
            while(piter != queue.end()
                && (*piter)->messageNumber <= iter->second)
                (*piter++)->timeStamp = Time::zero;
        } 
    }
     void markResend_W(AckRangeList& acks)
    {
        PacketQueue::const_iterator piter = resendQueue_.begin(); 
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
    virtual void writePackets_W(BitStream& bs, const Time& nowNS) override
    {
        AckRangeList acks;

        //先根据 ACK, 清除重发包
        while (ackRangeQueue_.pop(acks))
            markResend_W(acks, resendQueue_);
        {//按时间次序,重发过期包,时间0表示不再重发
            while (resendQueue_.size() > 0) {
                PacketPtr packet = resendQueue_.front();
                if (packet->timeStamp == Time::zero) {
                    resendQueue_.pop_front();
                    packet->release();
                }
                else if(packet->timeStamp < nowNS) {
                    //超过重发时间，且没有收到ack,所以重发
                    if (!packet->writeToStream(bs))
                        return;
                    resendQueue_.pop_front();
                    statistics_.resentAckPackets++;
                    statistics_.ackBitResent += packet->bitLength;
                    packet->timeStamp = nowNS + betweenResendTime_;
                    resendQueue_.push_back(packet);
                }
                else
                    break;
            }
        }

        //根据 nack 标记请求包
        acks.clear();
        while (nackRangeQueue_.pop(acks))
            markResend_W(acks, histroyQueue_);
        {// 遍历清除过期包,并重发请求包,时间0表示要重发
            PacketQueue::iterator iter = histroyQueue_.begin();
            while (iter != histroyQueue_.end()) {
                PacketQueue::iterator cur = iter++;
                PacketPtr packet = *cur;
                if(packet->timeStamp == Time::zero) {
                    if (!packet->writeToStream(bs))
                        return;
                    statistics_.resentNackPackets++;
                    statistics_.nackBitResent += packet->bitLength;
                    packet->timeStamp = nowNS + betweenResendTime_;
                }
                else if (packet->timeStamp < nowNS)
                    histroyQueue_.erase(cur);
                else if (acks.empty())
                    break;
            }
        }

        // 按优先级发送队列中的包
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

                packet->timeStamp = nowNS + betweenResendTime_;
                if (packet->reliability & Packet::kReliable)
                    resendQueue_.push_back(packet);
                else 
                    histroyQueue_.push_back(packet);
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
        if (rttAverage > Time::MS(2500))
            betweenResendTime_ = Time::MS(2500);
        else if (rttAverage < Time::MS(500))
            betweenResendTime_ = Time::MS(500);
        else
            betweenResendTime_ = rttAverage;
    }
public:
    Sender(const int mtu):
        bitrate_I_(BitrateIFace::create()),
        bitMTU_(mtu*8),
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

SenderIFace* SenderIFace::create(const int mtu)
{
    return new Sender(mtu);
}
}
