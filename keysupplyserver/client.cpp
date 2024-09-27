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

#define MAXSIZE 10//byte
#define IP_ADDR "127.0.0.1"
#define IP_PORT 8888



int i_sockfd = -1;

void SigCatch(int sigNum)	//�źŲ�׽����(����Ctrl+C)
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
	int nrecvSize = 0;

	signal(SIGINT, SigCatch);	//ע���źŲ�����

	if ((i_sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0)	//�����׽���
	{
		printf("socket Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	memset(&st_clnsock, 0, sizeof(st_clnsock));
	st_clnsock.sin_family = AF_INET;  //IPv4Э��
	//IP��ַת��(ֱ�ӿ��Դ������ֽ���ĵ��ʮ���� ת���������ֽ���)
	if (inet_pton(AF_INET, IP_ADDR, &st_clnsock.sin_addr) <= 0)
	{
		printf("inet_pton Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}
	st_clnsock.sin_port = htons(IP_PORT);	//�˿�ת��(�����ֽ��������ֽ���)

	if (connect(i_sockfd, (struct sockaddr*)&st_clnsock, sizeof(st_clnsock)) < 0)	//���������õ�IP�Ͷ˿ںŵķ���˷�������
	{
		printf("connect Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	printf("======connect to server, send request and wait for data======\n");

	int seq = 0;
	while(1)	//ѭ��, ���ܷ���˷��ص�����
	{
		// uint32_t seq = 0;
		char msg[MAX_DATA_SIZE+KEYSUPPLYHEADER+BASE_HEADER_SIZE];
		KeySupplyPacket packet;
		// while(1)
		// {
		// 	qpacket packet;
		// 	bool flag = create_qpacket(packet, seq);
		// 	if (!flag)
		// 	{
		// 		printf("Failed to create keys!");
		// 		exit(0);
		// 	}
		// 	// fgets(msg, MAXSIZE, stdin);
		// 	printf("type: %d, length: %d, seq: %d, will send: %s", packet.type, packet.length, packet.value.seq, trans(packet.value.key));
		// 	pack_tcp_key_packet(msg, packet);
		// 	if (write(i_sockfd, msg, MAXSIZE) < 0)	//��������
		// 	{
		// 		printf("write Error: %s (errno: %d)\n", strerror(errno), errno);
		// 		exit(0);
		// 	}
		memset(msg, 0, sizeof(msg));
		if((nrecvSize = read(i_sockfd, msg, MAX_DATA_SIZE+KEYSUPPLYHEADER+BASE_HEADER_SIZE)) < 0)	//��������
		{
			printf("read Error: %s (errno: %d)\n", strerror(errno), errno);
		}
		else if (nrecvSize == 0)
		{
			printf("Service Close!\n");
		}
		else
		{
			packet.UnpackTcpPacket(msg);
			// printf("Server return: %s\n", msg);
		}
		seq++;
		// }
	}
	return 0;
}