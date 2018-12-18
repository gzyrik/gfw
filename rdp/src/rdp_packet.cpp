#include "rdp_config.hpp"
#include "rdp_packet.hpp"
#include "rdp_bitstream.hpp"
#include <sstream>
#include <algorithm>
namespace rdp {
std::string Packet::toString() const
{
    static const char* reliable="USRO";
    static const char* orderable = "?FD";
    char buf[1500];

    int len = sprintf(buf, "%X%c%x", this->messageNumber, reliable[this->reliability&0x3], this->bitLength);
    if (this->reliability & kSequence) {
        if ((this->reliability & kReliable) == 0 || this->orderingFlags == 0)
            len += sprintf(buf+len, "(%x/%x)", this->orderingIndex, this->orderingChannel);
        else
            len += sprintf(buf+len, "(%x/%x|%c)", this->orderingIndex, this->orderingChannel, orderable[this->orderingFlags&0x3]);
    }
    if (this->splitPacketCount > 0)
        len += sprintf(buf+len, "[%x/%x|%x]", this->splitPacketIndex, this->splitPacketCount, this->splitPacketId); 
    len += sprintf(buf+len, "%-.*s", std::min(bits2bytes(this->bitLength), 64), this->payload);
    return std::string(buf, len);
}
//估计报文头大小,实际可能会小些
int Packet::headerBitSize(bool splitting) const
{
    int headerLength = 16;//messageNumber
    headerLength += 2; //reliability
    headerLength++;//(splitPacketCount || splitPacketIndex)
    if (this->reliability & kSequence) {
        if (this->reliability & kReliable)
            headerLength += 2;//orderingFlags
        headerLength += 8;//orderingChannel
        headerLength += 32;//compressed orderingIndex
    }
    if (splitting || this->splitPacketCount || this->splitPacketIndex){
        headerLength++;//whether splitPacketCount > 0
        headerLength +=16;//compressed splitPacketId
        headerLength +=16;//compressed splitPacketIndex
        headerLength +=16;//compressed splitPacketCount
    }
    headerLength += 16;//compressed bitLength
    return (headerLength+7)&~7;
}
//尝试流中是否含有数据包
bool Packet::maybePacket(const BitStream& bs)
{
    const int len = bs.bitSize - bs.bitRead;
    //messageNumber+reliability+splitPacketCount>0+compressed bitLength
    return len > 16+2+1+16;
}

bool Packet::writeToStream(BitStream& bs) const
{
    if (bs.bitWrite + bitSize() >= bs.bitSize)
        return false;
    const int bitsWrite = bs.bitWrite;//记录原写位置,用于恢复
    const bool bSplited = (this->splitPacketCount || this->splitPacketIndex);
    JIF(bs.write(this->messageNumber));
    JIF(bs.write(&this->reliability, 2));
    JIF(bs.write(bSplited));
    if (this->reliability & kSequence) {
        if (this->reliability & kReliable)
            JIF(bs.write(&this->orderingFlags, 2));
        JIF(bs.write(this->orderingChannel));
        JIF(bs.compressWrite(this->orderingIndex,true));
    }
    if (bSplited) {
        JIF(bs.write(this->splitPacketCount > 0));
        JIF(bs.compressWrite(this->splitPacketId, true));
        JIF(bs.compressWrite(this->splitPacketIndex, true));
        if (this->splitPacketCount > 0)
            JIF(bs.compressWrite(this->splitPacketCount, true));
    }
    JIF(bs.compressWrite(this->bitLength,true));
    JIF(bs.alignedWrite(this->payload, this->bitLength));
    return true;
clean:
    bs.bitWrite = bitsWrite;
    return false;
}
PacketPtr Packet::readFromStream(BitStream& bs)
{
    uint8_t buf[kMaxMtuBytes];
    Packet tmp;
    bool bSplited;
    memset(&tmp, 0, sizeof(Packet));
    const int bitRead = bs.bitRead;//记录原写位置,用于恢复
    JIF(bs.read(tmp.messageNumber));
    JIF(bs.read(&tmp.reliability, 2));
    JIF(bs.read(bSplited));
    if (tmp.reliability & kSequence) {
        if (tmp.reliability & kReliable)
            JIF(bs.read(&tmp.orderingFlags, 2));
        JIF(bs.read(tmp.orderingChannel));
        JIF(bs.compressRead(tmp.orderingIndex,true));
    }
    if (bSplited) {
        bool splitPacketCount;
        JIF(bs.read(splitPacketCount));
        JIF(bs.compressRead(tmp.splitPacketId, true));
        JIF(bs.compressRead(tmp.splitPacketIndex, true));
        if (splitPacketCount)
            JIF(bs.compressRead(tmp.splitPacketCount, true));
    }
    JIF(bs.compressRead(tmp.bitLength,true));
    JIF(bs.alignedRead(buf, tmp.bitLength));

    return tmp.clone(buf, tmp.bitLength);
clean:
    bs.bitRead = bitRead;
    return 0;
}
PacketPtr Packet::clone(const uint8_t *buf, int bitLength) const
{
    Packet* p = new Packet;
    memcpy(p, this, sizeof(Packet));
    p->bitLength = bitLength;
    const int bytes = bits2bytes(bitLength);
    p->payload = new uint8_t[bytes];
    memcpy(p->payload, buf, bytes);
    return p;
}
PacketPtr Packet::clone(int byteOffset, int bitLength) const
{
    return clone(this->payload+byteOffset, bitLength);
}
void Packet::release()
{
    delete this->payload;
    delete this;
}
PacketPtr Packet::merge(PacketPtr packets[], int num)
{
    return 0;
}
PacketPtr Packet::Reliable(const void* data, int bitLength)
{
    Packet* p = new Packet;
    memset(p, 0, sizeof(Packet));
    p->reliability = kReliable;
    const int bytes = bits2bytes(bitLength);
    p->payload = new uint8_t[bytes];
    p->bitLength = bitLength;
    p->splitPacketCount = 0;
    memcpy(p->payload, data, bytes);
    return p; 
}
}
