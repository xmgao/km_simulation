#include "packet/packets.hpp"
#include "sessionmanagement.hpp"
#include "debuglevel.hpp"
#include "keymanagement.hpp"
#include "handler.hpp"
#include "server.hpp"

extern SessionManager globalSessionManager;
extern KeyManager globalKeyManager;

// 辅助函数：从套接字读取指定大小的数据
ssize_t readFromSocket(int fd, void *buffer, size_t length)
{
    size_t totalRead = 0;
    while (totalRead < length)
    {
        ssize_t bytesRead = read(fd, static_cast<char *>(buffer) + totalRead, length - totalRead);
        if (bytesRead <= 0)
        {
            // 错误或连接关闭
            return bytesRead;
        }
        totalRead += bytesRead;
    }
    return totalRead;
}

// 辅助函数：发送确认消息
void sendConfirmMessage(int fd, ErrorCode errorCode)
{
    ConfirmMessagePacket pktConfirm;
    pktConfirm.constructConfirmMessagePacket(static_cast<uint32_t>(errorCode));
    send(fd, pktConfirm.getBufferPtr(), pktConfirm.getBufferSize(), 0);
}

// 处理OPENSESSION
void handleOpenSessionPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));

    if (readFromSocket(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length) <= 0)
    {
        std::cerr << "Failed to read OPENSESSION payload." << std::endl;
        return;
    }

    pkt1.setBufferSize(BASE_HEADER_SIZE + length);
    OpenSessionPacket pkt2(std::move(pkt1));

    uint32_t sourceip = *pkt2.getsourcePtr();
    uint32_t desip = *pkt2.getdesPtr();
    uint32_t session_id = *pkt2.getsessidPtr();
    bool is_inbound = *pkt2.getinboundPtr();

    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received OPENSESSION packet:"
                  << " source_ip: " << uint32ToIpString(sourceip)
                  << " dest_ip: " << uint32ToIpString(desip)
                  << " session_id: " << session_id
                  << " is_inbound: " << is_inbound
                  << std::endl;
    }

    bool result = globalSessionManager.addSession(sourceip, desip, session_id, is_inbound);
    if (!is_inbound)
    {
        sendConfirmMessage(fd, result ? ErrorCode::SUCCESS : ErrorCode::OPENSESSIONERROR);
    }
}

// 处理KEYREQUEST
void handleKeyRequestPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));

    if (readFromSocket(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length) <= 0)
    {
        std::cerr << "Failed to read KEYREQUEST payload." << std::endl;
        return;
    }

    pkt1.setBufferSize(BASE_HEADER_SIZE + length);
    KeyRequestPacket pkt2(std::move(pkt1));

    uint32_t session_id = *pkt2.getsessidPtr();
    uint32_t request_id = *pkt2.getreqidPtr();
    uint16_t request_len = *pkt2.getreqlenPtr();

    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received KEYREQUEST packet:"
                  << " session_id: " << session_id
                  << " request_id: " << request_id
                  << " request_len: " << request_len
                  << std::endl;
    }

    std::string getkeyvalue = globalSessionManager.getSessionKey(session_id, request_id, request_len);

    if (getkeyvalue.empty())
    {
        sendConfirmMessage(fd, ErrorCode::GETKEYERROR);
        std::cerr << "Failed to get key!" << std::endl;
        return;
    }

    // 返回密钥
    KeyRequestPacket pkt3;
    pkt3.constructkeyreturnpacket(session_id, request_id, request_len, getkeyvalue);
    send(fd, pkt3.getBufferPtr(), pkt3.getBufferSize(), 0);
    if (DEBUG_LEVEL == 1)
    {
        std::cout << "send KEYRETURN packet:"
                  << " session_id: " << session_id
                  << " request_id: " << request_id
                  << " request_len: " << request_len
                  << std::endl;
    }
}

