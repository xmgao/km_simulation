#include "server.hpp"
#include "sessionmanagement.hpp"
#include "handler.hpp"
#include <thread>

SessionManager globalSessionManager;
KeyManager globalKeyManager;
MessageHandlerRegistry global_registry;

// KM监听外部端口
int LISTEN_PORT = 50000;

// 检查并返回有效的端口号
int verifyPortNumber(const std::string &portStr)
{
    try
    {
        int port = std::stoi(portStr);
        if (port > 0 && port <= 65535)
        {
            return port;
        }
    }
    catch (const std::invalid_argument &e)
    {
        // 忽略，不打印异常，因为后续有错误提示
    }
    std::cerr << "Invalid keysupply port number. Must be between 1 and 65535." << std::endl;
    std::exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <keysupply IP address> <keysupply port>" << std::endl;
        return 1;
    }

    const std::string keysupply_ipAddress = argv[1];
    const int keysupply_port = verifyPortNumber(argv[2]);

    std::cout << "Registering message handlers..." << std::endl;
    global_registry.registerHandler(PacketType::OPENSESSION, handleOpenSessionPacket);
    global_registry.registerHandler(PacketType::KEYREQUEST, handleKeyRequestPacket);
    global_registry.registerHandler(PacketType::KEYSUPPLY, handleKeySupplyPacket);
    global_registry.registerHandler(PacketType::SESSIONKEYSYNC, handleSessionKeySyncPacket);
    global_registry.registerHandler(PacketType::CLOSESESSION, handleCloseSessionPacket);
    global_registry.registerHandler(PacketType::MSG_TYPE_UNKNOWN, handleUnknownPacket);

    Server server1(LISTEN_PORT);
    std::cout << "Running globalKeyManager..." << std::endl;
    globalKeyManager.run(server1, keysupply_ipAddress, keysupply_port);

    std::cout << "Starting server on port " << LISTEN_PORT << "..." << std::endl;
    server1.run();

    return 0;
}