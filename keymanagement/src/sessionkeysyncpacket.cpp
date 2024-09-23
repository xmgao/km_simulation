#include "sessionkeysyncpacket.hpp"
#include <iomanip>
#include <iostream>
#include <string>
#include <arpa/inet.h>

SessionKeySyncPacket::SessionKeySyncPacket()
    : sessionkeysync_sessionid_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      sessionkeysync_keyseq_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + sizeof(uint32_t))) {}

SessionKeySyncPacket::SessionKeySyncPacket(PacketBase &&pkt_base)
    : PacketBase(std::move(pkt_base)),
      sessionkeysync_sessionid_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      sessionkeysync_keyseq_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + sizeof(uint32_t))) {}

SessionKeySyncPacket::SessionKeySyncPacket(const SessionKeySyncPacket &other)
    : PacketBase(other),
      sessionkeysync_sessionid_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      sessionkeysync_keyseq_ptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE + sizeof(uint32_t))) {}

SessionKeySyncPacket::SessionKeySyncPacket(SessionKeySyncPacket &&other) noexcept = default;

uint32_t *SessionKeySyncPacket::getsessidPtr()
{
    return sessionkeysync_sessionid_ptr_;
}

uint32_t *SessionKeySyncPacket::getkeyseqPtr()
{
    return sessionkeysync_keyseq_ptr_;
}

void SessionKeySyncPacket::constructsessionkeysyncpacket(uint32_t session_id, uint32_t keyseq)
{
    uint16_t intvalue = static_cast<uint16_t>(PacketType::SESSIONKEYSYNC);
    std::memcpy(this->getBufferPtr(), &intvalue, sizeof(uint16_t));
    uint16_t length = SESSIONKEYSYNCHEADER;
    std::memcpy(this->getBufferPtr() + sizeof(uint16_t), &length, sizeof(uint16_t));
    this->setBufferSize(BASE_HEADER_SIZE + SESSIONKEYSYNCHEADER);
    *this->sessionkeysync_sessionid_ptr_ = session_id;
    *this->sessionkeysync_keyseq_ptr_ = keyseq;
}
