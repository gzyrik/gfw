struct Packet
{
    unsigned flags;
    unsigned char channel;
    unsigned  len;
};
class PacketList
{
};
struct rdp_t
{
    PacketList sendPackets[RDP_PRIORITY_HIGH+1];

};

int rdp_send(rdp_t* rdp, const void *data, unsigned len, unsigned flags, unsigned char channel)
{
    rdp->sendPackets[(flags>>3)&0x3].push_back(new Packet(data, len, flags, channel));
}


