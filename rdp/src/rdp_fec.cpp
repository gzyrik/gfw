#include "rdp_basictypes.hpp"
#include "rdp_fec.hpp"
namespace rdp{
namespace {
class Fec : public FecIFace
{
    transport::SenderIFace& sender_;
    transport::ReceiverIFace& receiver_;
    const int mtu_;

    virtual void send_W(BitStream& bs, const Time& now) 
    {
        sender_.send_W(bs, now);
    }
    virtual void onReceived_R(BitStream& bs, const Time& now)
    {
        receiver_.onReceived_R(bs, now);
    }
public:
    Fec(transport::SenderIFace& sender, transport::ReceiverIFace& receiver, const int mtu)
        : sender_(sender), receiver_(receiver), mtu_(mtu)
    {}
};
}

FecIFace* FecIFace::create(transport::SenderIFace& sender, transport::ReceiverIFace& receiver, const int mtu)
{
    return new Fec(sender, receiver, mtu);
}
}
