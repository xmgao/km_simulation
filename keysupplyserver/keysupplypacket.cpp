#include "keysupplypacket.hpp"
#include <iomanip>

// g++ packetbase.cpp keysupplypacket.cpp -o keysupplypacket

// 构造函数
KeySupplyPacket::KeySupplyPacket()
    : keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLYHEADER) {}

// 带参构造函数
KeySupplyPacket::KeySupplyPacket(PacketBase &&pkt_base)
    : PacketBase(std::move(pkt_base)),
      keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLYHEADER) {}

// 拷贝构造函数
KeySupplyPacket::KeySupplyPacket(const KeySupplyPacket &other)
    : PacketBase(other),
      keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLYHEADER) {}

// 移动构造函数
KeySupplyPacket::KeySupplyPacket(KeySupplyPacket &&other) noexcept = default;

uint8_t *KeySupplyPacket::getKeyBufferPtr()
{
    return keysupply_payloadptr_;
}
uint32_t *KeySupplyPacket::getSeqPtr()
{
    return keysupply_seqptr_;
}


void KeySupplyPacket::ConstrctPacket(uint32_t seq, uint16_t length, uint8_t* keys)
{
    uint16_t type = static_cast<uint16_t>(PacketType::KEYSUPPLY);
    memcpy(this->buffer_, &type, sizeof(uint16_t));
    memcpy(this->buffer_+2, &length, sizeof(uint16_t));
    this->setBufferSize(BASE_HEADER_SIZE + length);
    memcpy(this->keysupply_seqptr_, &seq, sizeof(uint32_t));
    uint16_t key_length = length - KEYSUPPLYHEADER;
    memcpy(this->keysupply_payloadptr_, keys, key_length);
}
