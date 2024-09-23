#include "packetbase.hpp"

PacketBase::PacketBase()
    : buffer_size_(BASE_HEADER_SIZE)
{
    buffer_ = new uint8_t[MAX_BUFFER_SIZE]();
}

PacketBase::PacketBase(const PacketBase &other)
    : buffer_size_(other.buffer_size_),
      buffer_(other.buffer_size_ ? new uint8_t[other.buffer_size_]() : nullptr)
{
    if (buffer_)
        std::memcpy(buffer_, other.buffer_, buffer_size_);
}

PacketBase::PacketBase(PacketBase &&other) noexcept
    : buffer_(std::move(other.buffer_)), buffer_size_(other.buffer_size_)
{
    other.buffer_ = nullptr;
    other.buffer_size_ = 0;
}

PacketBase::~PacketBase() = default;

uint8_t *PacketBase::getBufferPtr()
{
    return buffer_;
}

size_t PacketBase::getBufferSize() const
{
    return buffer_size_;
}

void PacketBase::setBufferSize(size_t size)
{
    buffer_size_ = size;
}
