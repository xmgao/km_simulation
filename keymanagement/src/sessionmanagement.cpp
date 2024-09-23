#include "sessionmanagement.hpp"
#include "server.hpp"
#include "opensessionpacket.hpp"
#include "debuglevel.hpp"

extern KeyManager globalKeyManager;
// 声明全局变量，但不定义
extern SessionManager globalSessionManager;

SessionManager::SessionManager()
    : session_number(0) {}

void keysync(SessionData &data, uint32_t keyseqnum)
{
    SessionKeySyncPacket pkt1;
    pkt1.constructsessionkeysyncpacket(data.session_id_, keyseqnum);
    ssize_t bytesSent = send(data.fd_, pkt1.getBufferPtr(), pkt1.getBufferSize(), 0);
    if (bytesSent == -1)
    {
        std::cerr << "Failed to send message: " << std::endl;
    }
}

bool addProactiveKey(SessionData &data)
{
    std::string key;
    // 示例生成密钥的逻辑
    bool is_odd = data.sourceip_ > data.desip_ ? true : false;
    int seq = globalKeyManager.getKeyinOrder(is_odd);
    if (seq == -1)
    {
        // 错误处理
        std::cerr << "failed to find useful seq." << std::endl;
        return false;
    }
    data.usedSeq.push(seq);
    std::string keyValue1 = globalKeyManager.getKey(seq);
    // 用完删除密钥
    globalKeyManager.removeKey(seq);
    // 将 keyValue 插入 SessionkeyCache_[i] 的末尾
    data.keyValue.insert(data.keyValue.end(), keyValue1.begin(), keyValue1.end());
    //密钥同步
    keysync(data,seq);
    return true;
}

bool SessionManager::addPassiveKey(uint32_t session_id, uint32_t key_seqnum)
{
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        // 返回找到的密钥
        std::string keyValue1 = globalKeyManager.getKey(key_seqnum);
        // 用完删除密钥
        globalKeyManager.removeKey(key_seqnum);
        it->second.usedSeq.push(key_seqnum);
        // 将 keyValue 插入 SessionkeyCache_[i] 的末尾
        it->second.keyValue.insert(it->second.keyValue.end(), keyValue1.begin(), keyValue1.end());
        return true;
    }
    else
    {
        // 错误处理
        std::cerr << "failed to find session:" << session_id << std::endl;
        return false;
    }
}



bool SessionManager::addSession(uint32_t sourceip, uint32_t desip, uint32_t session_id, bool is_outbound)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 检查session_id是否已存在
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        if (it->second.is_outbound_)
        {
            std::cerr << "session already exists" << std::endl;
            return false; // 主动端会话已存在
        }
        else
        {
            return true; // 被动端会话可以先由主动端KM创建
        }
    }
    // 创建新的KeyData对象并插入映射
    SessionData newSessionData;
    newSessionData.sourceip_ = sourceip;
    newSessionData.desip_ = desip;
    newSessionData.session_id_ = session_id;
    // 判断是否是主动端
    if (is_outbound)
    {
        newSessionData.is_outbound_ = true;

        // 通知被动端创建会话
        noticePassiveSession(newSessionData, session_id);
        // 提前插入一块密钥,被动端不需要提前，需要从主动端同步
        if (!addProactiveKey(newSessionData))
        {
            // 错误处理
            std::cerr << "generatekey failed." << std::endl;
            return false;
        } // 假设有一个生成密钥的函数
    }
    else
    {
        newSessionData.is_outbound_ = false;
    }

    SessionkeyCache_[session_id] = newSessionData;
    ++session_number;
    return true;
}

bool SessionManager::noticePassiveSession(SessionData &data, uint32_t session_id)
{
    data.fd_ = connectToServer(uint32ToIpString(data.desip_), LISTEN_PORT);
    if (data.fd_ == -1)
    {
        std::cerr << "Connection failed: " << std::endl;
        return false;
    }
    OpenSessionPacket pkt1;
    pkt1.constructopensessionpacket(data.sourceip_, data.desip_, session_id, false);

    ssize_t bytesSent = send(data.fd_, pkt1.getBufferPtr(), pkt1.getBufferSize(), 0);
    if (bytesSent == -1)
    {
        std::cerr << "Failed to send message: " << std::endl;
        return false;
    }

    return true;
}

std::string SessionManager::getKey(uint32_t session_id, uint32_t request_id, uint16_t request_len)
{

    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        // 返回找到的密钥

        int useful_size = it->second.keyValue.size() - it->second.index_;
        while (useful_size < request_len)
        {
            if (it->second.is_outbound_)
            {
                if (!addProactiveKey(it->second))
                {
                    // 错误处理
                    std::cerr << "addproactivekey failed." << std::endl;
                    return "";
                } // 主动端补充密钥
            }
            else
            {
                return "";
                // 被动端返回空密钥
            }
            useful_size = it->second.keyValue.size() - it->second.index_;
        }
        std::string returnkeyvalue(it->second.keyValue.begin() + it->second.index_, it->second.keyValue.begin() + it->second.index_ + request_len);
        it->second.index_ += request_len;
        return returnkeyvalue;
    }
    return ""; // 如果未找到，返回空字符串
}

bool SessionManager::closeSession(uint32_t session_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        SessionkeyCache_.erase(it);
        --session_number;
        std::cout << "closeSession success!session id:" << session_id << std::endl;
        return true;
    }
    else
    {
        // 错误处理
        std::cerr << "closeSession failed!Unable to find session" << std::endl;
    }
    return false;
}
