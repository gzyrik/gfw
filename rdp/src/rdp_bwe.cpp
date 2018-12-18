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
    virtual void onReceived_R(const int bitLength, const Time& peerStampMS, const int recvKbps, const Time& now) override
    {
        remoteControl_.UpdatePacket(peerStampMS.millisec(), recvKbps, bitLength, now.millisec());
    }
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now) override
    {
        return remoteControl_.UpdateBandwidthEstimate(rtt.millisec(), now.millisec(), 1, false);
    }
    bwe::RemoteRateControl  remoteControl_;
};
}

BWEIFace* BWEIFace::create()
{
    return new BWE;
}
}
