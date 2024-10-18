#include "keymanagement.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

#define KEY_UNIT_SIZE 512

// 声明全局变量，但不定义
extern KeyManager globalKeyManager;

// 构造函数
KeyManager::KeyManager()
    : count_(0), delete_count_(0), seq_odd_inorder_(1), seq_even_inorder_(2)
{
    // 如果还需要初始化文件系统部分，可以继续保留
}

// 添加密钥, 将密钥存储成本地密钥文件
void KeyManager::addKey(int seq, const uint8_t *keyValue, size_t keySize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 将密钥转换为字符串形式并添加到map中
    std::string keyValueStr(keyValue, keyValue + keySize);
    keyMap_[seq] = keyValueStr;
    count_++;
}

// 获取密钥，通过SEQ读取
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
        return ""; // 或者抛出异常
    }
}

// 获取当前可用的最小密钥序号，方便读取密钥
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
    return -1; // 如果没有找到合适的文件，返回-1
}

// 删除密钥，通过SEQ删除
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

        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 每秒检测一次
        //上锁，进入临界区
        std::lock_guard<std::mutex> lock(mutex_);
        int current_timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        int current_keypool_size = count_ + delete_count_;
        int current_used_size = delete_count_;

        int delta_timestamp = current_timestamp - last_timestamp; // 时间差，毫秒
        int delta_keypool_size = current_keypool_size - last_keypool_size;
        int delta_used_size = current_used_size - last_used_size;

        float generate_keyrate = static_cast<float>(delta_keypool_size * KEY_UNIT_SIZE) / (delta_timestamp / 1000.0);
        float consume_keyrate = static_cast<float>(delta_used_size * KEY_UNIT_SIZE) / (delta_timestamp / 1000.0);

        std::cout << "generate Key rate: " << generate_keyrate << " bps" << std::endl;
        std::cout << "consume Key rate: " << consume_keyrate << " bps" << std::endl;
        // 更新时间戳和密钥池大小
        last_timestamp = current_timestamp;
        last_keypool_size = current_keypool_size;
        last_used_size = current_used_size;
        //离开作用域时, lock对象销毁，自动解锁
    }
}

// 运行密钥管理
void KeyManager::run(Server &externalserver, const std::string &keysupply_ipAddress, int keysuply_port)
{
    // 开始尝试连接密钥生成模拟器
    int conn_fd = -1;
    int max_retries = 100; // 设置最大重试次数
    int retries = 0;
    while (conn_fd <= 0 && retries < max_retries)
    {
        conn_fd = connectToServer(keysupply_ipAddress, keysuply_port);
        if (conn_fd <= 0)
        {
            std::cerr << "Failed to connect, retrying..." << std::endl;
            // 等待2秒
            sleep(2);
            retries++;
        }
    }
    if (conn_fd > 0)
    {
        // 连接成功,添加事件上树
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
    // 连接后进行密钥速率检测，净速率
    std::thread keyRateThread(&KeyManager::monitorKeyRate, this);
    keyRateThread.detach();
}
