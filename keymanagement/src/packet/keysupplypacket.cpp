#include "packet/keysupplypacket.hpp"


// 构造函数
KeySupplyPacket::KeySupplyPacket()
    : keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLY_HEADER_SIZE) {}

// 带参构造函数
KeySupplyPacket::KeySupplyPacket(PacketBase &&pkt_base)
    : PacketBase(std::move(pkt_base)),
      keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLY_HEADER_SIZE) {}

// 拷贝构造函数
KeySupplyPacket::KeySupplyPacket(const KeySupplyPacket &other)
    : PacketBase(other),
      keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLY_HEADER_SIZE) {}

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

void KeySupplyPacket::ConstrctKeySupplyPacket(uint32_t seq, const std::string &getkeyvalue)
{
    uint16_t type = static_cast<uint16_t>(PacketType::KEYSUPPLY);
    std::memcpy(this->buffer_, &type, sizeof(uint16_t));
    uint16_t length = KEYSUPPLY_HEADER_SIZE + getkeyvalue.length();
    std::memcpy(this->buffer_ + sizeof(uint16_t), &length, sizeof(uint16_t));
    this->setBufferSize(BASE_HEADER_SIZE + length);
    std::memcpy(this->keysupply_seqptr_, &seq, sizeof(uint32_t));
    std::memcpy(this->keysupply_payloadptr_, &getkeyvalue[0], getkeyvalue.length());
}