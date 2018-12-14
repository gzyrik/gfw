#include "rdp_config.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_ringbuffer.hpp"
namespace {
using namespace rdp;

struct Bitrate : public BitrateIFace
{
    /**
     * 构造码率统计器
     *
     * @param[in] kbps      估计带宽
     * @param[in] duration  滑动窗口毫秒数
     * @param[in] bitMTU    最大网络包位数
     */
    Bitrate(const int kbps, const Time& duration, const int bitMTU):
        sampler_(duration, kbps*1000/bitMTU)
    {
    }
    /**
     * 更新传输长度,更新采样点
     * @param[in] 传输的位数
     * @param[in] 当前时刻
     */
    void update(const int bitLength, const Time& now)
    {
        sampler_.update(bitLength, now);
    }

    /** 估算该点码率 */
    int kbps(const Time& now)
    {
        return sampler_.value(now)/1000;
    }
    CapacitySampler<int>   sampler_;
};
}
namespace rdp {
BitrateIFace* BitrateIFace::create()
{
    return new Bitrate(200, Time::MS(1000), 8*kMaxMtuBytes);
}
}
