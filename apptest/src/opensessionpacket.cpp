#include "opensessionpacket.hpp"
#include <iomanip>
#include <iostream>
#include <string>
#include <arpa/inet.h>

OpenSessionPacket::OpenSessionPacket()
    : opensession_source_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      opensession_destination_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + sizeof(uint32_t))),
      opensession_session_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + 2 * sizeof(uint32_t))) {}

OpenSessionPacket::OpenSessionPacket(PacketBase &&pkt_base)
    : PacketBase(std::move(pkt_base)),
      opensession_source_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      opensession_destination_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + sizeof(uint32_t))),
      opensession_session_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + 2 * sizeof(uint32_t))) {}

OpenSessionPacket::OpenSessionPacket(const OpenSessionPacket &other)
    : PacketBase(other),
      opensession_source_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      opensession_destination_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + sizeof(uint32_t))),
      opensession_session_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + 2 * sizeof(uint32_t))) {}

OpenSessionPacket::OpenSessionPacket(OpenSessionPacket &&other) noexcept = default;

uint32_t *OpenSessionPacket::getsourcePtr()
{
    return opensession_source_ptr_;
}

uint32_t *OpenSessionPacket::getdesPtr()
{
    return opensession_destination_ptr_;
}

uint32_t *OpenSessionPacket::getsessidPtr()
{
    return opensession_session_ptr_;
}

bool *OpenSessionPacket::getoutboundPtr()
{
    return nullptr;
}

void OpenSessionPacket::constructopensessionpacket(uint32_t sourceip_, uint32_t desip_, uint32_t session_id, bool is_outbound)
{

    uint16_t intvalue = static_cast<uint16_t>(PacketType::OPENSESSION);
    std::memcpy(this->getBufferPtr(), &intvalue, sizeof(uint16_t));
    uint16_t length = OPENSESSIONHEADER;
    std::memcpy(this->getBufferPtr() + sizeof(uint16_t), &length, sizeof(uint16_t));
    this->setBufferSize(BASE_HEADER_SIZE + OPENSESSIONHEADER);
    *this->opensession_source_ptr_ = sourceip_;
    *this->opensession_destination_ptr_ = desip_;
    *this->opensession_session_ptr_ = session_id;
    *this->is_outbound_ptr = is_outbound;
}

void OpenSessionPacket::constructclosesessionpacket(uint32_t sourceip_, uint32_t desip_, uint32_t session_id, bool is_outbound)
{
    uint16_t intvalue = static_cast<uint16_t>(PacketType::CLOSESESSION);
    std::memcpy(this->getBufferPtr(), &intvalue, sizeof(uint16_t));
    uint16_t length = OPENSESSIONHEADER;
    std::memcpy(this->getBufferPtr() + sizeof(uint16_t), &length, sizeof(uint16_t));
    this->setBufferSize(BASE_HEADER_SIZE + OPENSESSIONHEADER);
    *this->opensession_source_ptr_ = sourceip_;
    *this->opensession_destination_ptr_ = desip_;
    *this->opensession_session_ptr_ = session_id;
    *this->is_outbound_ptr = is_outbound;
}