#include "rdp_channel.hpp"
#include "rdp_rwqueue.hpp"
#include "rdp_packet.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_bitstream.hpp"
#include <gtest/gtest.h>
#define EXPECT_CLAMP(x, min, max)do{\
    EXPECT_LE(x, max);\
    EXPECT_GE(x, min);\
}while(0)

using namespace rdp;
namespace {
struct EndPoint : public transport::SenderIFace, public Thread
{
    struct MTU
    {
        uint8_t data[1500];
        int size;
    };
    typedef RWQueue<MTU>  Line;

    Line line_;
	Channel channel_;
    SharedPtr<BitrateIFace> sentBitrate_;
    SharedPtr<BitrateIFace> receiveBitrate_;
    transport::ReceiverIFace* receiver_;
    EndPoint():
        sentBitrate_(BitrateIFace::create()),
        receiveBitrate_(BitrateIFace::create()),
		channel_(*this)
    {}
	void start(transport::ReceiverIFace& receiver)
	{
		receiver_ = &receiver;
		channel_.start();
		Thread::start();
	}
    virtual void send_W(BitStream& bs, const Time& now)
    {
        sentBitrate_->update(bs.bitSize, now);
        MTU* pMtu = line_.writeLock();
        pMtu->size = bits2bytes(bs.bitSize);
        memcpy(pMtu->data, bs.bitBuffer, pMtu->size);
        line_.writeUnlock();
    }
    virtual void run(void)
    {
        MTU* pMtu;
        while(state() != kJoining) {
            while (receiver_ && (pMtu = line_.readLock())) {
                const Time& now = Time::now();
                BitStream bs(pMtu->data,pMtu->size*8, -1);
                receiveBitrate_->update(bs.bitSize, now);
                receiver_->onReceived_R(bs, now);
                line_.readUnlock();
                printf("=====%d\n",receiveBitrate_->kbps(now));
            }
            Time::sleep(Time::MS(1));
        }
    }
    int sentKbps(const Time& now) const
    {
        return sentBitrate_->kbps(now);
    }
    int receiveKbps(const Time& now) const
    {
        return receiveBitrate_->kbps(now);
    }
};
struct PipLine
{
    EndPoint left_, right_;
    void start()
    {
        left_.start(right_.channel_);
        right_.start(left_.channel_);
    }
	void stop()
	{
		left_.join();
		right_.join();
		left_.channel_.join();
		right_.channel_.join();
	}
};

}

TEST(Channel, FixedBandWidth)
{
	const int rightKbps = rand() % 271 + 40;
	const int leftKbps  = rand()  % 251 + 40;
    PipLine line;
	line.left_.channel_.fixBandwidth(true, 1000);
	line.right_.channel_.fixBandwidth(true, 1000);
	line.start();

	for(int i=0;i<100;++i) {
        Time now(Time::now());
		line.left_.channel_.sendPacket(Packet::Reliable("aaaaa", 40), now);
		line.right_.channel_.sendPacket(Packet::Reliable("bbbbb", 40), now);
        Time::sleep(Time::MS(50));
	}
	Time now(Time::now());
	int leftSendKbps = line.left_.sentKbps(now);
	int rightSendKbps = line.right_.sentKbps(now);
    int rightTargetKbps = line.left_.channel_.kbps();
	line.stop();
    EXPECT_CLAMP(leftSendKbps,  leftKbps - 6, leftKbps + 6);
    EXPECT_CLAMP(rightSendKbps, rightKbps- 6, rightKbps+ 6);
}

