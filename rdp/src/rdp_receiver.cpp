#include "rdp_config.hpp"
#include "rdp_receiver.hpp"
#include "rdp_packet.hpp"
#include "rdp_bitstream.hpp"
#include "rdp_rwqueue.hpp"
#include "rdp_time.hpp"
#include "rdp_sender.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_bwe.hpp"
#include "rdp_shared.hpp"
#include <map>
#include <deque>
#include <vector>
namespace rdp {
namespace {
class Receiver : public ReceiverIFace
{
    typedef std::vector<PacketPtr> PacketList;
    struct SplitPacketFrame
    {
        SplitPacketFrame():bitLength(0),splitPacketCount(0){}
        ~SplitPacketFrame()
        {//TODO
        }
        Time lastUpdateTime;
        int bitLength;
        int splitPacketCount;
        PacketList splitPackets;
    };
    typedef std::map<int, SplitPacketFrame> SplitPacketFrameMap;

    uint32_t receivedPacketsBaseIndex_;
    uint32_t waitingForSequencedPacketReadIndex_[0x100];
    uint32_t waitingForOrderedPacketReadIndex_[0x100];

    PacketList          orderingQueues_[0x100];
    SplitPacketFrameMap splitPacketFrames_;
    Statistics          statistics_;
    std::deque<Time>    hasReceivedPackets_;
    OutputerIFace&      outputer_;

    AckFeedbackIFace&   ackFeedback_;         

    RWQueue<uint32_t>   ackQueue_;//接收ACK,由线程R->W->ackRanges_
    AckRangeList        ackRanges_;//组装ACK
    RWQueue<AckRangeList> nackQueue_;//接收NACK,由线程R->W->nackRanges_
    AckRangeList        nackRanges_;//组装NACK
    Time                nackNextTime_;


    RWQueue<PacketPtr>  receivedQueue_;//接收包,由线程R->W
    BitrateIFace*       bitrate_R_;//接收码率
    BWEIFace&           bwe_;//下行码率估计,产生TMMBR

    uint8_t             rttSampleIndex_;
    Time                rttSamples_[0x100];//由ACK带回的RTT
    Time                rttSum_;
    Time                peerTime_;//收到对端时间
    Time                reportNextTime_;

