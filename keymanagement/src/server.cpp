#include "packet/packets.hpp"
#include "sessionmanagement.hpp"
#include "debuglevel.hpp"
#include "handler.hpp"
#include "server.hpp"

// 声明外部的处理器注册表
extern MessageHandlerRegistry global_registry;

namespace
{
    void checkError(int result, const std::string &errorMsg)
    {
        if (result < 0)
        {
            throw std::runtime_error(errorMsg + ": " + std::strerror(errno));
        }
    }
}

// Server class implementation
Server::Server(int port)
    : epoll_fd_(0), port_(port), listen_fd_(0)
{
    listen_fd_ = createAndBindSocket(port_);
    epoll_fd_ = epoll_create1(0);
    checkError(epoll_fd_, "epoll_create1 failed");

    // Add listen socket to epoll
    addToEpoll(epoll_fd_, listen_fd_);
}

Server::~Server()
{
    close(listen_fd_);
    close(epoll_fd_);
}

int Server::createAndBindSocket(int port)
{
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    checkError(listen_sock, "Failed to create socket");

    int opt = 1;
    checkError(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), "setsockopt failed");

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(static_cast<uint16_t>(port));

    checkError(bind(listen_sock, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)), "bind failed");
    checkError(listen(listen_sock, SOMAXCONN), "listen failed");

    return listen_sock;
}

void addToEpoll(int epoll_fd, int fd)
{
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    checkError(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev), "epoll_ctl failed");
}

void Server::handleMessage(int fd)
{
    try
    {
        PacketBase pkt1;
        ssize_t bytes_read = read(fd, pkt1.getBufferPtr(), BASE_HEADER_SIZE);
        if (bytes_read < 0)
        {
            perror("read error");
            discon(fd, epoll_fd_);
            return;
        }

        if (bytes_read == 0)
        {
            std::cerr << "Connection closed on socket: " << fd << std::endl;
            discon(fd, epoll_fd_);
            return;
        }

        uint16_t typevalue;
        std::memcpy(&typevalue, pkt1.getBufferPtr(), sizeof(uint16_t));
        PacketType type = parsePacketType(typevalue);

        //std::cout << "Server received message of type: " << static_cast<uint16_t>(type) << std::endl;

        // Get and call the appropriate handler
        MessageHandler handler = global_registry.getHandler(type);
        if (handler)
        {
            handler(fd, pkt1); // Use handler to process the packet
        }
        else
        {
            std::cerr << "No handler registered for message type " << static_cast<uint16_t>(type) << std::endl;
            discon(fd, epoll_fd_);
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error handling message on socket " << fd << ": " << ex.what() << std::endl;
        discon(fd, epoll_fd_);
    }
}

void Server::run()
{
    std::vector<epoll_event> events(128);
    while (true)
    {
        int nfds = epoll_wait(epoll_fd_, events.data(), events.size(), -1);
        if (nfds < 0)
        {
            perror("epoll_wait failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == listen_fd_)
            {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int conn_fd = accept(listen_fd_, (struct sockaddr *)&client_addr, &client_len);
                if (conn_fd < 0)
                {
                    perror("accept failed");
                    continue; // Don't exit, just continue
                }
                addToEpoll(epoll_fd_, conn_fd);
            }
            else
            {
                handleMessage(events[i].data.fd);
            }
        }
    }
}

int connectToServer(const std::string &ipAddress, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating socket: " << std::strerror(errno) << std::endl;
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAddress.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Connection Failed: " << std::strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void discon(int fd, int epfd)
{
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) < 0)
    {
        perror("EPOLL_CTL_DEL error");
    }
    close(fd);
}

std::string uint32ToIpString(uint32_t ipNumeric)
{
    in_addr addr;
    addr.s_addr = htonl(ipNumeric);

    char ipString[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, ipString, INET_ADDRSTRLEN) == nullptr)
    {
        std::cerr << "Conversion failed." << std::endl;
        return "";
    }
    return std::string(ipString);
}

uint32_t IpStringTouint32(const std::string &ipString)
{
    in_addr addr;
    if (inet_pton(AF_INET, ipString.c_str(), &addr) != 1)
    {
        std::cerr << "Conversion failed." << std::endl;
        return 0;
    }
    return ntohl(addr.s_addr);
}