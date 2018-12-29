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
    /**
     * ����ʵ���յ�����,���¹���ֵ
     *
     * @param[in] bitLength �յ����ݳ���
     * @param[in] peerStamp �����ݵ�ʱ���
     * @param[in] recvKbps ��ǰ��������
     * @param[in] now ��ǰʱ��
     */
    virtual void onReceived_R(const int bitLength, const Time& peerStamp, const int recvKbps, const Time& now) = 0;

    /**
     * ����RTT���ڲ��Ĺ�������,�������д���
     */
    virtual int tmmbrKbps_W(const Time& rtt, const Time& now) = 0;

    virtual void fixKbps(const int minKbps, const int maxKbps) = 0;

    static BWEIFace* create();
    virtual ~BWEIFace(){}
}; 
}
#endif