    //写入所有Ack,并清除ackRanges
    bool writeRanges(BitStream& bs, AckRangeList& acks, int& bitSent)
    {
        const int bitWrite = bs.bitWrite;
        const uint16_t ackSize = (uint16_t)acks.size();
        JIF(bs.write(ackSize != 0));
        if (ackSize == 0) return true;

        JIF(bs.compressWrite(ackSize));
        for (AckRangeList::iterator iter = acks.begin();
            iter != acks.end(); ++iter) {
            const bool maxEqualToMin = (iter->first == iter->second);
            JIF(bs.write(maxEqualToMin));
            JIF(bs.compressWrite(iter->first));
            if (!maxEqualToMin)
                JIF(bs.compressWrite(iter->second));
        }
        acks.clear();
        bitSent += bs.bitWrite - bitWrite;
        return true;
clean:
        return false;
    }
    bool readRanges(BitStream& bs, AckRangeList& acks)
    {
        uint16_t ackSizes;
        bool hasAck;
        acks.clear();
        JIF(bs.read(hasAck));
        if (!hasAck) return true;

        JIF(bs.compressRead(ackSizes));
        for (uint16_t i=0; i< ackSizes; ++i) {
            bool maxEqualToMin;
            AckRangeList::value_type::first_type minMessageNumber, maxMessageNumber;
            JIF(bs.read(maxEqualToMin));
            JIF(bs.compressRead(minMessageNumber));
            if (!maxEqualToMin)
                JIF(bs.compressRead(maxMessageNumber));
            else
                maxMessageNumber = minMessageNumber;
            acks.push_back(AckRangeList::value_type(minMessageNumber, maxMessageNumber));
        }
        return true;
clean:
        return false;
    }
    bool readAks_R(BitStream& bs, const Time& nowNS)
    {
        /*
           +-+-+- ~ ~ +- ~ ~ +-+-+-+-+-+-+-+-+-+-+-+-+-+
           | V | ACKS | NACK |R|PeerMs |LocalMs  |Tmmbr|
           +-+- ~ ~ +-+-+- ~ ~ +-+-+-+-+-+-+-+-+-+-+-+-+
           V=kVersion, R=HasReport
           */
        AckRangeList acks, nacks;
        bool hasReport;


        uint8_t version = 0;
        JIF(bs.read(&version, 2));
        JIF(version == kVersion);
        JIF(readRanges(bs, acks));
        JIF(readRanges(bs, nacks));
        JIF(bs.read(hasReport));
        if (hasReport) {
            uint32_t hostTimeMS, peerTimeMS;
            uint32_t tmmrKbps;
            JIF(bs.read(hostTimeMS));//本地的回环时间
            JIF(bs.read(peerTimeMS));//对端时间,用于对端RTT计
            JIF(bs.compressRead(tmmrKbps));
            updateRttSample(Time::MS(hostTimeMS - nowNS.millisec()));
            peerTime_ = Time::MS(peerTimeMS);
            statistics_.bwKbps = tmmrKbps;
            bwe_.onReceived_R(bs.bitWrite, peerTime_, bitrate_R_->kbps(nowNS), nowNS);
        }
        ackFeedback_.onAcks_R(acks, nacks, rttSum_>>8);
        return true;
clean:
        return false;
    }
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) override
    {
        const int bitWrite = bs.bitWrite;
        const uint8_t version = kVersion;
        const bool needReport = nowNS > reportNextTime_;
        /*
           +-+-+- ~ ~ +- ~ ~ +-+-+-+-+-+-+-+-+-+-+-+-+-+
           | V | ACKS | NACK |R|PeerMs |LocalMs  |Tmmbr|
           +-+- ~ ~ +-+-+- ~ ~ +-+-+-+-+-+-+-+-+-+-+-+-+
           V=kVersion, R=HasReport
           */
        JIF(bs.write(&version, 2));
        JIF(writeRanges(bs, ackRanges_, statistics_.ackBitSent));
        JIF(writeRanges(bs, nackRanges_,statistics_.nackBitSent));
        JIF(bs.write(needReport));
        if (needReport) {
            reportNextTime_ = nowNS + Time::MS(200);
            JIF(bs.write((uint32_t)peerTime_.millisec()));
            JIF(bs.write((uint32_t)nowNS.millisec()));
            JIF(bs.compressWrite((uint32_t)bwe_.tmmbrKbps_W(rttSum_>>8, nowNS)));
        }

        statistics_.ackTotalBitSent += bs.bitWrite - bitWrite;
        return true;
clean:
        bs.bitWrite = bitWrite;
        return false;
    }
    //ReceiverIFace
    virtual void onReceived_R(BitStream& bs, const Time& nowNS) override
    {
        Packet *packet;
        bitrate_R_->update(bs.bitWrite, nowNS);
        JIF(readAks_R(bs, nowNS));
        while (Packet::maybePacket(bs) && (packet = Packet::readFromStream(bs))) {
            if (packet->reliability & Packet::kReliable)
                ackQueue_.push(packet->messageNumber);

            const size_t holeCount = packet->messageNumber - receivedPacketsBaseIndex_;
            if (holeCount == 0) {//恰好是要的包
                if (hasReceivedPackets_.size())
                    hasReceivedPackets_.pop_front();
                receivedPacketsBaseIndex_++;
            }
            else if (holeCount > 0x7fff) {//防止上缢出
                statistics_.duplicateReceivedPackets++;
                packet->release();
                continue;
            }
            else if (holeCount < (int)hasReceivedPackets_.size()) {//晚到的包
                if (hasReceivedPackets_[holeCount])
                    hasReceivedPackets_[holeCount] = Time::zero;
                else {
                    statistics_.duplicateReceivedPackets++;
                    packet->release();
                    continue;
                }
            }
            else {//提前到的包,设置中间未到包的等待过期时间
                while (holeCount > (int)hasReceivedPackets_.size())
                    hasReceivedPackets_.push_back(nowNS + Time::MS(600));
                hasReceivedPackets_.push_back(Time::zero);
            }
            //丢弃过期，还未到的包
            while (hasReceivedPackets_.size()
                && hasReceivedPackets_.front() < nowNS) {
                hasReceivedPackets_.pop_front();
                receivedPacketsBaseIndex_++;
            }
            receivedQueue_.push(packet);
            if (!hasReceivedPackets_.empty() && nowNS > nackNextTime_) {
                nackNextTime_ = hasReceivedPackets_.front();
                uint16_t seqno = receivedPacketsBaseIndex_;
                AckRangeList nacks;
                for (std::deque<Time>::iterator iter = hasReceivedPackets_.begin();
                    iter != hasReceivedPackets_.end(); ++iter, ++seqno) {
                    if (*iter != Time::zero) nacks.insert(seqno);
                }
                nackQueue_.writeLock()->swap(nacks);
                nackQueue_.writeUnlock();
            }
        }
clean:
        return;
    }

