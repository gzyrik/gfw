#include "rdp_time.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_shared.hpp"
#include <gtest/gtest.h>
#include <cmath>//for sin()
#ifdef _WIN32
#include <Windows.h> //for timeEndPeriod()/timeBeginPeriod()
#endif
using namespace rdp;
namespace {
}
TEST(BitrateIFace, Tmmbr_100kbps)
{
    SharedPtr<BitrateIFace> bitrate(BitrateIFace::create());
    uint64_t accumulate = 0;
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    Time lastNS = Time::now();
    const Time startNS(lastNS);
    bitrate->update(0, startNS);
    for(int i=0;i<360;++i) {
        Time::sleep(Time::MS(20));
        const Time& nowNS = Time::now();
        const int tmmbrKbps = 100 + (int)(sin(i*3.1415926/180) * 50);
        const int bitLength = static_cast<int>((nowNS-lastNS).millisec()*tmmbrKbps);
        bitrate->update(bitLength, nowNS);
        lastNS = nowNS;
        const int kbps = bitrate->kbps(nowNS);
        //printf("-----%d\n", kbps);
        accumulate += kbps;
    }
#ifdef _WIN32
    timeEndPeriod(1);
#endif
    const int avg = (int)(accumulate/360);
    EXPECT_LE(avg, 105);
    EXPECT_GE(avg, 95);
}
