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
    std::vector<Time>   expiredPacketTimes_;//[序号- receivedPacketsBaseIndex_] = 过期时刻
    std::vector<Time>   nackPacketTimes_;//[序号- receivedPacketsBaseIndex_] = Nack 时刻

    OutputerIFace&      outputer_;

    AckFeedbackIFace&   ackFeedback_;         

    RWQueue<uint32_t>   ackQueue_;//接收ACK,由线程R->W->ackRanges_
    AckRangeList        ackRanges_;//组装ACK
    RWQueue<uint32_t>   nackQueue_;//接收NACK,由线程R->W->nackRanges_
    AckRangeList        nackRanges_;//组装NACK

    RWQueue<PacketPtr>  receivedQueue_;//接收包,由线程R->W
    BitrateIFace*       bitrate_R_;//接收码率
    BWEIFace&           bwe_;//下行码率估计,产生TMMBR

    uint8_t             rttMsSampleIndex_;
    uint32_t            rttMsSamples_[0x100];//由ACK带回的RTT
    uint32_t            rttMsQ8_;

    uint8_t             jitterMsSampleIndex_;
    uint32_t            jitterMsSamples_[0x100];
    uint32_t            jitterMsQ8_;

    Time                lastSampleTime_;
    Time                lastPeerTime_;//收到对端时间

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
           +- ~ ~ +- ~ ~ +-+-+-+-+-+-+-+-+-+-+-+-+
           | ACKS | NACK |R|PeerMs |LocalMs|Tmmbr|
           +- ~ ~ +-+-+- ~ ~ +-+-+-+-+-+-+-+-+-+-+
           R=HasReport
           */
        AckRangeList acks, nacks;
        bool hasReport;

        JIF(readRanges(bs, acks));
        JIF(readRanges(bs, nacks));
        JIF(bs.read(hasReport));
        if (hasReport) {
            uint32_t hostTimeMs, peerTimeMs;
            uint32_t tmmbrKbps;
            JIF(bs.read(hostTimeMs));//本地的回环时间
            JIF(bs.read(peerTimeMs));//对端时间,用于对端RTT计
            JIF(bs.compressRead(tmmbrKbps));
            updateStatistics(Time::MS(hostTimeMs), Time::MS(peerTimeMs), tmmbrKbps, nowNS);
            bwe_.onReceived_R(bs.bitWrite, lastPeerTime_, bitrate_R_->kbps(nowNS), nowNS);
        }
        ackFeedback_.onAcks_R(acks, nacks, Time::MS(rttMsQ8_>>8));
        return true;
