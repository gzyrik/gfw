#include "rdp_config.hpp"
#include "rdp_fec.hpp"
#include "rdp_bitstream.hpp"
#include "fec/forward_error_correction.h"
namespace rdp{
namespace {
using namespace fec;
class Fec : public FECIFace
{
    transport::SenderIFace& sender_;
    transport::ReceiverIFace& receiver_;
    uint16_t  datSeqno_,fecSeqno_;
    ForwardErrorCorrection fec_;
    uint8_t minRate_, maxRate_;
    bool enableDecode_;

    virtual void fixRate(const uint8_t minRate, const uint8_t maxRate, const bool enableDecode) override
    {
        minRate_ = minRate;
        maxRate_ = maxRate;
        enableDecode_ = enableDecode;
    }
    virtual void markFec_W(BitStream& bs, const Time& nowNS) override
    {
        const int bitWrite = (bs.bitWrite + 7 ) & ~7;//对齐
        wordToBuffer(bs.bitBuffer+bitWrite, datSeqno_);
        bs.bitWrite = bitWrite + 16;
        ASSERT(bs.bitWrite <  bs.bitSize);
    }
    virtual void onReceived_R(BitStream& bs, const Time& nowNS) override
    {
        int bitRead = (bs.bitRead+ 7 ) & ~7;//对齐
        const uint16_t seqno = bufferToWord(bs.bitBuffer+bitRead);
        const bool isFec = (seqno & 0x8000) != 0;
        if (enableDecode_){
            if (isFec) bitRead += 16;//跳过FEC 包中多余的头

            ForwardErrorCorrection::ReceivedPacket* receivedPacket =
                new ForwardErrorCorrection::ReceivedPacket;
            receivedPacket->isFec = isFec;
            receivedPacket->seqNum = seqno;

            ForwardErrorCorrection::Packet* pkt = new ForwardErrorCorrection::Packet;
            pkt->length = bits2bytes(bs.bitWrite -bitRead);
            memcpy(pkt->data, bs.bitBuffer+bitRead/8, pkt->length);
            receivedPacket->pkt = pkt;

            ProcessFEC(receivedPacket, nowNS);
        }
        else if (!isFec){
            bs.bitRead = bitRead + 16;
            receiver_.onReceived_R(bs, nowNS);  
        }
    }
    uint8_t getFecRate() {//根据丢包率, 动态计算 FEC 占比
        uint8_t fecRate = 128;
        if (fecRate < minRate_) fecRate = minRate_;
        else if (fecRate > maxRate_) fecRate = maxRate_;
        return fecRate;
    }
    ForwardErrorCorrection::PacketList datList_;
    ForwardErrorCorrection::PacketList fecList_;
    ForwardErrorCorrection::Packet fecPackets_[ForwardErrorCorrection::kMaxMediaPackets];
    void sendFecs(const Time& now, bool doClear) {
        ForwardErrorCorrection::PacketList::iterator iter = fecList_.begin();
        while (iter != fecList_.end()) {
            ForwardErrorCorrection::PacketList::iterator cur = iter++;
            ForwardErrorCorrection::Packet* pkt = *cur;
            if (doClear || rand() % 100 < 50) {
                fecList_.erase(cur);
                uint8_t buf[kMaxMtuBytes];
                wordToBuffer(buf, fecSeqno_);
                memcpy(buf + 2,pkt->data,pkt->length);
                BitStream bs2(buf, (pkt->length + 2)*8, -1);
                sender_.send_W(bs2, now);
                if (++fecSeqno_ == 0xffff) fecSeqno_ = 0x8000;
            }
        }
    }
    virtual void send_W(BitStream& bs, const Time& nowNS) 
    {
        if (!fecList_.empty()) sendFecs(nowNS, true);

        ForwardErrorCorrection::Packet* pkt = 
            &fecPackets_[datSeqno_ % ForwardErrorCorrection::kMaxMediaPackets];
        if (++datSeqno_ == 0x7FFF) datSeqno_ = 0;

        pkt->length = bits2bytes(bs.bitWrite);
        memcpy(pkt->data, bs.bitBuffer, pkt->length);
        sender_.send_W(bs, nowNS);

        datList_.push_back(pkt);
        if (datList_.size() > 4)
        {
            uint8_t fecRate = getFecRate();
            if (fecRate != 0) {
                const uint16_t sequenceNumberBase = bufferToWord(fecPackets_[0].data);
                int ret = fec_.GenerateFEC(datList_, fecRate, 0, false, &fecList_, sequenceNumberBase);
                if (ret == 0) sendFecs(nowNS, false);
            }
            while (datList_.size() >= fec_.kMaxMediaPackets)
                datList_.pop_front();
        }
    }

    ForwardErrorCorrection::ReceivedPacketList receivedPackets_;
    ForwardErrorCorrection::RecoveredPacketList recoveredPackets_;
    void ProcessFEC(ForwardErrorCorrection::ReceivedPacket* receivedPacket, const Time& nowNS)
    {
        const uint16_t seqno = receivedPacket->seqNum;
        receivedPackets_.push_back(receivedPacket);
        int ret = fec_.DecodeFEC(&receivedPackets_, &recoveredPackets_);
        char logs[1500];
        int slen = _snprintf(logs, sizeof(logs), "%X->", seqno);
        int wasRecovered = 0;

        ForwardErrorCorrection::RecoveredPacketList::iterator iter = recoveredPackets_.begin();
        while (iter != recoveredPackets_.end()) {
            ForwardErrorCorrection::RecoveredPacket* recoveredPacket = *iter++;
            if (recoveredPacket->returned)
                continue;
            recoveredPacket->returned = true;
            ForwardErrorCorrection::Packet * pkt = recoveredPacket->pkt;

            ASSERT(bufferToWord(pkt->data) == recoveredPacket->seqNum);
            slen += _snprintf(logs+slen, sizeof(logs)-slen, "%c%X",
                recoveredPacket->wasRecovered ? '*':' ', recoveredPacket->seqNum);

            BitStream bs(pkt->data, pkt->length*8, -1, 16);
            receiver_.onReceived_R(bs, nowNS);
            if (recoveredPacket->wasRecovered) ++wasRecovered;
        }
        if (wasRecovered > 0) printf("FEC: %s\n", logs);
    }
public:
    Fec(transport::SenderIFace& sender, transport::ReceiverIFace& receiver) :
        sender_(sender), receiver_(receiver),
        fec_(0), datSeqno_(0), fecSeqno_(0x8000),
        minRate_(0), maxRate_(0xFF), enableDecode_(true)
    {}
    ~Fec()
    {
        fec_.ResetState(&recoveredPackets_);
    }
};
}

FECIFace* FECIFace::create(transport::SenderIFace& sender, transport::ReceiverIFace& receiver)
{
    return new Fec(sender, receiver);
}
}
