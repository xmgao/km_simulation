#include "keymanagement.hpp"
#include <sstream>
#include <chrono>
#include <thread>

#define KEY_UNIT_SIZE 512

// 声明全局变量，但不定义
extern KeyManager globalKeyManager;

// 构造函数
KeyManager::KeyManager()
    : count_(0), delete_count_(0), seq_odd_inorder_(1), seq_even_inorder_(2), seq_max_(0)
{
    // 如果还需要初始化文件系统部分，可以继续保留
}

// 添加密钥, 将密钥存储成本地密钥文件
void KeyManager::addKey(int seq, const uint8_t *keyValue, size_t keySize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    keyMap_[seq] = std::string(reinterpret_cast<const char *>(keyValue), keySize);
    if (seq > seq_max_)
    {
        seq_max_ = seq;
    }
    ++count_;
}

// 获取密钥，通过SEQ读取
std::string KeyManager::getKey(int seq)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = keyMap_.find(seq);
    return it != keyMap_.end() ? it->second : ""; // 或者抛出异常
}

// 获取当前可用的最小密钥序号，方便读取密钥
int KeyManager::getKeyinOrder(bool odd_number)
{
    std::lock_guard<std::mutex> lock(mutex_); // 防止序号在并发环境下被不当访问

    int &seq_inorder = odd_number ? seq_odd_inorder_ : seq_even_inorder_;
    int step = odd_number ? 2 : 2;
    // 查找密钥找到超过最大序列号
    while (seq_inorder <= seq_max_)
    {
        if (keyMap_.find(seq_inorder) != keyMap_.end())
        {
            int result = seq_inorder;
            seq_inorder += step;
            return result;
        }
        seq_inorder += step;
    }
    return -1; // 如果没有找到合适的密钥文件，说明当前密钥存量不足，返回-1
}

// 删除密钥，通过SEQ删除
void KeyManager::removeKey(int seq)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (keyMap_.erase(seq))
    {
        --count_;
        ++delete_count_;
    }
}

void KeyManager::monitorKeyRate()
{
    using namespace std::chrono;

    auto last_timestamp = system_clock::now();
    int last_keypool_size = count_ + delete_count_;
    int last_used_size = delete_count_;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒检测一次

        std::lock_guard<std::mutex> lock(mutex_);
        auto current_timestamp = system_clock::now();
        int current_keypool_size = count_ + delete_count_;
        int current_used_size = delete_count_;

        auto duration = duration_cast<milliseconds>(current_timestamp - last_timestamp).count();
        int delta_keypool_size = current_keypool_size - last_keypool_size;
        int delta_used_size = current_used_size - last_used_size;

        // float generate_keyrate = delta_keypool_size * KEY_UNIT_SIZE * 8 / (duration / 1000.0f);
        // float consume_keyrate = delta_used_size * KEY_UNIT_SIZE * 8 / (duration / 1000.0f);

        // std::cout << "Generate Key rate: " << generate_keyrate << " bps" << std::endl;
        // std::cout << "Consume Key rate: " << consume_keyrate << " bps" << std::endl;

        // 更新上次检查的时间和密钥池大小
        last_timestamp = current_timestamp;
        last_keypool_size = current_keypool_size;
        last_used_size = current_used_size;
    }
}

// 运行密钥管理
void KeyManager::run(Server &externalserver, const std::string &keysupply_ipAddress, int keysuply_port)
{
    int conn_fd = -1;
    const int max_retries = 100;
    int retries = 0;

    while (conn_fd <= 0 && retries < max_retries)
    {
        conn_fd = connectToServer(keysupply_ipAddress, keysuply_port);
        if (conn_fd <= 0)
        {
            std::cerr << "Failed to connect, retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ++retries;
        }
    }

    if (conn_fd > 0)
    {
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

    // 连接后进行密钥速率检测，启动密钥速率监控线程
    std::thread keyRateThread(&KeyManager::monitorKeyRate, this);
    keyRateThread.detach();
}