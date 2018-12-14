#ifndef __RDP_USEDETECTOR_HPP__
#define __RDP_USEDETECTOR_HPP__
#include "rdp_basictypes.hpp"
namespace rdp {
/**
 * 带宽使用检测器
 *
 * 根据收到数据和时间差,检测下行带宽使用情况
 */
class UseDetector
{
    /**
     * 带宽使用情况
     */
    enum BandwidthUsage
    {
        /** 正常使用情况 */
        kNormal,
        /** 超过带宽,易发生拥塞 */
        kOverusing,
        /** 低于带宽,没有充分利用 */
        kUnderUsing
    };

    /** 
     * 当前检测,与带宽上限的关系
     */
    enum RateControlRegion
    {
        /** 接近带宽上限, 侧重探测kOveruing敏感度 */
        kNearMax,
        /** 超过带宽上限, 侧重探测kUnderUsing敏感度 */
        kAboveMax,
        /** 未知状态, 恢复探测敏感度*/
        kMaxUnknown
    };

    /**
     * 设置当前检测的与上限的关系
     */
    void setRegion(enum RateControlRegion region);

    /** 返回当前带宽使用情况 */
    enum BandwidthUsage state() const;

    /** 重置检测 */
    void reset();

    /**
     * 根据收到数据,检测带宽使用状态
     *
     * @param[in] bitLength     收到数据的位数
     * @param[in] peerStampMS   数据发送时的时间截(远端时间)
     * @param[in] localNowMS    数据收到时的时间(本地时间)
     */
    void onReceived(int bitLength, const Time& peerStampMS, const Time& localNowMS);
};
}
#endif
