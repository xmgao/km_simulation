#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#include <string>
#include <stdexcept>

class Encryptor {
public:
    // ʹ�ø�����Կ�����Ľ��м��ܣ���������
    static bool encrypt(const std::string& plaintext, const std::string& key, std::string& ciphertext);

    // ʹ�ø�����Կ�����Ľ��н��ܣ���������
    static bool decrypt(const std::string& ciphertext, const std::string& key, std::string& plaintext);
};

#endif // ENCRYPTOR_H