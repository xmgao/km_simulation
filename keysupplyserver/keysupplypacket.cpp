#include "keysupplypacket.hpp"
#include <iomanip>
#include <netinet/in.h>

// g++ packetbase.cpp keysupplypacket.cpp -o keysupplypacket

// ??????
KeySupplyPacket::KeySupplyPacket()
    : keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLYHEADER) {}

// ???ι?????
KeySupplyPacket::KeySupplyPacket(PacketBase &&pkt_base)
    : PacketBase(std::move(pkt_base)),
      keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLYHEADER) {}

// ??????????
KeySupplyPacket::KeySupplyPacket(const KeySupplyPacket &other)
    : PacketBase(other),
      keysupply_seqptr_(reinterpret_cast<uint32_t *>(buffer_ + BASE_HEADER_SIZE)),
      keysupply_payloadptr_(buffer_ + BASE_HEADER_SIZE + KEYSUPPLYHEADER) {}

// ?????????
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
    uint16_t type = static_cast<uint16_t>(ValueType::KEYSUPPLY);
    memcpy(this->buffer_, &type, sizeof(uint16_t));
    memcpy(this->buffer_+2, &length, sizeof(uint16_t));
    memcpy(this->keysupply_seqptr_, &seq, sizeof(uint32_t));
    uint16_t key_length = length - KEYSUPPLYHEADER;
    memcpy(this->keysupply_payloadptr_, keys, key_length);
}

char trans_single(int value)
{
	if (value < 10)
	{
		return value + '0';
	}
	else if (value == 10)
	{
        return 'a';
    }
    else if (value == 11)
    {
        return 'b';
    }
    else if (value == 12)
    {
        return 'c';
    }
    else if (value == 13)
    {
        return 'd';
    }
    else if (value == 14)
    {
        return 'e';
    }
    else
    {
        return 'f';
    }
}


void KeySupplyPacket::PrintKeys(const char * file_name, const char * operate)
{
    uint16_t length;
    memcpy(&length, this->buffer_+2, sizeof(uint16_t));
    uint16_t key_length = length - KEYSUPPLYHEADER;
    uint32_t seq;
    memcpy(&seq, this->keysupply_seqptr_, sizeof(uint32_t));
    char keys[2*key_length+1];
    uint8_t value, high, low;
	char c;
    keys[2*key_length] = '\0';
    for (int i = 0; i < key_length; i++)
	{
		value = this->keysupply_payloadptr_[i];
		low = value % 16;
		high = value / 16;
		printf("value: %u, high: %u, low: %u\n", value, high, low);
		c = trans_single(high);
		keys[2*i] = c;
		c = trans_single(low);
		keys[2*i+1] = c;
	}
    FILE* fp;
    fp = fopen(file_name, operate);
    fwrite(keys, sizeof(char), 2*key_length, fp);
    fputs("\n", fp);
    fputc(seq+'0', fp);
    fputs("\n", fp);
    fclose(fp);
}


void KeySupplyPacket::PackTcpPacket(char* buf)
{
    // 将包头和密钥文件拼接成一个字符串
    // uint16_t type, length;
    // uint32_t seq;
    // memcpy(&type, this->buffer_, sizeof(uint16_t));
    // memcpy(&length,this->buffer_+2,  sizeof(uint16_t));
    // memcpy(&seq, this->keysupply_seqptr_, sizeof(uint32_t));

	// type = htons(type);
	// length = htons(length);
	// seq = htonl(seq);

    memcpy(buf, this->buffer_, 2);
    memcpy(buf + 2, this->buffer_+2, 2);
	memcpy(buf + 4, this->keysupply_seqptr_, 4);
	memcpy(buf + 8, this->keysupply_payloadptr_, MAX_DATA_SIZE);
}

void KeySupplyPacket::UnpackTcpPacket(char* buf)
{
	memcpy(this->buffer_, buf, 2);
	memcpy(this->buffer_+2, buf+2, 2);
	memcpy(this->keysupply_seqptr_, buf+4, 4);
	memcpy(this->keysupply_payloadptr_, buf+8, MAX_DATA_SIZE);

	// packet->type = ntohs(packet->type);
	// packet->length = ntohs(packet->length);
	// packet->value.seq = ntohl(packet->value.seq);
	// printf("type: %u, length: %u, seq: %u\n", packet->type, packet->length, packet->value.seq);
}