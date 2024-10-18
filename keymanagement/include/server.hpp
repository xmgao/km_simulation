#ifndef SERVER_HPP
#define SERVER_HPP

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



class Server
{
public:
    Server(int port);
    ~Server();
    void run();
    int epoll_fd_;

private:
    void handleMessage(int fd);

    int createAndBindSocket(int port);
    int port_;
    int listen_fd_;
};

// �¼�����
void addToEpoll(int epoll_fd, int fd);

// ����tcp����
int connectToServer(const std::string &ipAddress, int port);

// �ر�tcp���ӣ��¼�ɾ��
void discon(int fd, int epfd);

std::string uint32ToIpString(uint32_t ipNumeric);

uint32_t IpStringTouint32(const std::string &ipString);

#endif // SERVER_H