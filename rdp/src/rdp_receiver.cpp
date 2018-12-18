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

    uint16_t receivedPacketsBaseIndex_;
    uint32_t waitingForSequencedPacketReadIndex_[0x100];
    uint32_t waitingForOrderedPacketReadIndex_[0x100];

    PacketList          orderingQueues_[0x100];
    SplitPacketFrameMap splitPacketFrames_;
    Statistics          statistics_;
    std::deque<Time>    hasReceivedPackets_;
    OutputerIFace&      outputer_;

    AckFeedbackIFace&   ackFeedback_;         
    RWQueue<uint32_t>   ackQueue_;//接收ACK,由线程R->W->ackRanges
    AckRangeList        ackRanges_;//组装ACK
    RWQueue<PacketPtr>  receivedQueue_;//接收包,由线程R->W
    SharedPtr<BitrateIFace> bitrate_R_;//接收码率
    BWEIFace&           bwe_;//下行码率估计,产生TMMBR

    uint8_t             rttSampleIndex_;
    Time                rttSamples_[0x100];//由ACK带回的RTT
    Time                rttSum_;
    Time                peerTime_;//收到对端时间
    int                 tmmrKbps_;

    //ReceiverIFace
    // 数据流格式是:[ACK + RTT_LocalTime + TMMBR] + [RTT peerTime] + [PACKET]*
    // ACK -> [uint16_t]size + [uint32_t, uint32_t]range+[Time]RTT_LocalTime
    virtual void onReceived_R(BitStream& bs, const Time& nowNS)
    {
        bitrate_R_->update(bs.bitWrite,nowNS);
        {//读取ACK+RTT
            uint32_t hostTimeMS, peerTimeMS;
            uint32_t tmmrKbps;
            uint16_t ackSizes=0;
            JIF(bs.read(ackSizes));
            AckRangeList acks;
            for (int i=0; i< ackSizes; ++i) {
                bool maxEqualToMin;
                uint16_t minMessageNumber, maxMessageNumber;
                JIF(bs.read(maxEqualToMin));
                JIF(bs.read(minMessageNumber));
                if (!maxEqualToMin)
                    JIF(bs.read(maxMessageNumber));
                else
                    maxMessageNumber = minMessageNumber;
                acks.push_back(AckRangeList::value_type(minMessageNumber, maxMessageNumber));
            }
            JIF(bs.read(hostTimeMS));//本地的回环时间
            JIF(bs.read(peerTimeMS));//对端时间,用于对端RTT计
            JIF(bs.compressRead(tmmrKbps,true));
            updateRttSample(Time::MS(hostTimeMS - nowNS.millisec()));
            peerTime_ = Time::MS(peerTimeMS);
            tmmrKbps_ = tmmrKbps;
            bwe_.onReceived_R(bs.bitWrite, peerTime_, bitrate_R_->kbps(nowNS), nowNS);
            ackFeedback_.onAcks_R(acks, rttSum_>>8);
        }
        Packet *packet;
        while (Packet::maybePacket(bs) && (packet = Packet::readFromStream(bs)))
        {
            int holeCount = 0;
            if (packet->messageNumber >= 0x7fff && !receivedPacketsBaseIndex_
                && hasReceivedPackets_.empty()) {
                receivedPacketsBaseIndex_ = packet->messageNumber;
            }
            else {
                holeCount = packet->messageNumber - receivedPacketsBaseIndex_;
                if (holeCount < 0) holeCount += 0xffff;
            }

            if (packet->reliability & Packet::kReliable)
                ackQueue_.push(packet->messageNumber);

            if (holeCount == 0){//恰好是要的包
                if (hasReceivedPackets_.size())
                    hasReceivedPackets_.pop_front();
                receivedPacketsBaseIndex_++;
            }
            else if (holeCount > 0x7fff){//防止上缢出
                statistics_.duplicateReceivedPackets++;
                delete packet;
                continue;
            }
            else if (holeCount < (int)hasReceivedPackets_.size()) {//晚到的包
                if (hasReceivedPackets_[holeCount])
					hasReceivedPackets_[holeCount] = Time::MS(0);
                else {
                    statistics_.duplicateReceivedPackets++;
                    delete packet;
                    continue;
                }
            }
            else {//提前到的包,设置中间未到包的等待过期时间
                while (holeCount > (int)hasReceivedPackets_.size())
					hasReceivedPackets_.push_back(nowNS + Time::MS(60));
                hasReceivedPackets_.push_back(Time::MS(0));
            }
            //丢弃过期，还未到的包
            while (hasReceivedPackets_.size()
                   && hasReceivedPackets_.front() < nowNS) {
                hasReceivedPackets_.pop_front();
                receivedPacketsBaseIndex_++;
            }
            receivedQueue_.push(packet);
        }
clean:
        return;
    }
    //写入所有Ack,并清除ackRanges
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) override
    {
        const int bitsWrite = bs.bitWrite;
        JIF(bs.write((uint16_t)ackRanges_.size()));
        for(AckRangeList::const_iterator iter = ackRanges_.begin();
            iter != ackRanges_.end(); ++iter) {
            const bool maxEqualToMin = (iter->first == iter->second);
            JIF(bs.write(maxEqualToMin));
            JIF(bs.write(iter->first));
            if (!maxEqualToMin)
                JIF(bs.write(iter->second));
        }
        JIF(bs.write((uint32_t)peerTime_.millisec()));
        JIF(bs.write((uint32_t)nowNS.millisec()));
        {
            const uint32_t tmmbr = (uint32_t)bwe_.tmmbrKbps_W(rttSum_>>8, nowNS);
            JIF(bs.compressWrite(tmmbr, true));
        }
        ackRanges_.clear();
        statistics_.ackTotalBitsSent += bs.bitWrite - bitsWrite;
        return true;
clean:
        bs.bitWrite = bitsWrite;
        return false;
    }

    virtual void update_W(const Time& nowNS) override
    {
        handleAcks(nowNS);
        handlePackets(nowNS);
        discardExpiredSplitPacket(nowNS);
    }
    virtual int tmmbrKbps_W(void) const
    {
        return tmmrKbps_;
    }
    virtual void stats(Statistics& stats) const
    {
        stats = statistics_;
    }
