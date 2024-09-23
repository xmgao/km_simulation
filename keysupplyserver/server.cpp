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

void SigCatch(int sigNum) // �źŲ�׽����(����Ctrl+C)
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

	signal(SIGINT, SigCatch); // ע���źŲ�����

	if ((i_listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // ����socket�׽���
	{
		printf("socket Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	memset(&st_sersock, 0, sizeof(st_sersock));
	st_sersock.sin_family = AF_INET;				// IPv4Э��
	st_sersock.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANYת����������0.0.0.0����ָ��������˼��Ҳ���Ǳ�ʾ����������IP����Ϊ��Щ���Ӳ�ֹһ��������������������£�����ͱ�ʾ��������ip��ַ����˼��
	st_sersock.sin_port = htons(IP_PORT);

		int opt = 1;
	// ���ö˿ڸ���	(����˿ڱ�ռ�õ�����)
	if (setsockopt(i_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // ���ö˿ڸ���	(����˿ڱ�ռ�õ�����)
	{
		printf("setsockopt Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	if (bind(i_listenfd, (struct sockaddr *)&st_sersock, sizeof(st_sersock)) < 0) // ���׽��ְ�IP�Ͷ˿����ڼ���
	{
		printf("bind Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	if (listen(i_listenfd, 20) < 0) // �趨��ͬʱ�ŶӵĿͻ���������Ӹ���
	{
		printf("listen Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	printf("======waiting for client's request======\n");
	// ׼�����ܿͻ�������
	if ((i_connfd_1 = accept(i_listenfd, (struct sockaddr *)NULL, NULL)) < 0) // �����ȴ��ͻ���1����
	{
		printf("accept Error: %s (errno: %d)\n", strerror(errno), errno);
		//	continue;
	}
	else
	{
		printf("Client[%d], welcome!\n", i_connfd_1);
	}
	if ((i_connfd_2 = accept(i_listenfd, (struct sockaddr *)NULL, NULL)) < 0) // �����ȴ��ͻ���2����
	{
		printf("accept Error: %s (errno: %d)\n", strerror(errno), errno);
		//	continue;
	}
	else
	{
		printf("Client[%d], welcome!\n", i_connfd_2);
	}
	uint32_t seq = 1;
	while (1) // ѭ��
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
		if (nsendSize = write(i_connfd_1, msg, MAX_DATA_SIZE + KEYSUPPLYHEADER + BASE_HEADER_SIZE) < 0) // ��������-client1
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
		if (nsendSize = write(i_connfd_2, msg, MAX_DATA_SIZE + KEYSUPPLYHEADER + BASE_HEADER_SIZE) < 0) // ��������-client2
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