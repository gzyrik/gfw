#ifndef __RDP_SOCKET_HPP__
#define __RDP_SOCKET_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_time.hpp"
namespace rdp {
namespace transport {
/**
 * �ⲿ������ջص�
 */
struct ReceiverIFace
{
    virtual ~ReceiverIFace(){}
    //R�߳�, ��ȡ����������
    virtual void onReceived_R(BitStream& bs, const Time& now) = 0;
};
/**
 * �ⲿ���䷢�ͽӿ�
 */
struct SenderIFace
{
    virtual ~SenderIFace(){}
    virtual void send_W(BitStream& bs, const Time& now) = 0;
};

/**
 * ����������ӿ�
 */
struct DisturberIFace : public ReceiverIFace, public SenderIFace
{
    struct Status
    {
        int kbps;
        Time delay;
        Time jitter;
        int lostPercent;
        int disordPercent;
        int queueLength;
    };
    //����������
    virtual void restart(const Status& stat) = 0;
    static ReceiverIFace* createReceiver(ReceiverIFace& receiver);
    static SenderIFace* createSender(SenderIFace& sender);
};
}

}
#endif