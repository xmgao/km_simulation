#ifndef PACKET_BASE_HPP
#define PACKET_BASE_HPP

#include <iostream>
#include <cstring>
#include <cstdint>
#include <memory>

// �������ݳ���
#define MAX_BUFFER_SIZE 1024
#define BASE_HEADER_SIZE 4

enum class PacketType : uint16_t
{
    KEYSUPPLY = 0,
    KEYREQUEST,
    KEYRETURN,
    OPENSESSION,
    CLOSESESSION,
    SESSIONKEYSYNC,
    MSG_TYPE_UNKNOWN,
};

// ���� PacketBase ���Ѿ����岢���� buffer_ ��Ա
class PacketBase
{
protected:
    uint8_t *buffer_;
    size_t buffer_size_;

public:
    PacketBase();
    PacketBase(const PacketBase &other);
    PacketBase(PacketBase &&other) noexcept;
    virtual ~PacketBase();

    uint8_t *getBufferPtr();
    size_t getBufferSize() const;
    void setBufferSize(size_t size);
};

using PacketBasePtr = std::shared_ptr<PacketBase>;

#endif