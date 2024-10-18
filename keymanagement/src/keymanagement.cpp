#include "keymanagement.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

#define KEY_UNIT_SIZE 512

// ����ȫ�ֱ�������������
extern KeyManager globalKeyManager;

// ���캯��
KeyManager::KeyManager()
    : count_(0), delete_count_(0), seq_odd_inorder_(1), seq_even_inorder_(2)
{
    // �������Ҫ��ʼ���ļ�ϵͳ���֣����Լ�������
}

// �����Կ, ����Կ�洢�ɱ�����Կ�ļ�
void KeyManager::addKey(int seq, const uint8_t *keyValue, size_t keySize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // ����Կת��Ϊ�ַ�����ʽ����ӵ�map��
    std::string keyValueStr(keyValue, keyValue + keySize);
    keyMap_[seq] = keyValueStr;
    count_++;
}

// ��ȡ��Կ��ͨ��SEQ��ȡ
std::string KeyManager::getKey(int seq)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = keyMap_.find(seq);
    if (it != keyMap_.end())
    {
        return it->second;
    }
    else
    {
        return ""; // �����׳��쳣
    }
}

// ��ȡ��ǰ���õ���С��Կ��ţ������ȡ��Կ
int KeyManager::getKeyinOrder(bool odd_number)
{

    if (odd_number)
    {
        while (seq_odd_inorder_ <= 2 * count_)
        {
            auto it = keyMap_.find(seq_odd_inorder_);
            if (it != keyMap_.end())
            {
                seq_odd_inorder_ += 2;
                return seq_odd_inorder_ - 2;
            }
            else
            {
                seq_odd_inorder_ += 2;
                continue;
            }
        }
    }
    else
    {
        while (seq_even_inorder_ <= 2 * count_)
        {
            auto it = keyMap_.find(seq_even_inorder_);
            if (it != keyMap_.end())
            {
                seq_even_inorder_ += 2;
                return seq_even_inorder_ - 2;
            }
            else
            {
                seq_even_inorder_ += 2;
                continue;
            }
        }
    }
    return -1; // ���û���ҵ����ʵ��ļ�������-1
}

// ɾ����Կ��ͨ��SEQɾ��
void KeyManager::removeKey(int seq)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = keyMap_.find(seq);
    if (it != keyMap_.end())
    {
        keyMap_.erase(seq);
        count_--;
        delete_count_++;
    }
}

void KeyManager::monitorKeyRate()
{
    using namespace std::chrono;
    int last_timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int last_keypool_size = count_ + delete_count_;
    int last_used_size = delete_count_;

    while (true)
    {

        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // ÿ����һ��
        //�����������ٽ���
        std::lock_guard<std::mutex> lock(mutex_);
        int current_timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        int current_keypool_size = count_ + delete_count_;
        int current_used_size = delete_count_;

        int delta_timestamp = current_timestamp - last_timestamp; // ʱ������
        int delta_keypool_size = current_keypool_size - last_keypool_size;
        int delta_used_size = current_used_size - last_used_size;

        float generate_keyrate = static_cast<float>(delta_keypool_size * KEY_UNIT_SIZE) / (delta_timestamp / 1000.0);
        float consume_keyrate = static_cast<float>(delta_used_size * KEY_UNIT_SIZE) / (delta_timestamp / 1000.0);

        std::cout << "generate Key rate: " << generate_keyrate << " bps" << std::endl;
        std::cout << "consume Key rate: " << consume_keyrate << " bps" << std::endl;
        // ����ʱ�������Կ�ش�С
        last_timestamp = current_timestamp;
        last_keypool_size = current_keypool_size;
        last_used_size = current_used_size;
        //�뿪������ʱ, lock�������٣��Զ�����
    }
}

// ������Կ����
void KeyManager::run(Server &externalserver, const std::string &keysupply_ipAddress, int keysuply_port)
{
    // ��ʼ����������Կ����ģ����
    int conn_fd = -1;
    int max_retries = 100; // ����������Դ���
    int retries = 0;
    while (conn_fd <= 0 && retries < max_retries)
    {
        conn_fd = connectToServer(keysupply_ipAddress, keysuply_port);
        if (conn_fd <= 0)
        {
            std::cerr << "Failed to connect, retrying..." << std::endl;
            // �ȴ�2��
            sleep(2);
            retries++;
        }
    }
    if (conn_fd > 0)
    {
        // ���ӳɹ�,����¼�����
        if (externalserver.epoll_fd_ >= 0)
        {
            addToEpoll(externalserver.epoll_fd_, conn_fd);
        }
        else
        {
            std::cerr << "epoll_fd closed!" << std::endl;
            close(conn_fd);
        }
    }
    else
    {
        std::cerr << "Failed to connect after " << max_retries << " attempts." << std::endl;
    }
    // ���Ӻ������Կ���ʼ�⣬������
    std::thread keyRateThread(&KeyManager::monitorKeyRate, this);
    keyRateThread.detach();
}
