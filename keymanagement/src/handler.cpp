#include "packetbase.hpp"
#include "keyrequestpacket.hpp"
#include "opensessionpacket.hpp"
#include "keysupplypacket.hpp"
#include "sessionmanagement.hpp"
#include "debuglevel.hpp"
#include "keymanagement.hpp"
#include "handler.hpp"
#include "server.hpp"

extern SessionManager globalSessionManager;
extern KeyManager globalKeyManager;

// 处理OPENSESSION
void handleOpenSessionPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));
    // 读取payload
    read(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length); // read需要处理，while循环读入
    pkt1.setBufferSize(BASE_HEADER_SIZE + length);

    // 带参构造OPENSESSIONPacket
    OpenSessionPacket pkt2(std::move(pkt1));
    uint32_t sourceip = *pkt2.getsourcePtr();
    uint32_t desip = *pkt2.getdesPtr();
    uint32_t session_id = *pkt2.getsessidPtr();
    bool is_outbound = *pkt2.getoutboundPtr();
    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received OPENSESSION packet: "
                  << " source_ip: " << uint32ToIpString(sourceip)
                  << " dest_ip: " << uint32ToIpString(desip)
                  << " session_id: " << session_id
                  << " is_outbound: " << is_outbound
                  << std::endl;
    }
    if (!globalSessionManager.addSession(sourceip, desip, session_id, is_outbound))
    {
        ErrorMessagePacket pkt3;
        pkt3.constructErrorMessagePacket(static_cast<uint32_t>(ErrorType::OPENSESSIONERROR));
        send(fd, pkt3.getBufferPtr(), pkt3.getBufferSize(), 0);
    }
}

// 处理KEYREQUEST
void handleKeyRequestPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));
    // 读取payload
    read(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length);
    pkt1.setBufferSize(BASE_HEADER_SIZE + length);

    // 带参构造KeyRequestPacket
    KeyRequestPacket pkt2(std::move(pkt1));
    uint32_t session_id = *pkt2.getsessidPtr();
    uint32_t request_id = *pkt2.getreqidPtr();
    uint16_t request_len = *pkt2.getreqlenPtr();
    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received KEYREQUEST packet: "
                  << " session_id: " << session_id
                  << " request_id: " << request_id
                  << " request_len: " << request_len
                  << std::endl;
    }
    std::string getkeyvalue = globalSessionManager.getKey(session_id, request_id, request_len);
    if (getkeyvalue == "")
    {
        ErrorMessagePacket pkt3;
        pkt3.constructErrorMessagePacket(static_cast<uint32_t>(ErrorType::GETKEYERROR));
        send(fd, pkt3.getBufferPtr(), pkt3.getBufferSize(), 0);
        std::cerr << "Failed to get key! " << std::endl;
        return;
    }
    // 返回密钥
    KeyRequestPacket pkt3;
    pkt3.constructkeyreturnpacket(session_id, request_id, request_len, getkeyvalue);
    send(fd, pkt3.getBufferPtr(), pkt3.getBufferSize(), 0);
}

// 处理CLOSESESSION
void handleCloseSessionPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));
    // 读取payload
    read(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length);
    pkt1.setBufferSize(BASE_HEADER_SIZE + length);
    // 带参构造CloseSessionPacket
    OpenSessionPacket pkt2(std::move(pkt1));
    uint32_t sourceip = *pkt2.getsourcePtr();
    uint32_t desip = *pkt2.getdesPtr();
    uint32_t session_id = *pkt2.getsessidPtr();
    bool is_outbound = *pkt2.getoutboundPtr();
    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received CLOSESESSION packet: "
                  << " source_ip: " << uint32ToIpString(sourceip)
                  << " dest_ip: " << uint32ToIpString(desip)
                  << " session_id: " << session_id
                  << " is_outbound: " << is_outbound
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
    // 读取payload
    read(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length);
    pkt1.setBufferSize(BASE_HEADER_SIZE + length);
    // 带参构造KeySupplyPacket
    KeySupplyPacket pkt2(std::move(pkt1));
    uint32_t seqnumber = *pkt2.getSeqPtr();
    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received KEYSUPPLY packet: "
                  << "seqnumber: " << seqnumber
                  << std::endl;
    }
    // 添加密钥
    globalKeyManager.addKey(*pkt2.getSeqPtr(), pkt2.getKeyBufferPtr(), length - KEYSUPPLYHEADER);
}

// 处理SESSIONKEYSYNC
void handleSessionKeySyncPacket(int fd, PacketBase &pkt1)
{
    uint16_t length;
    std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));
    // 读取payload
    read(fd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length);
    pkt1.setBufferSize(BASE_HEADER_SIZE + length);
    // 主动端向被动端发起的密钥同步操作
    SessionKeySyncPacket pkt2(std::move(pkt1));
    uint32_t session_id = *pkt2.getsessidPtr();
    uint32_t keyseqnumber = *pkt2.getkeyseqPtr();
    if (DEBUG_LEVEL == 1)
    {
        std::cout << "Received SESSIONKEYSYNC packet: "
                  << "session_id: " << session_id
                  << "Packet2 keyseqnumber: " << keyseqnumber
                  << std::endl;
    }
    if (!globalSessionManager.addPassiveKey(session_id, keyseqnumber))
    {
        std::cerr << "unable to addpassiveSessionkey " << std::endl;
    }
}

// 处理UNKOWN_TYPE，假设Type错误，Length正确
void handleUnknownPacket(int fd, PacketBase &pkt)
{
    std::cout << "Received UNKOWN_TYPE!" << std::endl;
    // 读取并丢弃未知消息
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        // 继续读取，直到缓冲区为空
    }
    // 简单回复
    ErrorMessagePacket pkt3;
    pkt3.constructErrorMessagePacket(static_cast<uint32_t>(ErrorType::UNKONWNMESSAGE));
    send(fd, pkt3.getBufferPtr(), pkt3.getBufferSize(), 0);
    close(fd);
}

// 模拟从消息中解析出类型
PacketType parsePacketType(uint16_t type)
{
    if (type == static_cast<uint16_t>(PacketType::KEYSUPPLY))
        return PacketType::KEYSUPPLY;
    if (type == static_cast<uint16_t>(PacketType::KEYREQUEST))
        return PacketType::KEYREQUEST;
    if (type == static_cast<uint16_t>(PacketType::OPENSESSION))
        return PacketType::OPENSESSION;
    if (type == static_cast<uint16_t>(PacketType::CLOSESESSION))
        return PacketType::CLOSESESSION;
    if (type == static_cast<uint16_t>(PacketType::SESSIONKEYSYNC))
        return PacketType::SESSIONKEYSYNC;
    return PacketType::MSG_TYPE_UNKNOWN;
}
