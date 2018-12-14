#ifndef __RDP_USEDETECTOR_HPP__
#define __RDP_USEDETECTOR_HPP__
#include "rdp_basictypes.hpp"
namespace rdp {
/**
 * ����ʹ�ü����
 *
 * �����յ����ݺ�ʱ���,������д���ʹ�����
 */
class UseDetector
{
    /**
     * ����ʹ�����
     */
    enum BandwidthUsage
    {
        /** ����ʹ����� */
        kNormal,
        /** ��������,�׷���ӵ�� */
        kOverusing,
        /** ���ڴ���,û�г������ */
        kUnderUsing
    };

    /** 
     * ��ǰ���,��������޵Ĺ�ϵ
     */
    enum RateControlRegion
    {
        /** �ӽ���������, ����̽��kOveruing���ж� */
        kNearMax,
        /** ������������, ����̽��kUnderUsing���ж� */
        kAboveMax,
        /** δ֪״̬, �ָ�̽�����ж�*/
        kMaxUnknown
    };

    /**
     * ���õ�ǰ���������޵Ĺ�ϵ
     */
    void setRegion(enum RateControlRegion region);

    /** ���ص�ǰ����ʹ����� */
    enum BandwidthUsage state() const;

    /** ���ü�� */
    void reset();

    /**
     * �����յ�����,������ʹ��״̬
     *
     * @param[in] bitLength     �յ����ݵ�λ��
     * @param[in] peerStampMS   ���ݷ���ʱ��ʱ���(Զ��ʱ��)
     * @param[in] localNowMS    �����յ�ʱ��ʱ��(����ʱ��)
     */
    void onReceived(int bitLength, const Time& peerStampMS, const Time& localNowMS);
};
}
#endif
