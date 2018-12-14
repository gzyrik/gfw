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
    int headerLength = 32;//messageNumber
    headerLength += 2; //reliability
    headerLength++;//whelther splitPacketCount > 0
    if (this->reliability & kSequence) {
        if (this->reliability & kReliable)
            headerLength += 2;//orderingFlags
        headerLength += 8;//orderingChannel
        headerLength += 32;//compressed orderingIndex
    }
    if(this->splitPacketCount > 0 || splitting){
        headerLength +=16;//splitPacketId
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
    if (len < 32+2+1+16)//messageNumber+reliability+ splitPacketCount > 0 + compressed bitLength
        return false;
    return true;
}

bool Packet::writeToStream(BitStream& bs) const
{
    if (bs.bitWrite + bitSize() >= bs.bitSize)
        return false;
    const int bitsWrite = bs.bitWrite;//记录原写位置,用于恢复
    JIF(bs.write(this->messageNumber));
    JIF(bs.write(&this->reliability, 2));
    JIF(bs.write(this->splitPacketCount > 0));
    if (this->reliability & kSequence) {
        if (this->reliability & kReliable)
            JIF(bs.write(&this->orderingFlags, 2));
        JIF(bs.write(this->orderingChannel));
        JIF(bs.compressWrite(this->orderingIndex,true));
    }
    if (this->splitPacketCount > 0) {
        JIF(bs.write(this->splitPacketId));
        JIF(bs.compressWrite(this->splitPacketIndex, true));
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
    uint8_t buf[15000];
    Packet tmp;
    bool split;
    memset(&tmp, 0, sizeof(Packet));
    const int bitRead = bs.bitRead;//记录原写位置,用于恢复
    JIF(bs.read(tmp.messageNumber));
    if (!tmp.messageNumber)//从1开始,0为非法值
        return 0;
    JIF(bs.read(&tmp.reliability, 2));
    JIF(bs.read(split));
    if (tmp.reliability & kSequence) {
        if (tmp.reliability & kReliable)
            JIF(bs.read(&tmp.orderingFlags, 2));
        JIF(bs.read(tmp.orderingChannel));
        JIF(bs.compressRead(tmp.orderingIndex,true));
    }
    if (split) {
        JIF(bs.read(tmp.splitPacketId));
        JIF(bs.compressRead(tmp.splitPacketIndex, true));
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
