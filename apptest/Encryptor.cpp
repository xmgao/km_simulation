#include "Encryptor.hpp"
#include <iostream>

bool Encryptor::encrypt(const std::string& plaintext, const std::string& key, std::string& ciphertext) {
    // �����Կ�����Ƿ����������ĳ������
    if (key.size() < plaintext.size()) {
        return false; // ��Կ���Ȳ���
    }

    ciphertext.resize(plaintext.size());

    // ���ֽ�������
    for (size_t i = 0; i < plaintext.size(); ++i) {
        ciphertext[i] = plaintext[i] ^ key[i];
    }

    return true;
}

bool Encryptor::decrypt(const std::string& ciphertext, const std::string& key, std::string& plaintext) {
    // �����Կ�����Ƿ����������ĳ������
    if (key.size() < ciphertext.size()) {
        return false; // ��Կ���Ȳ���
    }

    plaintext.resize(ciphertext.size());

    // ���ֽ�������
    for (size_t i = 0; i < ciphertext.size(); ++i) {
        plaintext[i] = ciphertext[i] ^ key[i];
    }

    return true;
}

int encryptortest() {
    std::string plaintext = "This is a secret message.";
    std::string key = "XQbSZCGMQZZyBqnmekEXfjJzyJw"; // ������Կ�������������ĳ�����ͬ��

    std::string ciphertext;
    std::string decryptedtext;

    // ����
    if (Encryptor::encrypt(plaintext, key, ciphertext)) {
        std::cout << "Ciphertext: ";
        for (const auto &ch : ciphertext) {
            std::cout << std::hex << (int)ch << " ";
        }
        std::cout << std::dec << std::endl; // �ָ�ʮ���Ƹ�ʽ
    } else {
        std::cerr << "Encryption failed due to insufficient key length." << std::endl;
    }

    // ����
    if (Encryptor::decrypt(ciphertext, key, decryptedtext)) {
        std::cout << "Decrypted text: " << decryptedtext << std::endl;
    } else {
        std::cerr << "Decryption failed due to insufficient key length." << std::endl;
    }

    return 0;
}