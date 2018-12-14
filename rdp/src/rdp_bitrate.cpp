#include "rdp_config.hpp"
#include "rdp_bitrate.hpp"
#include "rdp_ringbuffer.hpp"
namespace {
using namespace rdp;

struct Bitrate : public BitrateIFace
{
    /**
     * ��������ͳ����
     *
     * @param[in] kbps      ���ƴ���
     * @param[in] duration  �������ں�����
     * @param[in] bitMTU    ��������λ��
     */
    Bitrate(const int kbps, const Time& duration, const int bitMTU):
        sampler_(duration, kbps*1000/bitMTU)
    {
    }
    /**
     * ���´��䳤��,���²�����
     * @param[in] �����λ��
     * @param[in] ��ǰʱ��
     */
    void update(const int bitLength, const Time& now)
    {
        sampler_.update(bitLength, now);
    }

    /** ����õ����� */
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
