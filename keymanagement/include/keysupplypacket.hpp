#ifndef KEYSUPPLYPACKET_HPP
#define KEYSUPPLYPACKET_HPP

#include "packetbase.hpp"

#define KEYSUPPLYHEADER 4

/**
 * @brief structure:
 *        |<-4 bytes->|<-4 bytes->|<----------512 bytes---------->|
 *        +-----------+-----------+-------------------------------+
 *        | Base hdr  |     seq   |           key value           |
 *        +-----------+-----------+-------------------------------+
 *        |<-----------------520 bytes max----------------------->|
 */

class KeySupplyPacket : public PacketBase
{
private:
    uint32_t *keysupply_seqptr_;
    uint8_t *keysupply_payloadptr_;

public:
    KeySupplyPacket();
    explicit KeySupplyPacket(PacketBase &&pkt_base);

    KeySupplyPacket(const KeySupplyPacket &other);
    KeySupplyPacket(KeySupplyPacket &&other) noexcept;

    void ConstrctKeySupplyPacket(uint32_t seq, const std::string &getkeyvalue);

    uint8_t *getKeyBufferPtr();
    uint32_t *getSeqPtr();
};

using KeySupplyPacketPtr = std::shared_ptr<KeySupplyPacket>;

#endif