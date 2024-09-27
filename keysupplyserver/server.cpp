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
#include <chrono>
#include <thread>
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
	char op;
	char index[14][7] = {"231231", "240101", "240104", "240105", "240106", "240107", "240108", "240110", "240111", "240112", "240114", "240115", "240116", "240117"};
	printf("please choose the channel attenuation cased by the environment (0-13):\n");
	printf("0: 3.33dB(3.23dB) 98.522kbps;\n");
	printf("1: 5.33dB(5.35dB) 89.575kbps;\n");
	printf("2: 7.33dB(7.30dB) 70.317kbps;\n");
	printf("3: 9.32dB(9.29dB) 48.371kbps;\n");
	printf("4: 11.34dB(11.30dB) 31.440kbps;\n");
	printf("5: 13.33dB(13.23dB) 17.72kbps;\n");
	printf("6: 15.33dB(15.36dB) 10.57kbps;\n");
	printf("7: 17.33dB(17.44dB) 6.56kbps;\n");
	printf("8: 19.34dB(19.34dB) 4.12kbps;\n");
	printf("9: 21.35dB(21.35dB) 2.41kbps;\n");
	printf("10: 23.34dB(23.16dB) 1.184kbps;\n");
	printf("11: 25.34dB(25.35dB) 0.422kbps;\n");
	printf("12: 26.35dB(26.35dB) 0.235kbps;\n");
	printf("13: ~0dB(~0dB) 100.98kbps;\n");
	printf("Your option:");
	op = getchar();
	char name[20] = "keyfile";
	strcat(name, index[op-48]);
	strcat(name, ".kf");
	// printf("%s", name);
	uint8_t timestamp[8], length[2];
	uint8_t* block;
	uint64_t pre_ts, af_ts, dura;
	FILE* fp;
	fp = fopen(name, "rb+");
	fread(timestamp, sizeof(uint8_t), 8, fp);
	pre_ts = timestamp[7];
	for(int i = 6; i >= 0; i--)
	{
		pre_ts = pre_ts * 256 + timestamp[i];
	}
	while (1) // 循环
	{
		KeySupplyPacket packet;
		auto start = std::chrono::system_clock::now();
		fread(length, sizeof(uint8_t), 2, fp);
		uint16_t len = length[1]*256+length[0];
		if(len <= MAX_DATA_SIZE)
		{
			block = (uint8_t*)malloc(len);
		}
		else
		{
			printf("\nthe size of key is too large!\n");
			exit(0);
		}
		char msg[len+KEYSUPPLYHEADER+BASE_HEADER_SIZE];
		memset(msg, 0, sizeof(msg));
		fread(block, sizeof(uint8_t), length[1]*256+length[0], fp);
		packet.ConstrctPacket(seq, len, block);
		fread(timestamp, sizeof(uint8_t), 8, fp);
		af_ts = timestamp[7];
		for(int i = 6; i >= 0; i--)
		{
			af_ts = af_ts * 256 + timestamp[i];
		}
		dura = af_ts-pre_ts;
		// printf("%lu ", dura);//the duration is ns.\n
		pre_ts = af_ts;
		// count = count-1;
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<uint64_t, std::nano> past = end - start;
		// printf("has pasted %" PRIu64 " ns.\n", past.count());
		uint32_t num = dura - past.count();
		// printf("%u ", num);//still need to wait: ns.\n
		std::this_thread::sleep_for(std::chrono::nanoseconds(num));
		packet.PackTcpPacket(msg);
		if ((nsendSize = write(i_connfd_1, msg, MAX_DATA_SIZE + KEYSUPPLYHEADER + BASE_HEADER_SIZE)) < 0)
		{
			printf("write Error: %s (errno: %d)\n", strerror(errno), errno);
			exit(0);
		}
		else if (nsendSize == 0)
		{
			printf("\nFailed to send keys to the client_1!\n");
			exit(0);
		}
		else
		{
			printf("\nSucceed to send keys to the client_1!\n");
		}
		if ((nsendSize = write(i_connfd_2, msg, MAX_DATA_SIZE+KEYSUPPLYHEADER+BASE_HEADER_SIZE)) < 0)	
		{
			printf("write Error: %s (errno: %d)\n", strerror(errno), errno);
			exit(0);
		}
		else if (nsendSize == 0)
		{
			printf("\nFailed to send keys to the client_2!\n");
			exit(0);
		}
		else
		{
			printf("\nSucceed to send keys to the client_2!\n");
		}
		seq++;
	}
	fclose(fp);
	close(i_connfd_1);
	close(i_connfd_2);
	close(i_listenfd);
	return 0;
}