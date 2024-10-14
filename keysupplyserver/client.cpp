#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fstream>
#include <cmath>
#include "packetbase.hpp"
#include "keysupplypacket.hpp"

using namespace std;

#define MAXSIZE 10 // byte
#define IP_ADDR "127.0.0.1"
#define IP_PORT 50003

int i_sockfd = -1;

void SigCatch(int sigNum) // 信号捕捉函数(捕获Ctrl+C)
{
	if (i_sockfd != -1)
	{
		close(i_sockfd);
	}
	printf("Bye~! Will Exit...\n");
	exit(0);
}

int main()
{
	struct sockaddr_in st_clnsock;
	// char msg[1024];
	signal(SIGINT, SigCatch); // 注册信号捕获函数

	if ((i_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // 建立套接字
	{
		printf("socket Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	memset(&st_clnsock, 0, sizeof(st_clnsock));
	st_clnsock.sin_family = AF_INET; // IPv4协议
	// IP地址转换(直接可以从物理字节序的点分十进制 转换成网络字节序)
	if (inet_pton(AF_INET, IP_ADDR, &st_clnsock.sin_addr) <= 0)
	{
		printf("inet_pton Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}
	st_clnsock.sin_port = htons(IP_PORT); // 端口转换(物理字节序到网络字节序)

	if (connect(i_sockfd, (struct sockaddr *)&st_clnsock, sizeof(st_clnsock)) < 0) // 主动向设置的IP和端口号的服务端发出连接
	{
		printf("connect Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	printf("======connect to server, send request and wait for data======\n");
	while (1) // 循环, 接受服务端返回的数据
	{
		KeySupplyPacket pkt1;
		// 读取packet header
		ssize_t bytes_read = read(i_sockfd, pkt1.getBufferPtr(), BASE_HEADER_SIZE);
		if (bytes_read < 0)
		{
			perror("read Error");
			close(i_sockfd);
			exit(0);
		}
		else if (bytes_read == 0)
		{
			// 服务端关闭了连接
			std::cout << "Connection closed on socket: " << i_sockfd << std::endl;
			close(i_sockfd);
		}

		uint16_t length;
		std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));
		// 读取payload
		read(i_sockfd, pkt1.getBufferPtr() + BASE_HEADER_SIZE, length);
		pkt1.setBufferSize(BASE_HEADER_SIZE + length);
		uint32_t seqnumber = *pkt1.getSeqPtr();
		{
			std::cout << "Received KEYSUPPLY packet: "
					  << "seqnumber: " << seqnumber
					  << std::endl;
		}

	}
	return 0;
}