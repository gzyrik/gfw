#ifndef __RDP_BWESTIMATOR_HPP__
#define __RDP_BWESTIMATOR_HPP__
#include "rdp_basictypes.hpp"
namespace rdp{
/**
 * Bandwidth Estimator ���д��������
 * 
 * �������д���ʹ�����(UseDetector)�����д���ͳ��(Bitrate),
 * �������д����С,���ڲ����Զ˵�TMMBR
 */
struct BWEIFace
{
    static BWEIFace* create();

    virtual ~BWEIFace(){}

    /**
     * ����ʵ���յ�����,���¹���ֵ
     *
     * @param[in] bitLength �յ����ݳ���
     * @param[in] peerStampMS �����ݵ�ʱ���
     * @param[in] messageNumber �����ݵ����
     * @param[in] now ��ǰʱ��
     */
    virtual void onReceived_R(const int bitLength, const Time& peerStampMS, const uint32_t messageNumber, const Time& now) = 0;

    /**
     * ����RTT���ڲ��Ĺ�������,�������д���
     */
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now) = 0;
}; 
}
#endif
