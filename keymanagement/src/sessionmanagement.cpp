#include "sessionmanagement.hpp"
#include "server.hpp"
#include "packet/packets.hpp"
#include "debuglevel.hpp"

namespace
{
    void logError(const std::string &message)
    {
        std::cerr << message << std::endl;
    }

    void logInfo(const std::string &message)
    {
        std::cout << message << std::endl;
    }
}

extern KeyManager globalKeyManager;
extern int LISTEN_PORT;

SessionManager::SessionManager() : session_number(0) {}

void keysync(SessionData &data, uint32_t keyseqnum)
{
    SessionKeySyncPacket pkt1;
    pkt1.constructsessionkeysyncpacket(data.session_id_, keyseqnum);
    if (send(data.fd_, pkt1.getBufferPtr(), pkt1.getBufferSize(), 0) == -1)
    {
        logError("Failed to send message during key synchronization.");
    }
}

bool addKeyToSession(SessionData &data, bool isProactive)
{
    int seq = globalKeyManager.getKeyinOrder(data.sourceip_ > data.desip_);
    if (seq == -1)
    {
        logError("Failed to find a useful sequence number.");
        return false;
    }

    data.usedSeq.push(seq);
    std::string keyValue = globalKeyManager.getKey(seq);
    globalKeyManager.removeKey(seq);

    data.keyValue.insert(data.keyValue.end(), keyValue.begin(), keyValue.end());

    if (isProactive)
    {
        keysync(data, seq);
    }

    return true;
}

bool SessionManager::addPassiveKey(uint32_t session_id, uint32_t key_seqnum)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        it->second.usedSeq.push(key_seqnum);
        std::string keyValue = globalKeyManager.getKey(key_seqnum);
        globalKeyManager.removeKey(key_seqnum);
        it->second.keyValue.insert(it->second.keyValue.end(), keyValue.begin(), keyValue.end());
        return true;
    }
    logError("Failed to find session: " + std::to_string(session_id));
    return false;
}

bool SessionManager::addSession(uint32_t sourceip, uint32_t desip, uint32_t session_id, bool is_inbound)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        if (!it->second.is_inbound_)
        {
            logError("Session already exists.");
            return false;
        }
        return true;
    }

    SessionData newSessionData;
    newSessionData.sourceip_ = sourceip;
    newSessionData.desip_ = desip;
    newSessionData.session_id_ = session_id;
    newSessionData.is_inbound_ = is_inbound;
    if (!is_inbound)
    {
        if (!noticePassiveSession(newSessionData, session_id))
        {
            logError("Failed to open session!.");
            return false;
        }
    }
    SessionkeyCache_[session_id] = std::move(newSessionData);
    ++session_number;
    return true;
}

bool SessionManager::noticePassiveSession(SessionData &data, uint32_t session_id)
{
    data.fd_ = connectToServer(uint32ToIpString(data.desip_), LISTEN_PORT);
    if (data.fd_ == -1)
    {
        logError("Connection failed.");
        return false;
    }

    OpenSessionPacket pkt1;
    pkt1.constructopensessionpacket(data.sourceip_, data.desip_, session_id, true);
    if (send(data.fd_, pkt1.getBufferPtr(), pkt1.getBufferSize(), 0) == -1)
    {
        logError("Failed to send open session packet.");
        close(data.fd_);
        return false;
    }
    // 只要send成功被动端一定会正确处理打开会话，故返回true
    return true;
}

std::string SessionManager::getSessionKey(uint32_t session_id, uint32_t request_id, uint16_t request_len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        while (it->second.keyValue.size() - it->second.index_ < request_len)
        {
            if (it->second.is_inbound_ || !addKeyToSession(it->second, true))
            {
                logError("Insufficient key materials.");
                return "";
            }
        }
        auto start = it->second.keyValue.begin() + it->second.index_;
        std::string returnKey(start, start + request_len);
        it->second.index_ += request_len;
        return returnKey;
    }
    logError("Session not found.");
    return "";
}

bool SessionManager::closeSession(uint32_t session_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        SessionkeyCache_.erase(it);
        --session_number;
        logInfo("Session closed successfully: " + std::to_string(session_id));
        return true;
    }
    logError("Close session failed: Unable to find session " + std::to_string(session_id));
    return false;
}