#ifndef OPENSESSIONPACKET_HPP
#define OPENSESSIONPACKET_HPP

#include "packetbase.hpp"

/**
 * @brief structure:
 *        |<-4 bytes->|<-4 bytes->|<-4 bytes->|<-4 bytes->|<-1 bytes->|
 *        +-----------+-----------+-----------------------------------+
 *        | Base hdr  |  sourece  | destinaton|   sess_id |is_outbound|
 *        +-----------+-----------+-----------------------------------+
 *        |<---------------------17 bytes max------------------------>|
 */

#define OPENSESSIONHEADER 13

class OpenSessionPacket : public PacketBase
{
private:
    uint32_t *opensession_source_ptr_;
    uint32_t *opensession_destination_ptr_;
    uint32_t *opensession_session_ptr_;
    bool *is_outbound_ptr;

public:
    OpenSessionPacket();
    explicit OpenSessionPacket(PacketBase &&pkt_base);

    OpenSessionPacket(const OpenSessionPacket &other);
    OpenSessionPacket(OpenSessionPacket &&other) noexcept;

    uint32_t *getsourcePtr();
    uint32_t *getdesPtr();
    uint32_t *getsessidPtr();
    bool *getoutboundPtr();

    void constructopensessionpacket(uint32_t sourceip_, uint32_t desip_, uint32_t session_id,bool is_outbound);

    void constructclosesessionpacket(uint32_t sourceip_, uint32_t desip_, uint32_t session_id,bool is_outbound);
};

using OpenSessionPacketPtr = std::shared_ptr<OpenSessionPacket>;

#endif