#include "opensessionpacket.hpp"
#include "keyrequestpacket.hpp"
#include "Encryptor.hpp"
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
#include <thread>
#include <chrono>

const int LISTEN_PORT = 50000;     // KM�����˿�
const int APP_LISTEN_PORT = 50001; // APP�����˿�

// 192.168.8.154  192.168.8.182

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

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <proactiveAPP_IP_address> <passiveAPP_IP_address>" << std::endl;
        return 1;
    }

    std::string proactiveAPP_ipAddress = argv[1];
    std::string passiveAPP_ipAddress = argv[2];

    // ���ӱ�����KM
    int conn_KM_fd = -1;
    const int max_retries = 100; // ����������Դ���
    int retries = 0;
    const std::string KM_IP_ADDRESS = "127.0.0.1";

    while (conn_KM_fd <= 0 && retries < max_retries)
    {
        conn_KM_fd = connectToServer(KM_IP_ADDRESS, LISTEN_PORT);
        if (conn_KM_fd <= 0)
        {
            std::cerr << "Failed to connect, retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2)); // �ȴ�2��
            retries++;
        }
    }
    // �������״̬������ʧ�����
    if (conn_KM_fd <= 0)
    {
        std::cerr << "Failed to connect after " << max_retries << " retries." << std::endl;
        // ��������Ӵ����߼������緵���ض���������׳��쳣
        return 0;
    }

    // ����ɹ���������������
    std::cout << "Connected successfully!" << std::endl;
    // ����������APP����
    struct sockaddr_in st_sersock;
    int conn_Listen_fd = -1;

    if ((conn_Listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // ����socket�׽���
    {
        printf("socket Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    memset(&st_sersock, 0, sizeof(st_sersock));
    st_sersock.sin_family = AF_INET;
    st_sersock.sin_addr.s_addr = htonl(INADDR_ANY); // ����������
    st_sersock.sin_port = htons(APP_LISTEN_PORT);

    int opt = 1;
    // ���ö˿ڸ���	(����˿ڱ�ռ�õ�����)
    if (setsockopt(conn_Listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // ���ö˿ڸ���	(����˿ڱ�ռ�õ�����)
    {
        printf("setsockopt Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    if (bind(conn_Listen_fd, (struct sockaddr *)&st_sersock, sizeof(st_sersock)) < 0) // ���׽��ְ�IP�Ͷ˿����ڼ���
    {
        printf("bind Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    if (listen(conn_Listen_fd, 20) < 0) // �趨��ͬʱ�ŶӵĿͻ���������Ӹ���
    {
        printf("listen Error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    printf("======waiting for client's request======\n");
    // ׼������������APP����
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_proactiveAPP_fd = accept(conn_Listen_fd, (struct sockaddr *)&client_addr, &client_len);
    if (conn_proactiveAPP_fd < 0)
    {
        perror("accept");
        return 0;
    }

    uint32_t sourceip = IpStringTouint32(proactiveAPP_ipAddress);
    uint32_t desip = IpStringTouint32(passiveAPP_ipAddress);
    uint32_t session_id = 1;
    uint32_t request_len = 128;
    uint32_t request_id = 1;

    // ���ӳɹ����������ѭ��
    while (request_id < 20)
    {
        // ��ȡ������APP�������ݰ�
        char buffer[512];
        ssize_t bytes_read = read(conn_proactiveAPP_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0)
        {
            perror("read Error");
            close(conn_proactiveAPP_fd);
            return 0;
        }
        // ������������ת��Ϊ std::string��ȷ��ֻʹ�ö�ȡ���ֽ���
        std::string ciphertext(buffer, bytes_read);

        std::string decryptedtext;
        // ��ȡ��Կ
        std::string getkeyvalue;

    label1: // ��ǩ��������������Կ

        KeyRequestPacket pkt2;
        pkt2.constructkeyrequestpacket(session_id, request_id, request_len);
        if (send(conn_KM_fd, pkt2.getBufferPtr(), pkt2.getBufferSize(), 0) == -1)
        {
            perror("send Error");
            close(conn_KM_fd);
            return 0;
        }

        // ������Կ����
        PacketBase pkt3;

        // ��ȡpacket header
        ssize_t bytes_read2 = read(conn_KM_fd, pkt3.getBufferPtr(), BASE_HEADER_SIZE);
        if (bytes_read2 <= 0)
        {
            perror("read Error");
            close(conn_KM_fd);
            return 0;
        }

        uint16_t value1, length;
        std::memcpy(&value1, pkt3.getBufferPtr(), sizeof(uint16_t));
        std::memcpy(&length, pkt3.getBufferPtr() + sizeof(uint16_t), sizeof(uint16_t));

        // ��ȡpayload
        bytes_read2 = read(conn_KM_fd, pkt3.getBufferPtr() + BASE_HEADER_SIZE, length);
        if (bytes_read2 != length)
        {
            perror("Incomplete payload read");
            close(conn_KM_fd);
            return 0;
        }

        pkt3.setBufferSize(BASE_HEADER_SIZE + length);

        if (value1 == static_cast<uint16_t>(PacketType::KEYRETURN))
        {
            // ���ι���KeyRequestPacket
            KeyRequestPacket pkt4(std::move(pkt3));
            getkeyvalue.resize(request_len);
            std::memcpy(&getkeyvalue[0], pkt4.getKeyBufferPtr(), request_len);
        }
        else
        {
            perror("getkey Error");
            goto label1;
        }

        // ����
        if (Encryptor::decrypt(ciphertext, getkeyvalue, decryptedtext))
        {
            std::cout << "Decrypted text: " << decryptedtext << std::endl;
            for (const auto &ch : ciphertext)
            {
                std::cout << std::hex << (int)ch << " ";
            }
            std::cout << std::dec << std::endl; // �ָ�ʮ���Ƹ�ʽ
        }
        else
        {
            std::cerr << "Decryption failed due to insufficient key length." << std::endl;
        }

        // ��������ID����ֹ�ظ�ʹ����ͬ����Կ
        request_id++;
    }
    // �رջỰ
    OpenSessionPacket closepkt;
    closepkt.constructclosesessionpacket(sourceip, desip, session_id, true);
    send(conn_KM_fd, closepkt.getBufferPtr(), closepkt.getBufferSize(), 0);
    close(conn_KM_fd);
    // �ر��ļ�������
    close(conn_proactiveAPP_fd);
    close(conn_Listen_fd);
}
