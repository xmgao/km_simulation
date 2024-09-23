#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fstream>
#include <cmath>
#include "packetbase.hpp"
#include "keysupplypacket.hpp"

using namespace std;

#define IP_PORT 50003
#define key_length 512

bool create_qpacket(KeySupplyPacket &packet, uint32_t seq)
{ // read keys from feature files, when failing to catch,return false;
	uint8_t keys[key_length];
	for (int i = 0; i < key_length; i++)
	{
		keys[i] = rand() % 256;
	}
	uint16_t length = key_length + KEYSUPPLYHEADER;
	packet.ConstrctPacket(seq, length, keys);
	return true;
}

int i_listenfd = -1;
int i_connfd_1 = -1;
int i_connfd_2 = -1;

void SigCatch(int sigNum) // 信号捕捉函数(捕获Ctrl+C)
{
	if (i_listenfd != -1 && i_connfd_1 != -1 && i_connfd_2 != -1)
	{
		close(i_connfd_1);
		close(i_connfd_2);
		close(i_listenfd);
	}
	printf("Bye~! Will Exit...\n");
	exit(0);
}

int main()
{
	// int i_listenfd, i_connfd;
	struct sockaddr_in st_sersock;
	// char msg[MAXSIZE];
	int nsendSize = 0;

	signal(SIGINT, SigCatch); // 注册信号捕获函数

	if ((i_listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // 建立socket套接字
	{
		printf("socket Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	memset(&st_sersock, 0, sizeof(st_sersock));
	st_sersock.sin_family = AF_INET;				// IPv4协议
	st_sersock.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY转换过来就是0.0.0.0，泛指本机的意思，也就是表示本机的所有IP，因为有些机子不止一块网卡，多网卡的情况下，这个就表示所有网卡ip地址的意思。
	st_sersock.sin_port = htons(IP_PORT);

		int opt = 1;
	// 设置端口复用	(解决端口被占用的问题)
	if (setsockopt(i_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // 设置端口复用	(解决端口被占用的问题)
	{
		printf("setsockopt Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	if (bind(i_listenfd, (struct sockaddr *)&st_sersock, sizeof(st_sersock)) < 0) // 将套接字绑定IP和端口用于监听
	{
		printf("bind Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	if (listen(i_listenfd, 20) < 0) // 设定可同时排队的客户端最大连接个数
	{
		printf("listen Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	printf("======waiting for client's request======\n");
	// 准备接受客户端连接
	if ((i_connfd_1 = accept(i_listenfd, (struct sockaddr *)NULL, NULL)) < 0) // 阻塞等待客户端1连接
	{
		printf("accept Error: %s (errno: %d)\n", strerror(errno), errno);
		//	continue;
	}
	else
	{
		printf("Client[%d], welcome!\n", i_connfd_1);
	}
	if ((i_connfd_2 = accept(i_listenfd, (struct sockaddr *)NULL, NULL)) < 0) // 阻塞等待客户端2连接
	{
		printf("accept Error: %s (errno: %d)\n", strerror(errno), errno);
		//	continue;
	}
	else
	{
		printf("Client[%d], welcome!\n", i_connfd_2);
	}
	uint32_t seq = 1;
	while (1) // 循环
	{
		char msg[MAX_DATA_SIZE + KEYSUPPLYHEADER + BASE_HEADER_SIZE];
		memset(msg, 0, sizeof(msg));
		KeySupplyPacket packet;
		bool flag = create_qpacket(packet, seq);
		if (!flag)
		{
			printf("Failed to create keys!");
			exit(0);
		}
		packet.PackTcpPacket(msg);
		if (nsendSize = write(i_connfd_1, msg, MAX_DATA_SIZE + KEYSUPPLYHEADER + BASE_HEADER_SIZE) < 0) // 发送数据-client1
		{
			printf("write Error: %s (errno: %d)\n", strerror(errno), errno);
			exit(0);
		}
		else if (nsendSize = 0)
		{
			printf("\nFailed to send keys to the client_1!\n");
			exit(0);
		}
		else
		{
			printf("\nSucceed to send keys to the client_1!\n");
		}
		if (nsendSize = write(i_connfd_2, msg, MAX_DATA_SIZE + KEYSUPPLYHEADER + BASE_HEADER_SIZE) < 0) // 发送数据-client2
		{
			printf("write Error: %s (errno: %d)\n", strerror(errno), errno);
			exit(0);
		}
		else if (nsendSize = 0)
		{
			printf("\nFailed to send keys to the client_2!\n");
			exit(0);
		}
		else
		{
			printf("\nSucceed to send keys to the client_2!\n");
		}
		seq++;
		sleep(1);
	}
	// close(i_connfd);
	// close(i_listenfd);
	return 0;
}