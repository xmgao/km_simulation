#ifndef SESSIONKEYSYNCPACKET_HPP
#define SESSIONKEYSYNCPACKET_HPP

#include "packetbase.hpp"

/**
 * @brief structure:
 *        |<-4 bytes->|<-4 bytes->|<-4 bytes->|
 *        +-----------+-----------+-----------+
 *        | Base hdr  | sess_id   |  key_seq  |
 *        +-----------+-----------+-----------+
 *        |<---------12 bytes max------------>|
 */

#define SESSIONKEYSYNCHEADER 8

class SessionKeySyncPacket : public PacketBase
{
private:
    uint32_t *sessionkeysync_sessionid_ptr_;
    uint32_t *sessionkeysync_keyseq_ptr_;

public:
    SessionKeySyncPacket();
    explicit SessionKeySyncPacket(PacketBase &&pkt_base);

    SessionKeySyncPacket(const SessionKeySyncPacket &other);
    SessionKeySyncPacket(SessionKeySyncPacket &&other) noexcept;

    uint32_t *getsessidPtr();
    uint32_t *getkeyseqPtr();

    void constructsessionkeysyncpacket(uint32_t session_id, uint32_t keyseq);
};

using SessionKeySyncPacketPtr = std::shared_ptr<SessionKeySyncPacket>;

#endif