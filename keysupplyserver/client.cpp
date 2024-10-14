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

void SigCatch(int sigNum) // �źŲ�׽����(����Ctrl+C)
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
	signal(SIGINT, SigCatch); // ע���źŲ�����

	if ((i_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // �����׽���
	{
		printf("socket Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	memset(&st_clnsock, 0, sizeof(st_clnsock));
	st_clnsock.sin_family = AF_INET; // IPv4Э��
	// IP��ַת��(ֱ�ӿ��Դ������ֽ���ĵ��ʮ���� ת���������ֽ���)
	if (inet_pton(AF_INET, IP_ADDR, &st_clnsock.sin_addr) <= 0)
	{
		printf("inet_pton Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}
	st_clnsock.sin_port = htons(IP_PORT); // �˿�ת��(�����ֽ��������ֽ���)

	if (connect(i_sockfd, (struct sockaddr *)&st_clnsock, sizeof(st_clnsock)) < 0) // ���������õ�IP�Ͷ˿ںŵķ���˷�������
	{
		printf("connect Error: %s (errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	printf("======connect to server, send request and wait for data======\n");
	while (1) // ѭ��, ���ܷ���˷��ص�����
	{
		KeySupplyPacket pkt1;
		// ��ȡpacket header
		ssize_t bytes_read = read(i_sockfd, pkt1.getBufferPtr(), BASE_HEADER_SIZE);
		if (bytes_read < 0)
		{
			perror("read Error");
			close(i_sockfd);
			exit(0);
		}
		else if (bytes_read == 0)
		{
			// ����˹ر�������
			std::cout << "Connection closed on socket: " << i_sockfd << std::endl;
			close(i_sockfd);
		}

		uint16_t length;
		std::memcpy(&length, pkt1.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));
		// ��ȡpayload
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