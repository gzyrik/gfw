#include "rdp_config.hpp"
#include "rdp_time.hpp"
#include <gtest/gtest.h>
#ifdef _WIN32
#include <Windows.h>
#endif
using namespace rdp;
TEST(Time, basic)
{
    Time n(Time::now());
    EXPECT_EQ(n.nanosec()/1000, n.microsec());
    EXPECT_EQ(n.microsec()/1000, n.millisec());
    EXPECT_EQ(n.millisec()/1000, n.sec());
}

TEST(Utils, seqDelta)
{
    const int maxSize = 512;
    EXPECT_EQ(seqDelta(0, 0x10000-maxSize, maxSize), maxSize);
    EXPECT_EQ(seqDelta(0x10000-maxSize, 0, maxSize), -maxSize);
    EXPECT_EQ(seqDelta(0xffff, 0), -1);
    EXPECT_EQ(seqDelta(0, 0xffff), 1);
    EXPECT_EQ(seqDelta(0xff, 0xf), 0xff-0xf);
}

TEST(Time, now)
{
    for(int i=0;i<100; ++i) {
#ifdef _WIN32
        timeBeginPeriod(4);
#endif
        Time t0(Time::now());
        Time::sleep(Time::MS(15));
        Time t1 = Time::now();
#ifdef _WIN32
        timeEndPeriod(4);
#endif
        Time delta(t1 - t0);
        EXPECT_LE(delta.millisec(), 20);
        EXPECT_GE(delta.millisec(), 10);
    }
}