// 处理CLOSESESSION
void handleCloseSessionPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));

    if (readFromSocket(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length) <= 0)
    {
        std::cerr << "Failed to read CLOSESESSION payload." << std::endl;
        return;
    }

    pkt1.setBufferSize(BASE_HEADER_SIZE + length);

    OpenSessionPacket pkt2(std::move(pkt1));
    uint32_t sourceip = *pkt2.getsourcePtr();
    uint32_t desip = *pkt2.getdesPtr();
    uint32_t session_id = *pkt2.getsessidPtr();
    bool is_inbound = *pkt2.getinboundPtr();

    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received CLOSESESSION packet:"
                  << " source_ip: " << uint32ToIpString(sourceip)
                  << " dest_ip: " << uint32ToIpString(desip)
                  << " session_id: " << session_id
                  << " is_inbound: " << is_inbound
                  << std::endl;
    }

    globalSessionManager.closeSession(session_id);
    close(fd);
}

// 处理KEYSUPPLY
void handleKeySupplyPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));

    if (readFromSocket(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length) <= 0)
    {
        std::cerr << "Failed to read KEYSUPPLY payload." << std::endl;
        return;
    }

    pkt1.setBufferSize(BASE_HEADER_SIZE + length);
    KeySupplyPacket pkt2(std::move(pkt1));
    uint32_t seqnumber = *pkt2.getSeqPtr();

    if (DEBUG_LEVEL <= 0)
    {
        std::cout << "Received KEYSUPPLY packet:"
                  << " seqnumber: " << seqnumber
                  << std::endl;
    }

    globalKeyManager.addKey(seqnumber, pkt2.getKeyBufferPtr(), length - KEYSUPPLY_HEADER_SIZE);
}

// 处理SESSIONKEYSYNC
void handleSessionKeySyncPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));

    if (readFromSocket(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length) <= 0)
    {
        std::cerr << "Failed to read SESSIONKEYSYNC payload." << std::endl;
        return;
    }

    pkt1.setBufferSize(BASE_HEADER_SIZE + length);
    SessionKeySyncPacket pkt2(std::move(pkt1));

    uint32_t session_id = *pkt2.getsessidPtr();
    uint32_t keyseqnumber = *pkt2.getkeyseqPtr();

    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received SESSIONKEYSYNC packet:"
                  << " session_id: " << session_id
                  << " keyseqnumber: " << keyseqnumber
                  << std::endl;
    }

    if (globalSessionManager.addPassiveKey(session_id, keyseqnumber))
    {
        std::cout << "add passive session key success." << std::endl;
    }
    else
    {
        std::cerr << "Unable to add passive session key." << std::endl;
    }
}

// 处理UNKOWN_TYPE，假设Type错误，Length正确
void handleUnknownPacket(int fd, PacketBase &pkt)
{
    std::cout << "Received UNKNOWN_TYPE packet." << std::endl;
    // 读取并丢弃未知消息
    char buffer[MAX_BUFFER_SIZE];
    while (read(fd, buffer, sizeof(buffer)) > 0)
    {
        // Continue reading until the buffer is empty
    }

    // 简单回复
    sendConfirmMessage(fd, ErrorCode::UNKONWNMESSAGE);
    close(fd);
}

// 模拟从消息中解析出类型
PacketType parsePacketType(uint16_t type)
{
    switch (type)
    {
    case static_cast<uint16_t>(PacketType::KEYSUPPLY):
        return PacketType::KEYSUPPLY;
    case static_cast<uint16_t>(PacketType::KEYREQUEST):
        return PacketType::KEYREQUEST;
    case static_cast<uint16_t>(PacketType::OPENSESSION):
        return PacketType::OPENSESSION;
    case static_cast<uint16_t>(PacketType::CLOSESESSION):
        return PacketType::CLOSESESSION;
    case static_cast<uint16_t>(PacketType::SESSIONKEYSYNC):
        return PacketType::SESSIONKEYSYNC;
    default:
        return PacketType::MSG_TYPE_UNKNOWN;
    }
}