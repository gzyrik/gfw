#ifndef __RDP_FEC_HPP__
#define __RDP_FEC_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_transport.hpp"
namespace rdp {
struct FECIFace : public transport::SenderIFace, public transport::ReceiverIFace
{
    //д��FEC��ʶ
    //@param[in] bwBits ��ǰ�ɷ���λ
    virtual void markFec_W(BitStream& bs, const int bwBits, const Time& nowNS) = 0;

    //����FEC���͵ı�����[0,0xff]
    //@param[in] enableDecode �Ƿ�FEC����,ͨ��ֻ���ڵ���
    virtual void fixRate(const uint8_t minRate, const uint8_t maxRate, const bool enableDecode) = 0;

    static FECIFace* create(transport::SenderIFace& sender, transport::ReceiverIFace& receiver);
    virtual ~FECIFace(){}
};
}
#endif
