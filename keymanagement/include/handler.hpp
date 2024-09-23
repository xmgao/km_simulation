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

// 函数指针类型定义（每个处理函数接受文件描述符并处理消息）
typedef void (*MessageHandler)(int, PacketBase &);

// 注册和管理回调函数的类
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

// 处理OPENSESSION
void handleOpenSessionPacket(int fd, PacketBase &pkt1);

// 处理KEYREQUEST
void handleKeyRequestPacket(int fd, PacketBase &pkt1);

// 处理CLOSESESSION
void handleCloseSessionPacket(int fd, PacketBase &pkt1);

// 处理KEYSUPPLY
void handleKeySupplyPacket(int fd, PacketBase &pkt1);

// 处理SESSIONKEYSYNC
void handleSessionKeySyncPacket(int fd, PacketBase &pkt1);

// 处理UNKOWN_TYPE
void handleUnknownPacket(int fd, PacketBase &pkt);
// 模拟从消息中解析出类型
PacketType parsePacketType(uint16_t type);

#endif
