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
    // ��ʶ�������˻��Ǳ�����,false�����������
    bool is_inbound_;
    // ������Ա�����ڴ洢ʹ�ù���seq�Ķ���
    std::queue<int> usedSeq;
};

class SessionManager
{
public:
    // ���캯��
    SessionManager();

    // �½��Ự
    bool addSession(uint32_t sourceip, uint32_t desip, uint32_t session_id,bool is_inbound);

    // ֪ͨ�����˴����Ự
    bool noticePassiveSession(SessionData &data, uint32_t session_id);

    //�����˱�����ȡ��Կ
    bool addPassiveKey(uint32_t  session_id,uint32_t key_seqnum);

    // ��ȡ��Կ��ͨ��request��ȡ
    std::string getKey(uint32_t session_id, uint32_t request_id, uint16_t request_len);

    // ɾ���Ự��ͨ��session_idɾ��
    bool closeSession(uint32_t session_id);

private:
    std::mutex mutex_;
    // ����session_id��KeyData��ӳ��
    std::unordered_map<uint32_t, SessionData> SessionkeyCache_;
    // ����ȫ�ֻỰ����
    int session_number;
};



#endif // SESSIONMANAGEMENT_HPP