#ifndef __RDP_BITRATE_HPP__
#define __RDP_BITRATE_HPP__
#include "rdp_basictypes.hpp"
namespace rdp {
/** 
 * 平均码率
 */
struct BitrateIFace
{
    /**
     * 更新传输长度,更新采样点
     * @param[in] 传输的位数
     * @param[in] 当前时刻
     */
    virtual void update(const int bitLength, const Time& nowNS) = 0;

    /** 返回当前码率,由于Time的误差至少1MS,允许kbps的误差 */
    virtual int kbps(const Time& nowNS) = 0;

    static BitrateIFace* create();
    virtual ~BitrateIFace(){}
};

}
#endif
