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
    // ���캯��
    KeyManager();

    // �����Կ, ����Կ�洢�ɱ�����Կ�ļ�
    void addKey(int seq, const uint8_t *keyValue, size_t keySize);

    // ��ȡ��Կ��ͨ��SEQ��ȡ
    std::string getKey(int seq);

    // ��ȡ��ǰ���õ���С��Կ��ţ������ȡ��Կ
    int getKeyinOrder(bool odd_number);

    // ɾ����Կ��ͨ��SEQɾ��
    void removeKey(int seq);

    //��Կ���ʼ��
    void monitorKeyRate();

    void run(Server &externalserver, const std::string &keysupply_ipAddress, int keysuply_port);

private:
    std::mutex mutex_;
    // ��ǰ��Կ�ص���Կ�ļ�����
    int count;
    // ��ǰ����˳�����С������Կ���
    int seq_odd_inorder_;
    // ��ǰ����˳�����Сż����Կ���
    int seq_even_inorder_;
    
    std::map<int, std::string> keyMap_; // ���ڴ洢��Կ,��Կ����char buffer[]��vector<unint_8>
};



#endif