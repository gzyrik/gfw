#include "rdp_config.hpp"
#include "rdp_bwe.hpp"
#include "rdp_time.hpp"
#include "bwe/BWE_rate_control.h"
namespace rdp {
namespace {
/*
 * 估计下行带宽大小,产生对端的TMMBR
 */
class BWE: public BWEIFace
{
    int minKbps_, maxKbps_;
    virtual void onReceived_R(const int bitLength, const Time& peerStampMS, const int recvKbps, const Time& now) override
    {
        remoteControl_.UpdatePacket(peerStampMS.millisec(), recvKbps, bitLength, now.millisec());
    }
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now) override
    {
        int kbps = remoteControl_.UpdateBandwidthEstimate(rtt.millisec(), now.millisec(), 1, false);
        if (kbps < minKbps_)
            kbps = minKbps_ ;
        else if (maxKbps_ && kbps > maxKbps_)
            kbps = maxKbps_;
        return kbps;
    }
    virtual void fixKbps(const int minKbps, const int maxKbps) override
    {
        minKbps_ = minKbps;
        maxKbps_ = maxKbps;
    }

    bwe::RemoteRateControl  remoteControl_;
public:
    BWE():minKbps_(0),maxKbps_(0){};
};
}

BWEIFace* BWEIFace::create()
{
    return new BWE;
}
}
