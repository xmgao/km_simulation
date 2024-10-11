
#include "server.hpp"
#include "sessionmanagement.hpp"
#include "handler.hpp"
#include <thread>
#include <cstdlib> // for std::atoi

// 全局sessionManager实例
SessionManager globalSessionManager;

// 全局KeyManager实例
KeyManager globalKeyManager;

// 创建并注册消息处理器
MessageHandlerRegistry global_registry;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <keysupply IP address> <keysuply_port>" << std::endl;
        return 1;
    }

    std::string keysupply_ipAddress = argv[1];
    int keysuply_port = std::atoi(argv[2]);

    // 端口号范围验证
    if (keysuply_port <= 0 || keysuply_port > 65535)
    {
        std::cerr << "Invalid keysuply_port number. Must be between 1 and 65535." << std::endl;
        return 1;
    }

    std::cout << "begin register!" << std::endl;
    global_registry.registerHandler(PacketType::OPENSESSION, handleOpenSessionPacket);
    global_registry.registerHandler(PacketType::KEYREQUEST, handleKeyRequestPacket);
    global_registry.registerHandler(PacketType::KEYSUPPLY, handleKeySupplyPacket);
    global_registry.registerHandler(PacketType::SESSIONKEYSYNC, handleSessionKeySyncPacket);
    global_registry.registerHandler(PacketType::CLOSESESSION, handleCloseSessionPacket);
    global_registry.registerHandler(PacketType::MSG_TYPE_UNKNOWN, handleUnknownPacket);


    Server server1(LISTEN_PORT);
    std::cout << "begin globalKeyManager!" << std::endl;
    globalKeyManager.run(server1, keysupply_ipAddress, keysuply_port);
    std::cout << "begin server1.run!" << std::endl;
    server1.run();

    return 0;
}