    virtual void update_W(const Time& nowNS) override
    {
        handleAcks(nowNS);
        handlePackets(nowNS);
        discardExpiredSplitPacket(nowNS);
    }
    virtual void stats(Statistics& stats) const override
    {
        stats = statistics_;
    }
private:
    void handleAcks(const Time& nowNS)
    {
        uint32_t messageNumber;
        while (ackQueue_.pop(messageNumber))
            ackRanges_.insert(messageNumber);
        nackQueue_.pop(nackRanges_);
    }
    //分类数据报
    void handlePackets(const Time& nowNS)
    {
        PacketPtr packet;
        while (receivedQueue_.pop(packet)) {
            statistics_.receivedPackets++;
            //TODO 调用插件处理
            if (packet->reliability & Packet::kSequence) {//有序包
                if (packet->reliability == Packet::kOrderable) {//预存提前包
                    if (packet->splitPacketCount > 0)//是个split包
                        packet = handleSplitPacket(packet, nowNS);
                    if (packet)
                        handleOrderPacket(packet);
                }
                else {
                    if (packet->orderingIndex >= waitingForSequencedPacketReadIndex_[packet->orderingChannel]) {
                        statistics_.sequencedInOrder++;
                        if (packet->splitPacketCount > 0)//是个split包
                            packet = handleSplitPacket(packet, nowNS);
                        if (packet) {
                            waitingForSequencedPacketReadIndex_[packet->orderingChannel] = packet->orderingIndex + 1;
                            outputer_.onPackets_W(&packet, 1);
                        }
                    }
                    else {//丢弃乱序包
                        statistics_.sequencedOutOfOrder++;
                        outputer_.onDisorderPacket_W(packet);
                    }
                }
            } 
            else {
                if (packet->splitPacketCount > 0)//是个split包
                    packet = handleSplitPacket(packet, nowNS);
                if (packet)
                    outputer_.onPackets_W(&packet, 1);
            }
        }
    }

