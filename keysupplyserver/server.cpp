#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include "packetbase.hpp"
#include "keysupplypacket.hpp"

// 常数定义
#define IP_PORT 50003
#define KEYLENGTH 512

using namespace std;

bool createQPacket(KeySupplyPacket &packet, uint32_t seq)
{
	uint8_t keys[KEYLENGTH];
	for (int i = 0; i < KEYLENGTH; i++)
	{
		keys[i] = rand() % 256;
	}
	uint16_t length = KEYLENGTH + KEYSUPPLYHEADER;
	packet.ConstrctPacket(seq, length, keys);
	return true;
}

// 全局变量
int i_listenfd = -1;
int i_connfd_1 = -1;
int i_connfd_2 = -1;

// 信号捕获函数
void sigCatch(int sigNum)
{
	if (i_listenfd != -1)
		close(i_listenfd);
	if (i_connfd_1 != -1)
		close(i_connfd_1);
	if (i_connfd_2 != -1)
		close(i_connfd_2);
	cerr << "Exiting on signal " << sigNum << ".\n";
	exit(0);
}

// 初始化网络连接
int initNetworkConnection(int &listen_fd, struct sockaddr_in &serv_addr)
{
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		cerr << "Socket creation failed: " << strerror(errno) << "\n";
		return -1;
	}

	int opt = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		cerr << "Setting socket options failed: " << strerror(errno) << "\n";
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(IP_PORT);

	if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		cerr << "Bind failed: " << strerror(errno) << "\n";
		return -1;
	}

	if (listen(listen_fd, 20) < 0)
	{
		cerr << "Listen failed: " << strerror(errno) << "\n";
		return -1;
	}

	return listen_fd;
}

// 建立和接受客户端连接
int acceptClient(int listen_fd)
{
	int conn_fd = accept(listen_fd, nullptr, nullptr);
	if (conn_fd < 0)
	{
		cerr << "Accept failed: " << strerror(errno) << "\n";
	}
	return conn_fd;
}

// 处理键值发送逻辑
void processKeySupply(uint64_t &pre_ts, ifstream &inputFile)
{
	uint32_t seq = 1;
	string line;

	while (getline(inputFile, line))
	{
		KeySupplyPacket packet;

		// 创建数据包
		auto start = chrono::system_clock::now();
		createQPacket(packet, seq);

		// 计算时间间隔
		uint64_t timestamp = stoull(line);
		uint64_t dura = timestamp - pre_ts;
		pre_ts = timestamp;

		auto end = chrono::system_clock::now();
		chrono::duration<uint64_t, nano> past = end - start;
		uint64_t delay_ns = dura - past.count();

		this_thread::sleep_for(chrono::nanoseconds(delay_ns));

		// 发送数据包
		auto sendData = [&](int conn_fd)
		{
			if (write(conn_fd, packet.getBufferPtr(), packet.getBufferSize()) < 0)
			{
				cerr << "Failed to send keys to the client [" << conn_fd << "]: " << strerror(errno) << "\n";
				return false;
			}
			return true;
		};

		if (!sendData(i_connfd_1) || !sendData(i_connfd_2))
		{
			break;
		}

		seq++;
	}
}

int main()
{
	// 注册信号处理
	signal(SIGINT, sigCatch);

	// 选择文件
	int op;
	cout << "Please choose the channel attenuation case by inputting a number (0-13): ";
	cin >> op;

	string fileNames[] = {
		"231231", "240101", "240104", "240105", "240106", "240107",
		"240108", "240110", "240111", "240112", "240114", "240115",
		"240116", "240117"};

	string filePath = "timestap/" + fileNames[op] + ".txt";

	struct sockaddr_in serv_addr;
	if (initNetworkConnection(i_listenfd, serv_addr) < 0)
	{
		return 1;
	}

	cout << "Server is listening on port " << IP_PORT << ".\n";

	i_connfd_1 = acceptClient(i_listenfd);
	i_connfd_2 = acceptClient(i_listenfd);

	if (i_connfd_1 < 0 || i_connfd_2 < 0)
	{
		return 1;
	}
	cout << "Accepted connections from clients.\n";

	ifstream inputFile(filePath);

	if (!inputFile.is_open())
	{
		cerr << "Failed to open file: " << filePath << "\n";
		return 1;
	}

	uint64_t pre_ts = 0;
	string initialLine;
	if (getline(inputFile, initialLine))
	{
		pre_ts = stoull(initialLine);
	}
	else
	{
		cerr << "File is empty or read failed.\n";
		return 1;
	}

	processKeySupply(pre_ts, inputFile);

	// 释放资源
	inputFile.close();
	if (i_connfd_1 != -1)
		close(i_connfd_1);
	if (i_connfd_2 != -1)
		close(i_connfd_2);
	if (i_listenfd != -1)
		close(i_listenfd);

	return 0;
}