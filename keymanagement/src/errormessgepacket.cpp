#include "errormessgepacket.hpp"
#include <iostream>
#include <string>

ErrorMessagePacket::ErrorMessagePacket()
    : error_type_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)) {}

ErrorMessagePacket::ErrorMessagePacket(PacketBase &&pkt_base)
    : PacketBase(std::move(pkt_base)),
      error_type_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)) {}

ErrorMessagePacket::ErrorMessagePacket(const ErrorMessagePacket &other)
    : PacketBase(other),
      error_type_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)) {}

ErrorMessagePacket::ErrorMessagePacket(ErrorMessagePacket &&other) noexcept = default;

uint32_t *ErrorMessagePacket::geterrortypePtr()
{
    return error_type_ptr_;
}

void ErrorMessagePacket::constructErrorMessagePacket(uint32_t errortype)
{
    uint16_t intvalue = static_cast<uint16_t>(PacketType::ERRORMESSAGE);
    std::memcpy(this->getBufferPtr(), &intvalue, sizeof(uint16_t));
    uint16_t length = ERRORMESSAGEHEADER;
    std::memcpy(this->getBufferPtr() + sizeof(uint16_t), &length, sizeof(uint16_t));
    this->setBufferSize(BASE_HEADER_SIZE + ERRORMESSAGEHEADER);
    *this->error_type_ptr_ = errortype;
}