#ifndef __RDP_FEC_HPP__
#define __RDP_FEC_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_transport.hpp"
namespace rdp {
struct FECIFace : public transport::SenderIFace, public transport::ReceiverIFace
{
    //д��FEC��ʶ
    virtual void markFec_W(BitStream& bs, const Time& nowNS) = 0;

    //����FEC���͵ı�����
    virtual void fixRate(const uint8_t minRate, const uint8_t maxRate, const bool enableDecode) = 0;

    static FECIFace* create(transport::SenderIFace& sender, transport::ReceiverIFace& receiver);
    virtual ~FECIFace(){}
};
}
#endif