private:
    void handleAcks(const Time& nowNS)
    {
        uint32_t *pMessageNumber;
        while ((pMessageNumber = ackQueue_.readLock())) {
            uint32_t messageNumber = *pMessageNumber;
            ackQueue_.readUnlock();
            ackRanges_.insert(messageNumber);
        }
    }
    //分类数据报
    void handlePackets(const Time& nowNS)
    {
        PacketPtr *ppacket;
        while ((ppacket = receivedQueue_.readLock())) {
            PacketPtr packet = *ppacket;
            receivedQueue_.readUnlock();
            statistics_.receivedPackets++;
            //TODO 调用插件处理
            if (packet->reliability & Packet::kSequence) {//有序包
                if (packet->reliability & Packet::kReliable) {//预存提前包
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
                            outputer_.onPacket_W(packet, true);
                        }
                    }
                    else {//丢弃过期包
                        packet->release();
                        statistics_.sequencedOutOfOrder++;
                    }
                }
            } 
            else {
                if (packet->splitPacketCount > 0)//是个split包
                    packet = handleSplitPacket(packet, nowNS);
                if (packet)
                    outputer_.onPacket_W(packet, false);
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
                outputer_.onSplitPackets_W(&frame.splitPackets.front(), frame.splitPackets.size());
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
        uint32_t& readIndex = waitingForOrderedPacketReadIndex_[orderingChannel];
        if (packet->orderingIndex < readIndex) {//晚到包
            statistics_.orderedOutOfOrder++;
            if (packet->controlFlags & Packet::kDelayable)
                outputer_.onOrderPackets_W(&packet, 1, false);
            return;
        }
        PacketList& orderingQueue = orderingQueues_[orderingChannel];
        if (readIndex == packet->orderingIndex) {
            statistics_.orderedInOrder++;
pop:
            outputer_.onOrderPackets_W(&packet, 1, true);
            readIndex = packet->orderingIndex + 1;
            while (orderingQueue.size() && orderingQueue.back()->orderingIndex == readIndex) {
                outputer_.onOrderPackets_W(&packet, 1, true);
                ++readIndex;
                orderingQueue.pop_back();
            }
            return;
        }
        else {
            statistics_.orderedOutOfOrder++;
            PacketList::iterator iter = std::lower_bound(orderingQueue.begin(), orderingQueue.end(), packet);
            if (packet->controlFlags & Packet::kResetable) { //强制弹出packet->orderingIndex之前
                outputer_.onOrderPackets_W(&(*iter), orderingQueue.end() - iter, false);
                orderingQueue.erase(iter,orderingQueue.end());
                goto pop;
            }
            else {//默认缓存提前包
                orderingQueue.insert(iter, packet);
            }
        }
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
            rttSum_ -= rttSamples_[rttSampleIndex_];
            rttSum_ += rttTime;
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
        tmmrKbps_(kMaxBandwidthKbps),
        receivedPacketsBaseIndex_(0)
    {
        memset(&statistics_, 0, sizeof(statistics_));
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
