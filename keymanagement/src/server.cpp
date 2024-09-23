
#include "packetbase.hpp"
#include "keyrequestpacket.hpp"
#include "opensessionpacket.hpp"
#include "sessionmanagement.hpp"
#include "debuglevel.hpp"
#include "handler.hpp"
#include "server.hpp"

extern MessageHandlerRegistry global_registry;

Server::Server(int port)
    : port_(port)
{
    listen_fd_ = createAndBindSocket(port_);
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    addToEpoll(epoll_fd_, listen_fd_);
}

Server::~Server()
{
    close(listen_fd_);
}

int Server::createAndBindSocket(int port)
{
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sock, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return listen_sock;
}

void addToEpoll(int epoll_fd, int fd)
{
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

void Server::handleMessage(int fd)
{

    // ����һ�� PacketBase ����
    PacketBase pkt1;

    // ��ȡpacket header
    ssize_t bytes_read = read(fd, pkt1.getBufferPtr(), BASE_HEADER_SIZE);
    if (bytes_read < 0)
    {
        perror("read Error");
        close(fd);
    }
    else if (bytes_read == 0)
    {
        // �ͻ��˹ر�������
        std::cout << "Connection closed on socket: " << fd << std::endl;
        close(fd);
    }
    else
    {
        uint16_t typevalue;
        std::memcpy(&typevalue, pkt1.getBufferPtr(), sizeof(uint16_t));
        // ������Ϣ����
        std::cout << "Server Received message! " << std::endl;
        PacketType type = parsePacketType(typevalue);
        // ��ȡ��������Ӧ�Ļص�����
        MessageHandler handler = global_registry.getHandler(type);
        if (handler)
        {
            handler(fd, pkt1);
        }
        else
        {
            printf("No handler registered for message type %d\n", static_cast<uint16_t>(type));
            close(fd);
        }
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
            perror("epoll_wait");
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
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                addToEpoll(epoll_fd_, conn_fd);
            }
            else
            {
                handleMessage(events[i].data.fd);
            }
        }
    }

    close(epoll_fd_);
}

int connectToServer(const std::string &ipAddress, int port)
{
    // �����׽���
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // ���÷�������ַ
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // ת��IP��ַ
    if (inet_pton(AF_INET, ipAddress.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        close(sockfd);
        return -1;
    }
    // ��������
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Connection Failed" << std::endl;
        close(sockfd);
        return -1;
    }
    // �����ļ�������
    return sockfd;
}

void discon(int fd, int epfd)
{
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    if (ret < 0)
    {
        perror("EPOLL_CTL_DEL error...\n");
    }
    close(fd);
}

std::string uint32ToIpString(uint32_t ipNumeric)
{
    struct in_addr addr;
    addr.s_addr = htonl(ipNumeric); // �������ֽ���ת��Ϊ�����ֽ���

    char ipString[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN���㹻�洢IPv4��ַ���ַ�������
    if (inet_ntop(AF_INET, &addr, ipString, INET_ADDRSTRLEN) == nullptr)
    {
        // ������
        std::cerr << "Conversion failed." << std::endl;
        return "";
    }
    return std::string(ipString);
}

// ��IP��ַ�ַ���ת��Ϊuint32_t
uint32_t IpStringTouint32(const std::string &ipString)
{
    struct in_addr addr;
    // ���ַ�����ʽ��IPת��Ϊ�����ֽ���Ķ����Ƹ�ʽ
    if (inet_pton(AF_INET, ipString.c_str(), &addr) != 1)
    {
        // ������
        std::cerr << "Conversion failed." << std::endl;
        return 0;
    }
    // �������ֽ���ת��Ϊ�����ֽ���
    return ntohl(addr.s_addr);
}