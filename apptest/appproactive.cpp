#include "opensessionpacket.hpp"
#include "keyrequestpacket.hpp"
#include "Encryptor.hpp"
#include <sys/epoll.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h> // for close()
#include <thread>
#include <chrono>

const int LISTEN_PORT = 50000;     // KM监听端口
const int APP_LISTEN_PORT = 50001; // APP监听端口

int connectToServer(const std::string &ipAddress, int port)
{
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // 转换IP地址
    if (inet_pton(AF_INET, ipAddress.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        close(sockfd);
        return -1;
    }

    // 发起连接
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Connection Failed" << std::endl;
        close(sockfd);
        return -1;
    }

    // 返回文件描述符
    return sockfd;
}

std::string uint32ToIpString(uint32_t ipNumeric)
{
    struct in_addr addr;
    addr.s_addr = htonl(ipNumeric); // 将主机字节序转换为网络字节序

    char ipString[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN是足够存储IPv4地址的字符串长度
    if (inet_ntop(AF_INET, &addr, ipString, INET_ADDRSTRLEN) == nullptr)
    {
        // 错误处理
        std::cerr << "Conversion failed." << std::endl;
        return "";
    }
    return std::string(ipString);
}

// 将IP地址字符串转换为uint32_t
uint32_t IpStringTouint32(const std::string &ipString)
{
    struct in_addr addr;
    // 将字符串形式的IP转换为网络字节序的二进制格式
    if (inet_pton(AF_INET, ipString.c_str(), &addr) != 1)
    {
        // 错误处理
        std::cerr << "Conversion failed." << std::endl;
        return 0;
    }
    // 将网络字节序转换为主机字节序
    return ntohl(addr.s_addr);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <proactiveAPP_IP_address> <passiveAPP_IP_address>" << std::endl;
        return 1;
    }

    std::string proactiveAPP_ipAddress = argv[1];
    std::string passiveAPP_ipAddress = argv[2];

    // 连接KM
    int conn_KM_fd = -1;
    const int max_retries = 100; // 设置最大重试次数
    int retries = 0;
    const std::string KM_IP_ADDRESS = "127.0.0.1";

    while (conn_KM_fd <= 0 && retries < max_retries)
    {
        conn_KM_fd = connectToServer(KM_IP_ADDRESS, LISTEN_PORT);
        if (conn_KM_fd <= 0)
        {
            std::cerr << "Failed to connect, retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2)); // 等待2秒
            retries++;
        }
    }
    // 检查连接状态并处理失败情况
    if (conn_KM_fd <= 0)
    {
        std::cerr << "Failed to connect after " << max_retries << " retries." << std::endl;
        // 在这里添加处理逻辑，比如返回特定错误码或抛出异常
        return 0;
    }

    // 如果成功，继续处理连接
    std::cout << "Connected successfully!" << std::endl;
    // 后续的处理逻辑
    // 打开会话
    OpenSessionPacket pkt1;
    uint32_t sourceip = IpStringTouint32(proactiveAPP_ipAddress);
    uint32_t desip = IpStringTouint32(passiveAPP_ipAddress);
    uint32_t session_id = 1;
    pkt1.constructopensessionpacket(sourceip, desip, session_id, true);
    send(conn_KM_fd, pkt1.getBufferPtr(), pkt1.getBufferSize(), 0);

    // 连接被动端APP
    int conn_PassiveAPP_fd = connectToServer(passiveAPP_ipAddress, APP_LISTEN_PORT);
    if (conn_PassiveAPP_fd <= 0)
    {
        std::cerr << "Failed to connect Passive APP" << std::endl;
        return 0;
    }
    uint32_t request_id = 1;
    // 加密数据传输
    while (request_id <= 30)
    {
        std::string getkeyvalue;

        uint32_t request_len = 128;

    label1: // 标签用于重新请求密钥
        // 构造请求密钥包
        KeyRequestPacket pkt2;
        pkt2.constructkeyrequestpacket(session_id, request_id, request_len);
        if (send(conn_KM_fd, pkt2.getBufferPtr(), pkt2.getBufferSize(), 0) == -1)
        {
            perror("send Error");
            close(conn_KM_fd);
            return 0;
        }

        // 处理密钥返回
        PacketBase pkt3;

        // 读取packet header
        ssize_t bytes_read = read(conn_KM_fd, pkt3.getBufferPtr(), BASE_HEADER_SIZE);
        if (bytes_read <= 0)
        {
            perror("read Error");
            close(conn_KM_fd);
            return 0;
        }

        uint16_t value1, length;
        std::memcpy(&value1, pkt3.getBufferPtr(), sizeof(uint16_t));
        std::memcpy(&length, pkt3.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));

        // 读取payload
        bytes_read = read(conn_KM_fd, pkt3.getBufferPtr() + BASE_HEADER_SIZE, length);
        if (bytes_read != length)
        {
            perror("Incomplete payload read");
            close(conn_KM_fd);
            return 0;
        }

        pkt3.setBufferSize(BASE_HEADER_SIZE + length);

        if (value1 == static_cast<uint16_t>(PacketType::KEYRETURN))
        {
            // 带参构造KeyRequestPacket
            KeyRequestPacket pkt4(std::move(pkt3));
            getkeyvalue.resize(request_len);
            std::memcpy(&getkeyvalue[0], pkt4.getKeyBufferPtr(), request_len);
        }
        else
        {
            perror("getkey Error");
            goto label1;
        }

        // 加密数据
        std::string plaintext = "This is the data to be encrypted.";
        std::string ciphertext;
        if (Encryptor::encrypt(plaintext, getkeyvalue, ciphertext))
        {
            std::cout << "Ciphertext: ";
            for (const auto &ch : ciphertext)
            {
                std::cout << std::hex << (int)ch << " ";
            }
            std::cout << std::dec << std::endl; // 恢复十进制格式
        }
        else
        {
            std::cerr << "Encryption failed due to insufficient key length." << std::endl;
        }
        // 发送到终端APP
        if (send(conn_PassiveAPP_fd, &ciphertext[0], ciphertext.length(), 0) == -1)
        {
            perror("send Error");
            close(conn_PassiveAPP_fd);
            return 0;
        }

        // 增加请求ID，防止重复使用相同的密钥
        request_id++;
        // Sleep 或其他逻辑
        usleep(1000000); // 设置合适的时间间隔
    }

    // 关闭会话
    OpenSessionPacket closepkt;
    closepkt.constructclosesessionpacket(sourceip, desip, session_id, true);
    send(conn_KM_fd, closepkt.getBufferPtr(), closepkt.getBufferSize(), 0);
}