    //丢弃过期的分割包
    void discardExpiredSplitPacket(const Time& nowNS)
    {
        SplitPacketFrameMap::iterator iter = splitPacketFrames_.begin();
        while (iter != splitPacketFrames_.end()) {
            SplitPacketFrameMap::iterator cur = iter++;
            struct SplitPacketFrame& frame = cur->second;
            if (nowNS > frame.lastUpdateTime) {
                PacketPtr packet = frame.splitPackets.front();
                if (packet->reliability & Packet::kReliable) {
                    if ((packet->reliability & Packet::kSequence) == 0)
                        continue;//可信的split包,不丢弃
                }
                outputer_.onExpiredPackets_W(&frame.splitPackets.front(), frame.splitPackets.size());
                splitPacketFrames_.erase(cur);
            }
        }
    }
    //处理拆分报
    PacketPtr handleSplitPacket(PacketPtr packet, const Time& nowNS)
    {
        const int splitPacketId = packet->splitPacketId;
        SplitPacketFrame& frame = splitPacketFrames_[splitPacketId];
        if (frame.splitPackets.size() < packet->splitPacketCount)
            frame.splitPackets.resize(packet->splitPacketCount);
        frame.splitPackets[packet->splitPacketIndex] = packet;
        frame.lastUpdateTime = nowNS;
        frame.bitLength += packet->bitLength;
        frame.splitPacketCount++;
        if(frame.splitPacketCount < packet->splitPacketCount)
            return nullptr;
        packet = Packet::merge(&frame.splitPackets.front(), frame.splitPackets.size());
        frame.splitPackets.clear();
        splitPacketFrames_.erase(splitPacketId);
        return packet;
    }

    //处理次序报
    void handleOrderPacket(PacketPtr packet)
    {
        const int orderingChannel = packet->orderingChannel;
        PacketList& orderingQueue = orderingQueues_[orderingChannel];
        const uint16_t readIndex = waitingForOrderedPacketReadIndex_[orderingChannel];
        if (packet->orderingIndex < readIndex) {//晚到包
            statistics_.orderedOutOfOrder++;
            outputer_.onDisorderPacket_W(packet);
            return;
        }
        size_t index = packet->orderingIndex - readIndex;
        if (index == 0 && orderingQueue.empty()) {//针对恰好包的小优化
            statistics_.orderedInOrder++;
            outputer_.onPackets_W(&packet, 1);
            waitingForOrderedPacketReadIndex_[orderingChannel] ++;
            return;
        }

        if (index >= orderingQueue.size()) orderingQueue.resize(index+1);
        ASSERT(!orderingQueue[index]);
        orderingQueue[index] = packet;//默认缓存提前包

        if (index == 0) {
            statistics_.orderedInOrder++;
            while (index < orderingQueue.size()
                && orderingQueue[index]->orderingIndex - readIndex == index){
                ++index;
            }
            PacketList::iterator iter = orderingQueue.begin();
            outputer_.onPackets_W(&(*iter), index);
            orderingQueue.erase(iter, iter+index);
            waitingForOrderedPacketReadIndex_[orderingChannel] += index;
        }
        else
            statistics_.orderedOutOfOrder++;
    }
    void updateRttSample(const Time& rttTime)
    {
        if (rttSum_ == 0) {
            for(int i=0; i<0x100; ++i) {
                rttSamples_[i] = rttTime;
                rttSum_ += rttTime;
            }
            rttSampleIndex_ = 0;
        }
        else {
            rttSum_ += rttTime;
            rttSum_ -= rttSamples_[rttSampleIndex_];
            rttSampleIndex_++;
        }
    }
public:
    Receiver(AckFeedbackIFace& ackFeedback,
             OutputerIFace& outputer,
             BWEIFace& bwe) :
        ackFeedback_(ackFeedback),
        outputer_(outputer),
        bitrate_R_(BitrateIFace::create()),
        bwe_(bwe),
        receivedPacketsBaseIndex_(0)
    {
        memset(&statistics_, 0, sizeof(statistics_));
        memset(waitingForSequencedPacketReadIndex_, 0, sizeof(waitingForSequencedPacketReadIndex_));
        memset(waitingForOrderedPacketReadIndex_, 0, sizeof(waitingForOrderedPacketReadIndex_));
        statistics_.bwKbps= kStartBwKbps;
    }
    ~Receiver()
    {
        delete bitrate_R_;
    }
};
}//namespace

ReceiverIFace* ReceiverIFace::create(AckFeedbackIFace& ackFeedback,
                                     OutputerIFace& outputer,
                                     BWEIFace& bwe)
{
    return new Receiver(ackFeedback, outputer, bwe);
}

}