clean:
        return false;
    }
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) override
    {
        const int bitWrite = bs.bitWrite;
        const bool needReport = nowNS > reportNextTime_;
        /*
           +- ~ ~ +- ~ ~ +-+-+-+-+-+-+-+-+-+-+-+-+
           | ACKS | NACK |R|PeerMs |LocalMs|Tmmbr|
           +- ~ ~ +-+-+- ~ ~ +-+-+-+-+-+-+-+-+-+-+
           R=HasReport
           */
        JIF(writeRanges(bs, ackRanges_, statistics_.ackBitSent));
        JIF(writeRanges(bs, nackRanges_,statistics_.nackBitSent));
        JIF(bs.write(needReport));
        if (needReport) {
            reportNextTime_ = nowNS + Time::MS(200);
            JIF(bs.write((uint32_t)lastPeerTime_.millisec()));
            JIF(bs.write((uint32_t)nowNS.millisec()));
            JIF(bs.compressWrite((uint32_t)bwe_.tmmbrKbps_W(Time::MS(rttMsQ8_>>8), nowNS)));
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
                if (expiredPacketTimes_.empty())
                    receivedPacketsBaseIndex_++;
                else
                    expiredPacketTimes_[0] = Time::zero;
            }
            else if (holeCount > 0x7fff) {//防止上缢出
                statistics_.duplicateReceivedPackets++;
                packet->release();
                continue;
            }
            else if (holeCount < (int)expiredPacketTimes_.size()) {//晚到的包
                if (expiredPacketTimes_[holeCount] != Time::zero)
                    expiredPacketTimes_[holeCount] = Time::zero;
                else {
                    statistics_.duplicateReceivedPackets++;
                    packet->release();
                    continue;
                }
            }
            else {//提前到的包,设置中间未到包的过期时刻
                const Time expired(nowNS + Time::MS(kExpiredMs));
                while (holeCount > (int)expiredPacketTimes_.size())
                    expiredPacketTimes_.push_back(expired);
                expiredPacketTimes_.push_back(Time::zero);
                nackPacketTimes_.resize(expiredPacketTimes_.size(), Time::zero);
            }
            receivedQueue_.push(packet);
            //丢弃过期包, 每个 RTT 周期发NACK
            size_t expiredNum = 0;
            const Time nackMs(nowNS + Time::MS(rttMsQ8_>>8));
            const size_t size = expiredPacketTimes_.size();
            for (size_t i=0; i< size;++i) {
                if (expiredPacketTimes_[i] < nowNS){
                    if (expiredNum == i) expiredNum = i + 1;
                } else if (expiredPacketTimes_[i] > nackMs //避免即将过期包
                    && nackPacketTimes_[i] < nowNS) {
                    nackQueue_.push(receivedPacketsBaseIndex_ + i);
                    nackPacketTimes_[i] = nackMs;
                }
            }
            if (expiredNum > 0) {
                receivedPacketsBaseIndex_ += expiredNum;
                expiredPacketTimes_.erase(expiredPacketTimes_.begin(), 
                    expiredPacketTimes_.begin() + expiredNum);
                nackPacketTimes_.erase(nackPacketTimes_.begin(),
                    nackPacketTimes_.begin() + expiredNum);
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
        while (nackQueue_.pop(messageNumber))
            nackRanges_.insert(messageNumber);
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
    void updateStatistics(const Time& localTimeMs, const Time& peerTimeMs, uint32_t tmmrKbps, const Time& nowNs)
    {
        uint32_t rttMs = uint32_t((nowNs - localTimeMs).millisec());
        if (rttMsQ8_ == 0 && rttMsSampleIndex_ == 0) {
            for(int i=0; i<0x100; ++i) {
                rttMsSamples_[i] = rttMs;
                rttMsQ8_ += rttMs;
            }
        }
        else {
            rttMsQ8_ += rttMs - rttMsSamples_[rttMsSampleIndex_++];
        }

        if (lastSampleTime_ != 0) {
            uint32_t jitterDiff = abs(long(
                (nowNs - lastSampleTime_ - (peerTimeMs - lastPeerTime_)).millisec()));
            if (jitterMsQ8_ == 0 && jitterMsSampleIndex_ == 0) {
                for(int i=0; i<0x100; ++i) {
                    jitterMsSamples_[i] = jitterDiff;
                    jitterMsQ8_ += jitterDiff;
                }
            }
            else {
                jitterMsQ8_ += jitterDiff - jitterMsSamples_[jitterMsSampleIndex_++];
            }
        }
        lastPeerTime_ = peerTimeMs;
        lastSampleTime_ = nowNs;
        statistics_.bwKbps = tmmrKbps;
        statistics_.jitterMs = (jitterMsQ8_>>8);
        statistics_.rttMs = rttMsQ8_ >> 8;
    }
public:
    Receiver(AckFeedbackIFace& ackFeedback,
             OutputerIFace& outputer,
             BWEIFace& bwe) :
        ackFeedback_(ackFeedback),
        outputer_(outputer),
        bitrate_R_(BitrateIFace::create()),
        bwe_(bwe),
        receivedPacketsBaseIndex_(0),
        rttMsSampleIndex_(0), rttMsQ8_(0), 
        jitterMsSampleIndex_(0), jitterMsQ8_(0)
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
