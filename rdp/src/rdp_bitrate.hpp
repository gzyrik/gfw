#ifndef __RDP_BITRATE_HPP__
#define __RDP_BITRATE_HPP__
#include "rdp_basictypes.hpp"
namespace rdp {
/** 
 * ƽ������
 */
struct BitrateIFace
{
    /**
     * ���´��䳤��,���²�����
     * @param[in] �����λ��
     * @param[in] ��ǰʱ��
     */
    virtual void update(const int bitLength, const Time& nowNS) = 0;

    /** ���ص�ǰ����,����Time���������1MS,����kbps����� */
    virtual int kbps(const Time& nowNS) = 0;

    static BitrateIFace* create();
    virtual ~BitrateIFace(){}
};

}
#endif
