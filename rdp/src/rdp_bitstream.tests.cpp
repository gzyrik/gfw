#include "rdp_bitstream.hpp"
#include "rdp_packet.hpp"
#include <gtest/gtest.h>
using namespace rdp;
TEST(BitStream, construct)
{
    uint8_t buf[16];
    EXPECT_EQ(16*CHAR_BIT, bitsof(buf));
    {
        BitStream bs(buf, bitsof(buf));
        EXPECT_EQ(buf, bs.bitBuffer);
        EXPECT_EQ(bitsof(buf), bs.bitSize);
        EXPECT_EQ(0, bs.bitWrite);
        EXPECT_EQ(0, bs.bitRead);
    }
    {
        BitStream bs(buf, bitsof(buf), -1);
        EXPECT_EQ(buf, bs.bitBuffer);
        EXPECT_EQ(bitsof(buf), bs.bitSize);
        EXPECT_EQ(bitsof(buf), bs.bitWrite);
        EXPECT_EQ(0, bs.bitRead);
    }
    {
        BitStream bs(buf, bitsof(buf), 17, 7);
        EXPECT_EQ(buf, bs.bitBuffer);
        EXPECT_EQ(bitsof(buf), bs.bitSize);
        EXPECT_EQ(17, bs.bitWrite);
        EXPECT_EQ(7, bs.bitRead);
    }
}

TEST(BitStream, toString)
{
    uint8_t buf[]={0xFA, 0x74};
    EXPECT_EQ("11111010 01110100", BitStream(buf, bitsof(buf), -1, 0).toString());
    EXPECT_EQ("11010 01110100", BitStream(buf, bitsof(buf), -1, 3).toString());
    EXPECT_EQ("0 01110100", BitStream(buf, bitsof(buf), -1, 7).toString());
    EXPECT_EQ("", BitStream(buf, bitsof(buf), 5, 5).toString());
}
TEST(BitStream, write_read)
{
    uint8_t buf[2];
    uint8_t dat[2];
    BitStream bs0(buf, bitsof(buf), 3, 3);
    EXPECT_TRUE(bs0.write(true));
    EXPECT_TRUE(bs0.write(false));
    EXPECT_EQ("10", bs0.toString());
    {
        BitStream bs(bs0);
        EXPECT_TRUE(bs.write("\x39", 7, false));
        EXPECT_EQ("10001 1100", bs.toString());
        EXPECT_TRUE(bs.read(dat, 9, false));
        EXPECT_EQ(0x8E, dat[0]);
        EXPECT_EQ(0, dat[1]&0x80);
        bool b;
        EXPECT_EQ(bs.bitRead, bs.bitWrite);
        EXPECT_EQ(false, bs.read(b));
    }
    {
        BitStream bs(bs0);
        EXPECT_TRUE(bs.write("\x39", 7));
        EXPECT_EQ("10011 1001", bs.toString());
        EXPECT_TRUE(bs.read(dat, 9, true));
        EXPECT_EQ(0x9C, dat[0]);
        EXPECT_EQ(1, dat[1]&1);
        EXPECT_EQ(bs.bitRead, bs.bitWrite);
    }
    {
        BitStream bs(bs0);
        EXPECT_TRUE(bs.write("\x39\x7A", 11, false));
        EXPECT_EQ("10001 11001011", bs.toString());
        EXPECT_EQ(false, bs.write(false));
        EXPECT_EQ("10001 11001011", bs.toString());
        EXPECT_TRUE(bs.read(dat, 13, false));
        EXPECT_EQ(0x8E, dat[0]);
        EXPECT_EQ(0x58, dat[1]&0xF8);
        EXPECT_EQ(bs.bitRead, bs.bitWrite);
    }
}
TEST(BitStream, alignedWrite_Read)
{
    uint8_t buf[10];
    unsigned out, dat = 0xdeadbeef;
    BitStream bs(buf, bitsof(buf), 3, 3);
    EXPECT_TRUE(bs.alignedWrite(&dat, 17));
    EXPECT_TRUE(bs.alignedRead(&out, 17));
    EXPECT_EQ(out&0x1FFFF, dat&0x1FFFF);
}
TEST(BitStream, zwrite_zread)
{
    uint8_t buf[sizeof(void*)+1];
    void* p = buf;
    BitStream bs0(buf, bitsof(buf), 3, 3);
    EXPECT_TRUE(bs0.compressWrite(p,true));
    {
        p = (void*)-1;
        BitStream bs(bs0);
        EXPECT_TRUE(bs.compressRead(p, true));
        EXPECT_EQ(bs.bitRead, bs.bitWrite);
        EXPECT_EQ(buf, p);
    }
    {
        p = 0;
        BitStream bs(bs0);
        EXPECT_TRUE(bs.compressRead(p, true));
        EXPECT_EQ(bs.bitRead, bs.bitWrite);
        EXPECT_EQ(buf, p);
    }
}
TEST(BitStream, packet)
{
    uint8_t data[4];
    uint8_t buf[256];
    Packet p;
    int bitsize;
    p.payload = data;
    p.bitLength = 17;
    p.messageNumber = 345677;
    p.reliability = Packet::kSequence;
    p.splitPacketCount = 0;
    bitsize = p.bitSize();
    {
        BitStream bs(buf, bitsof(buf));
        EXPECT_TRUE(p.writeToStream(bs));
        EXPECT_LE(bs.bitWrite, bitsize);
        EXPECT_GE(bs.bitWrite, bitsize-8);
        PacketPtr p1 = Packet::readFromStream(bs);
        ASSERT_TRUE(p1 != 0);
        EXPECT_EQ(p1->messageNumber, p.messageNumber);
        EXPECT_EQ(p1->reliability, p.reliability);
        EXPECT_EQ(p1->bitLength, p.bitLength);
        p1->release();
    }
}
