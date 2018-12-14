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
    RWQueue<uint32_t>   ackQueue_;//����ACK,���߳�R->W->ackRanges
    AckRangeList        ackRanges_;//��װACK
    RWQueue<PacketPtr>  receivedQueue_;//���հ�,���߳�R->W
    SharedPtr<BitrateIFace> bitrate_;//��������
    BWEIFace&           bwe_;//�������ʹ���,����TMMBR

    uint8_t             rttSampleIndex_;
    Time                rttSamples_[0x100];//��ACK���ص�RTT
    Time                rttSum_;
    Time                peerTime_;//�յ��Զ�ʱ��
    int                 tmmrKbps_;

    //ReceiverIFace
    // ��������ʽ��:[ACK + RTT_LocalTime + TMMBR] + [RTT peerTime] + [PACKET]*
    // ACK -> [uint16_t]size + [uint32_t, uint32_t]range+[Time]RTT_LocalTime
    virtual void onReceived_R(BitStream& bs, const Time& nowNS)
    {
        {//��ȡACK+RTT
            uint32_t hostTimeMS, peerTimeMS;
            uint32_t tmmrKbps;
            uint16_t ackSizes=0;
            JIF(bs.read(ackSizes));
            AckRangeList acks;
            for (int i=0; i< ackSizes; ++i) {
                bool maxEqualToMin;
                uint32_t minMessageNumber, maxMessageNumber;
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
            tmmrKbps_ = tmmrKbps;
            bitrate_->update(bs.bitWrite,nowNS);
            bwe_.onReceived_R(bs.bitWrite, peerTime_, bitrate_->kbps(nowNS), nowNS);
            ackFeedback_.onAcks_R(acks, rttSum_>>8);
        }
        Packet *packet;
        while (Packet::maybePacket(bs) && (packet = Packet::readFromStream(bs)))
        {
            const size_t holeCount = packet->messageNumber - receivedPacketsBaseIndex_;
            if (packet->reliability & Packet::kReliable)
                ackQueue_.push(packet->messageNumber);

            if (holeCount == 0){//ǡ����Ҫ�İ�
                if (hasReceivedPackets_.size())
                    hasReceivedPackets_.pop_front();
                receivedPacketsBaseIndex_++;
            }
            else if (holeCount > (size_t)-1/2){//��ֹ���˳�
                statistics_.duplicateReceivedPackets++;
                delete packet;
                continue;
            }
            else if (holeCount < hasReceivedPackets_.size()) {//�����İ�
                if (hasReceivedPackets_[holeCount])
					hasReceivedPackets_[holeCount] = Time::MS(0);
                else {
                    statistics_.duplicateReceivedPackets++;
                    delete packet;
                    continue;
                }
            }
            else {//��ǰ���İ�,�����м�δ�����ĵȴ�����ʱ��
                while (holeCount > hasReceivedPackets_.size())
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
    virtual bool writeAcks_W(BitStream& bs, const Time& nowNS)
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

    virtual void update_W(const Time& nowNS)
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
    //�������ݱ�
    void handlePackets(const Time& nowNS)
    {
        PacketPtr *ppacket;
        while ((ppacket = receivedQueue_.readLock())) {
            PacketPtr packet = *ppacket;
            receivedQueue_.readUnlock();
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
                if (packet->splitPacketCount > 0)//�Ǹ�splik��
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
    //������ֱ�
    PacketPtr handleSplitPacket(PacketPtr packet, const Time& nowNS)
    {
        const int splitPacketId = packet->splitPacketId;
        SplitPacketFrame& frame = splitPacketFrames_[splitPacketId];
        if (!frame.splitPacketCount)
            frame.splitPackets.resize(packet->splitPacketCount);
        frame.splitPackets[packet->splitPacketIndex] = packet;
        frame.lastUpdateTime = nowNS;
        frame.bitLength    += packet->bitLength;
        frame.splitPacketCount++;
        if(frame.splitPacketCount < packet->splitPacketCount)
            return 0;
        packet = Packet::merge(&frame.splitPackets.front(), frame.splitPackets.size());
        frame.splitPackets.clear();
        splitPacketFrames_.erase(splitPacketId);
        return packet;
    }

    //��������
    void handleOrderPacket(PacketPtr packet)
    {
        const int orderingChannel = packet->orderingChannel;
        uint32_t& readIndex = waitingForOrderedPacketReadIndex_[orderingChannel];
        if (packet->orderingIndex < readIndex) {//������
            statistics_.orderedOutOfOrder++;
            if (packet->orderingFlags & Packet::kDelayable)
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
            if (packet->orderingFlags & Packet::kResetable) { //ǿ�Ƶ���packet->orderingIndex֮ǰ
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
        bitrate_(BitrateIFace::create()),
        bwe_(bwe),
        tmmrKbps_(kMaxBandwidthKbps),
        receivedPacketsBaseIndex_(0)
    {
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