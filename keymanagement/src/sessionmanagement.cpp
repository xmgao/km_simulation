#include "sessionmanagement.hpp"
#include "server.hpp"
#include "opensessionpacket.hpp"
#include "debuglevel.hpp"

extern KeyManager globalKeyManager;
// ����ȫ�ֱ�������������
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
    // ʾ��������Կ���߼�
    bool is_odd = data.sourceip_ > data.desip_ ? true : false;
    int seq = globalKeyManager.getKeyinOrder(is_odd);
    if (seq == -1)
    {
        // ������
        std::cerr << "failed to find useful seq." << std::endl;
        return false;
    }
    data.usedSeq.push(seq);
    std::string keyValue1 = globalKeyManager.getKey(seq);
    // ����ɾ����Կ
    globalKeyManager.removeKey(seq);
    // �� keyValue ���� SessionkeyCache_[i] ��ĩβ
    data.keyValue.insert(data.keyValue.end(), keyValue1.begin(), keyValue1.end());
    //��Կͬ��
    keysync(data,seq);
    return true;
}

bool SessionManager::addPassiveKey(uint32_t session_id, uint32_t key_seqnum)
{
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        // �����ҵ�����Կ
        std::string keyValue1 = globalKeyManager.getKey(key_seqnum);
        // ����ɾ����Կ
        globalKeyManager.removeKey(key_seqnum);
        it->second.usedSeq.push(key_seqnum);
        // �� keyValue ���� SessionkeyCache_[i] ��ĩβ
        it->second.keyValue.insert(it->second.keyValue.end(), keyValue1.begin(), keyValue1.end());
        return true;
    }
    else
    {
        // ������
        std::cerr << "failed to find session:" << session_id << std::endl;
        return false;
    }
}



bool SessionManager::addSession(uint32_t sourceip, uint32_t desip, uint32_t session_id, bool is_outbound)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // ���session_id�Ƿ��Ѵ���
    auto it = SessionkeyCache_.find(session_id);
    if (it != SessionkeyCache_.end())
    {
        if (it->second.is_outbound_)
        {
            std::cerr << "session already exists" << std::endl;
            return false; // �����˻Ự�Ѵ���
        }
        else
        {
            return true; // �����˻Ự��������������KM����
        }
    }
    // �����µ�KeyData���󲢲���ӳ��
    SessionData newSessionData;
    newSessionData.sourceip_ = sourceip;
    newSessionData.desip_ = desip;
    newSessionData.session_id_ = session_id;
    // �ж��Ƿ���������
    if (is_outbound)
    {
        newSessionData.is_outbound_ = true;

        // ֪ͨ�����˴����Ự
        noticePassiveSession(newSessionData, session_id);
        // ��ǰ����һ����Կ,�����˲���Ҫ��ǰ����Ҫ��������ͬ��
        if (!addProactiveKey(newSessionData))
        {
            // ������
            std::cerr << "generatekey failed." << std::endl;
            return false;
        } // ������һ��������Կ�ĺ���
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
        // �����ҵ�����Կ

        int useful_size = it->second.keyValue.size() - it->second.index_;
        while (useful_size < request_len)
        {
            if (it->second.is_outbound_)
            {
                if (!addProactiveKey(it->second))
                {
                    // ������
                    std::cerr << "addproactivekey failed." << std::endl;
                    return "";
                } // �����˲�����Կ
            }
            else
            {
                return "";
                // �����˷��ؿ���Կ
            }
            useful_size = it->second.keyValue.size() - it->second.index_;
        }
        std::string returnkeyvalue(it->second.keyValue.begin() + it->second.index_, it->second.keyValue.begin() + it->second.index_ + request_len);
        it->second.index_ += request_len;
        return returnkeyvalue;
    }
    return ""; // ���δ�ҵ������ؿ��ַ���
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
        // ������
        std::cerr << "closeSession failed!Unable to find session" << std::endl;
    }
    return false;
}
