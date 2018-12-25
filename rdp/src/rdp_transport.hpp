#ifndef __RDP_SOCKET_HPP__
#define __RDP_SOCKET_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_time.hpp"
namespace rdp {
namespace transport {
/**
 * 外部传输接收回调
 */
struct ReceiverIFace
{
    //R线程, 读取网络数据流
    virtual void onReceived_R(BitStream& bs, const Time& now) = 0;

    virtual ~ReceiverIFace(){}
};
/**
 * 外部传输发送接口
 */
struct SenderIFace
{
    virtual ~SenderIFace(){}
    virtual void send_W(BitStream& bs, const Time& now) = 0;
};

/**
 * 单向干扰器接口
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
    //重启干扰器
    virtual void restart(const Status& stat) = 0;
    static ReceiverIFace* create(ReceiverIFace& receiver, const Status& status);
    static SenderIFace*   create(SenderIFace& sender, const Status& status);
};
}

}
#endif
