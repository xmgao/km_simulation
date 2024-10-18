#ifndef SESSIONMANAGEMENT_HPP
#define SESSIONMANAGEMENT_HPP

#include "keymanagement.hpp"
#include "sessionkeysyncpacket.hpp"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <iomanip>
#include <stdexcept>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <queue>

struct SessionData
{
    std::vector<uint8_t> keyValue;
    uint32_t session_id_;
    uint32_t sourceip_;
    uint32_t desip_;
    int index_ = 0;
    int fd_ = -1;
    // 标识是主动端还是被动端,false如果是主动端
    bool is_inbound_;
    // 新增成员：用于存储使用过的seq的队列
    std::queue<int> usedSeq;
};

class SessionManager
{
public:
    // 构造函数
    SessionManager();

    // 新建会话
    bool addSession(uint32_t sourceip, uint32_t desip, uint32_t session_id,bool is_inbound);

    // 通知被动端创建会话
    bool noticePassiveSession(SessionData &data, uint32_t session_id);

    //被动端被动获取密钥
    bool addPassiveKey(uint32_t  session_id,uint32_t key_seqnum);

    // 获取密钥，通过request读取
    std::string getKey(uint32_t session_id, uint32_t request_id, uint16_t request_len);

    // 删除会话，通过session_id删除
    bool closeSession(uint32_t session_id);

private:
    std::mutex mutex_;
    // 定义session_id到KeyData的映射
    std::unordered_map<uint32_t, SessionData> SessionkeyCache_;
    // 定义全局会话数量
    int session_number;
};



#endif // SESSIONMANAGEMENT_HPP