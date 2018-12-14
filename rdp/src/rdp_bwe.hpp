#ifndef __RDP_BWESTIMATOR_HPP__
#define __RDP_BWESTIMATOR_HPP__
#include "rdp_basictypes.hpp"
namespace rdp{
/**
 * Bandwidth Estimator 下行带宽估计器
 * 
 * 根据下行带宽使用情况(UseDetector)和下行带宽统计(Bitrate),
 * 估计下行带宽大小,用于产生对端的TMMBR
 */
struct BWEIFace
{
    static BWEIFace* create();

    virtual ~BWEIFace(){}

    /**
     * 根据实际收到数据,更新估计值
     *
     * @param[in] bitLength 收到数据长度
     * @param[in] peerStampMS 该数据的时间截
     * @param[in] messageNumber 该数据的序号
     * @param[in] now 当前时刻
     */
    virtual void onReceived_R(const int bitLength, const Time& peerStampMS, const uint32_t messageNumber, const Time& now) = 0;

    /**
     * 根据RTT和内部的估计趋势,估计下行带宽
     */
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now) = 0;
}; 
}
#endif
