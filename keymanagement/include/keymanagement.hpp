#ifndef KEYMANAGEMENT_HPP
#define KEYMANAGEMENT_HPP

#include "server.hpp"
#include <string>
#include <map>
#include <mutex>
#include <cstdint>
#include <iomanip>


class KeyManager
{
public:
    // 构造函数
    KeyManager();

    // 添加密钥, 将密钥存储成本地密钥文件
    void addKey(int seq, const uint8_t *keyValue, size_t keySize);

    // 获取密钥，通过SEQ读取
    std::string getKey(int seq);

    // 获取当前可用的最小密钥序号，方便读取密钥
    int getKeyinOrder(bool odd_number);

    // 删除密钥，通过SEQ删除
    void removeKey(int seq);

    //密钥速率检测
    void monitorKeyRate();

    void run(Server &externalserver, const std::string &keysupply_ipAddress, int keysuply_port);

private:
    std::mutex mutex_;
    // 当前密钥池的密钥文件个数
    int count;
    // 当前按照顺序的最小奇数密钥序号
    int seq_odd_inorder_;
    // 当前按照顺序的最小偶数密钥序号
    int seq_even_inorder_;
    
    std::map<int, std::string> keyMap_; // 用于存储密钥,密钥块用char buffer[]或vector<unint_8>
};



#endif