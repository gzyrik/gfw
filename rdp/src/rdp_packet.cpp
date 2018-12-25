#include "rdp_config.hpp"
#include "rdp_packet.hpp"
#include "rdp_bitstream.hpp"
#include <sstream>
#include <algorithm>
namespace rdp {
std::string Packet::toString() const
{
    static const char* reliables ="USRO";
    char buf[kMaxMtuBytes];

    int len = sprintf(buf, "%X%c%x", this->messageNumber, reliables[this->reliability&0x3], this->bitLength);
    if (this->reliability & kSequence)
        len += sprintf(buf+len, "(%x/%x)", this->orderingIndex, this->orderingChannel);
    if (this->splitPacketCount > 0)
        len += sprintf(buf+len, "[%x/%x|%x]", this->splitPacketIndex, this->splitPacketCount, this->splitPacketId); 
    len += sprintf(buf+len, ": %-.*s", std::min(bits2bytes(this->bitLength), 64), this->payload);
    return std::string(buf, len);
}
//估计报文头大小,实际可能会小些
int Packet::headerBitSize(bool splitting) const
{
    int headerLength = 32;//messageNumber
    headerLength += 2; //reliability
    if (this->reliability & kSequence) {
        headerLength += 8;//orderingChannel
        headerLength += 32;//compressed orderingIndex
    }
    headerLength++;//(splitPacketCount > 0)
    if (splitting || this->splitPacketCount > 0){
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
    const int len = bs.bitWrite - bs.bitRead;
    //messageNumber+reliability+splitPacketCount>0+compressed bitLength
    return len >= 32+0+0+16;
}

bool Packet::writeToStream(BitStream& bs) const
{
    if (bs.bitWrite + bitSize() >= bs.bitSize)
        return false;
    const int bitWrite = bs.bitWrite;//记录原写位置,用于恢复
    /*
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       | messageNumber |  R  |orderingChannel|orderingIndex|S|
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       S=bSplited,  R=reliability
       */
    JIF(bs.compressWrite(this->messageNumber));
    JIF(bs.write(&this->reliability, 3));
    if (this->reliability & kSequence) {
        JIF(bs.write(this->orderingChannel));
        JIF(bs.compressWrite(this->orderingIndex));
    }
    JIF(bs.write(this->splitPacketCount > 0));
    if ((this->splitPacketCount > 0)) {
        JIF(bs.compressWrite(this->splitPacketId));
        JIF(bs.compressWrite(this->splitPacketIndex));
        JIF(bs.compressWrite(this->splitPacketCount));
    }
    JIF(bs.compressWrite(this->bitLength));
    JIF(bs.alignedWrite(this->payload, this->bitLength));
    return true;
clean:
    bs.bitWrite = bitWrite;
    return false;
}
PacketPtr Packet::readFromStream(BitStream& bs)
{
    const int bitRead = bs.bitRead;//记录原写位置,用于恢复
    uint8_t buf[kMaxMtuBytes];
    Packet tmp = {0};
    bool bSplited;
    /*
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       | messageNumber |  R  |orderingChannel|orderingIndex|S|
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       S=bSplited,  R=reliability
       */
    JIF(bs.compressRead(tmp.messageNumber));
    JIF(bs.read(&tmp.reliability, 3));
    if (tmp.reliability & kSequence) {
        JIF(bs.read(tmp.orderingChannel));
        JIF(bs.compressRead(tmp.orderingIndex));
    }
    JIF(bs.read(bSplited));
    if (bSplited) {
        JIF(bs.compressRead(tmp.splitPacketId));
        JIF(bs.compressRead(tmp.splitPacketIndex));
        JIF(bs.compressRead(tmp.splitPacketCount));
    }
    JIF(bs.compressRead(tmp.bitLength));
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
PacketPtr Packet::Create(enum Reliability reliability, const void* data, int bitLength, uint8_t orderingChannel)
{
    Packet* p = new Packet;
    memset(p, 0, sizeof(Packet));
    p->reliability = reliability;
    p->orderingChannel = orderingChannel;
    const int bytes = bits2bytes(bitLength);
    p->payload = new uint8_t[bytes];
    p->bitLength = bitLength;
    p->splitPacketCount = 0;
    memcpy(p->payload, data, bytes);
    return p; 
}
}
