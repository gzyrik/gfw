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
    virtual void onReceived_R(const int bitLength, const Time& peerStampMS, const uint32_t messageNumber, const Time& now);
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now);
    bwe::RemoteRateControl  remoteControl_;
};

void BWE::onReceived_R(const int bitLength, const Time& peerStampMS, const uint32_t messageNumber, const Time& now)
{
    remoteControl_.UpdatePacket(peerStampMS.millisec(), messageNumber, bitLength, now.millisec());
}

int BWE::tmmbrKbps_W(const Time& rtt, const Time& now)
{
    return remoteControl_.UpdateBandwidthEstimate(rtt, now, 1, false);
}
}

BWEIFace* BWEIFace::create()
{
    return new BWE;
}
}
