#ifndef PACKET_BASE_HPP
#define PACKET_BASE_HPP

#include <iostream>
#include <cstring>
#include <cstdint>
#include <memory>

// ???????????
#define MAX_BUFFER_SIZE 1024
#define BASE_HEADER_SIZE 4
constexpr size_t MAX_DATA_SIZE = 512;

// ????Value?????????????????????????????§Ó??????? value ?????
enum class ValueType : uint16_t
{
    KEYSUPPLY = 0,
    KEYREQUEST,
    OPENSESSION,
    CLOSESESSION
};

// ???? PacketBase ????????á³???? buffer_ ???
class PacketBase
{
protected:
    uint8_t *buffer_;
    size_t buffer_size_;

public:
    PacketBase();
    PacketBase(const PacketBase& other);
    PacketBase(PacketBase&& other) noexcept;
    virtual ~PacketBase();

    uint8_t* getBufferPtr();
    size_t getBufferSize() const;
    void setBufferSize(size_t size);

};

using PacketBasePtr = std::shared_ptr<PacketBase>;

#endif