#ifndef __RDP_FEC_HPP__
#define __RDP_FEC_HPP__
#include "rdp_basictypes.hpp"
#include "rdp_transport.hpp"
namespace rdp {
struct FecIFace : public transport::SenderIFace, public transport::ReceiverIFace
{
    static FecIFace* create(transport::SenderIFace& sender, transport::ReceiverIFace& receiver, const int mtu);
    virtual ~FecIFace(){}
};
}
#endif
