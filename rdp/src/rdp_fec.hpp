#ifndef __RDP_FEC_HPP__
#define __RDP_FEC_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_transport.hpp"
namespace rdp {
struct FECIFace : public transport::SenderIFace, public transport::ReceiverIFace
{
    //写入FEC标识
    //@param[in] bwBits 当前可发送位
    virtual void markFec_W(BitStream& bs, const int bwBits, const Time& nowNS) = 0;

    //设置FEC发送的保护比[0,0xff]
    //@param[in] enableDecode 是否FEC解码,通常只用于调试
    virtual void fixRate(const uint8_t minRate, const uint8_t maxRate, const bool enableDecode) = 0;

    static FECIFace* create(transport::SenderIFace& sender, transport::ReceiverIFace& receiver);
    virtual ~FECIFace(){}
};
}
#endif
