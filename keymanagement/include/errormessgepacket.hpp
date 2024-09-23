#ifndef ERRORMESSAGEPACKET_HPP
#define ERRORMESSAGEPACKET_HPP

#include "packetbase.hpp"

/**
 * @brief structure:
 *        |<-4 bytes->|<-4 bytes->|
 *        +-----------+-----------+
 *        | Base hdr  | Error Type|
 *        +-----------+-----------+
 *        |<-------8 bytes max--->|
 */

#define ERRORMESSAGEHEADER 8

enum class ErrorType : uint32_t
{
    OPENSESSIONERROR = 0,
    GETKEYERROR,
    UNKONWNMESSAGE
};

class ErrorMessagePacket : public PacketBase
{
private:
    uint32_t *error_type_ptr_;

public:
    ErrorMessagePacket();
    explicit ErrorMessagePacket(PacketBase &&pkt_base);

    ErrorMessagePacket(const ErrorMessagePacket &other);
    ErrorMessagePacket(ErrorMessagePacket &&other) noexcept;

    uint32_t *geterrortypePtr();

    void constructErrorMessagePacket(uint32_t errortype);
};

using ErrorMessagePacketPtr = std::shared_ptr<ErrorMessagePacket>;

#endif
