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
    uint16_t waitingForSequencedPacketReadIndex_[0x100];
    uint16_t waitingForOrderedPacketReadIndex_[0x100];

    PacketList          orderingQueues_[0x100];
    SplitPacketFrameMap splitPacketFrames_;
    Statistics          statistics_;
    std::deque<Time>    hasReceivedPackets_;
    OutputerIFace&      outputer_;

    AckFeedbackIFace&   ackFeedback_;         
    RWQueue<uint32_t>   ackQueue_;//����ACK,���߳�R->W->ackRanges
    AckRangeList        ackRanges_;//��װACK
    RWQueue<PacketPtr>  receivedQueue_;//���հ�,���߳�R->W
    SharedPtr<BitrateIFace> bitrate_R_;//��������
    BWEIFace&           bwe_;//�������ʹ���,����TMMBR

    uint8_t             rttSampleIndex_;
    Time                rttSamples_[0x100];//��ACK���ص�RTT
    Time                rttSum_;
    Time                peerTime_;//�յ��Զ�ʱ��


    //ReceiverIFace
    // ��������ʽ��: ACK + LocalTime + PeerTime + TMMBR + [PACKET]*
    // ACK -> [uint8_t]size + [uint16_t, uint16_t]range*
    virtual void onReceived_R(BitStream& bs, const Time& nowNS)
    {
        bitrate_R_->update(bs.bitWrite,nowNS);
        {//��ȡACK, RTT, TMMR
            uint32_t hostTimeMS, peerTimeMS;
            uint32_t tmmrKbps;
            uint8_t ackSizes;
            JIF(bs.read(ackSizes));
            AckRangeList acks;
            for (uint8_t i=0; i< ackSizes; ++i) {
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
            JIF(bs.read(hostTimeMS));//���صĻػ�ʱ��
            JIF(bs.read(peerTimeMS));//�Զ�ʱ��,���ڶԶ�RTT��
            JIF(bs.compressRead(tmmrKbps,true));
            updateRttSample(Time::MS(hostTimeMS - nowNS.millisec()));
            peerTime_ = Time::MS(peerTimeMS);
            statistics_.bwKbps = tmmrKbps;
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

            if (holeCount == 0){//ǡ����Ҫ�İ�
                if (hasReceivedPackets_.size())
                    hasReceivedPackets_.pop_front();
                receivedPacketsBaseIndex_++;
            }
            else if (holeCount > 0x7fff){//��ֹ���˳�
                statistics_.duplicateReceivedPackets++;
                delete packet;
                continue;
            }
            else if (holeCount < (int)hasReceivedPackets_.size()) {//���İ�
                if (hasReceivedPackets_[holeCount])
                    hasReceivedPackets_[holeCount] = Time::zero;
                else {
                    statistics_.duplicateReceivedPackets++;
                    delete packet;
                    continue;
                }
            }
            else {//��ǰ���İ�,�����м�δ�����ĵȴ�����ʱ��
                while (holeCount > (int)hasReceivedPackets_.size())
                    hasReceivedPackets_.push_back(nowNS + Time::MS(60));
                hasReceivedPackets_.push_back(Time::MS(0));
            }
            //�������ڣ���δ���İ�
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
    //д������Ack,�����ackRanges
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS) override
    {
        const int bitsWrite = bs.bitWrite;
        int ackSize = (int)ackRanges_.size();
        if (ackSize > 0xFF) ackSize = 0xFF;
        AckRangeList::iterator iter = ackRanges_.begin();
        JIF(bs.write((uint8_t)ackSize));
        while (iter != ackRanges_.end() && ackSize-- > 0) {
            const bool maxEqualToMin = (iter->first == iter->second);
            JIF(bs.write(maxEqualToMin));
            JIF(bs.write(iter->first));
            if (!maxEqualToMin)
                JIF(bs.write(iter->second));
            ++iter;
        }
        JIF(bs.write((uint32_t)peerTime_.millisec()));
        JIF(bs.write((uint32_t)nowNS.millisec()));
        {
            const uint32_t tmmbr = (uint32_t)bwe_.tmmbrKbps_W(rttSum_>>8, nowNS);
            JIF(bs.compressWrite(tmmbr, true));
        }
        ackRanges_.erase(ackRanges_.begin(), iter);
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
    virtual void stats(Statistics& stats) const override
    {
        stats = statistics_;
    }
private:
    void handleAcks(const Time& nowNS)
    {
        uint32_t messageNumber;
        while (ackQueue_.pop(messageNumber)) {
            ackRanges_.insert(messageNumber);
        }
    }
    //�������ݱ�
    void handlePackets(const Time& nowNS)
    {
        PacketPtr packet;
        while (receivedQueue_.pop(packet)) {
            statistics_.receivedPackets++;
            //TODO ���ò������
            if (packet->reliability & Packet::kSequence) {//�����
                if (packet->reliability & Packet::kReliable) {//Ԥ����ǰ��
                    if (packet->splitPacketCount > 0)//�Ǹ�split��
                        packet = handleSplitPacket(packet, nowNS);
                    if (packet)
                        handleOrderPacket(packet);
                }
                else {
                    if (packet->orderingIndex >= waitingForSequencedPacketReadIndex_[packet->orderingChannel]) {
                        statistics_.sequencedInOrder++;
                        if (packet->splitPacketCount > 0)//�Ǹ�split��
                            packet = handleSplitPacket(packet, nowNS);
                        if (packet) {
                            waitingForSequencedPacketReadIndex_[packet->orderingChannel] = packet->orderingIndex + 1;
                            outputer_.onPacket_W(packet, true);
                        }
                    }
                    else {//�������ڰ�
                        packet->release();
                        statistics_.sequencedOutOfOrder++;
                    }
                }
            } 
            else {
                if (packet->splitPacketCount > 0)//�Ǹ�split��
                    packet = handleSplitPacket(packet, nowNS);
                if (packet)
                    outputer_.onPacket_W(packet, false);
            }
        }
    }
    //�������ڵķָ��
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
                        continue;//���ŵ�split��,������
                }
                outputer_.onSplitPackets_W(&frame.splitPackets.front(), frame.splitPackets.size());
                splitPacketFrames_.erase(cur);
            }
        }
    }
    //�����ֱ�
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

    //�������
    void handleOrderPacket(PacketPtr packet)
    {
        const int orderingChannel = packet->orderingChannel;
        uint16_t& readIndex = waitingForOrderedPacketReadIndex_[orderingChannel];
        if (packet->orderingIndex < readIndex) {//����
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
            if (packet->controlFlags & Packet::kResetable) { //ǿ�Ƶ���packet->orderingIndex֮ǰ
                outputer_.onOrderPackets_W(&(*iter), orderingQueue.end() - iter, false);
                orderingQueue.erase(iter,orderingQueue.end());
                goto pop;
            }
            else {//Ĭ�ϻ�����ǰ��
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
        receivedPacketsBaseIndex_(0)
    {
        memset(&statistics_, 0, sizeof(statistics_));
        memset(waitingForSequencedPacketReadIndex_, 0, sizeof(waitingForSequencedPacketReadIndex_));
        memset(waitingForOrderedPacketReadIndex_, 0, sizeof(waitingForOrderedPacketReadIndex_));
        statistics_.bwKbps= kStartBwKbps;
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
