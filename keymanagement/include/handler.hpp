#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <map>

// ����ָ�����Ͷ��壨ÿ�������������ļ���������������Ϣ��
typedef void (*MessageHandler)(int, PacketBase &);

// ע��͹���ص���������
class MessageHandlerRegistry
{
public:
    void registerHandler(PacketType type, MessageHandler handler)
    {
        handlers[type] = handler;
    }

    MessageHandler getHandler(PacketType type) const
    {
        auto it = handlers.find(type);
        if (it != handlers.end())
        {
            return it->second;
        }
        return nullptr;
    }

private:
    std::map<PacketType, MessageHandler> handlers;
};

// ����OPENSESSION
void handleOpenSessionPacket(int fd, PacketBase &pkt1);

// ����KEYREQUEST
void handleKeyRequestPacket(int fd, PacketBase &pkt1);

// ����CLOSESESSION
void handleCloseSessionPacket(int fd, PacketBase &pkt1);

// ����KEYSUPPLY
void handleKeySupplyPacket(int fd, PacketBase &pkt1);

// ����SESSIONKEYSYNC
void handleSessionKeySyncPacket(int fd, PacketBase &pkt1);

// ����UNKOWN_TYPE
void handleUnknownPacket(int fd, PacketBase &pkt);
// ģ�����Ϣ�н���������
PacketType parsePacketType(uint16_t type);

#endif
