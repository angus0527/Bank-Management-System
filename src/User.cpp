#include "../include/User.h"

// 对密码逐字节与固定掩码 0x5A 做异或混淆，避免明文密码直接存盘
std::string User::encryptPassword(const std::string &plain) {
    std::string result = plain;
    for (char &c : result) {
        c ^= 0x5A;
    }
    return result;
}

// 基类构造：统一接收并保存已加密的密码密文
User::User(long long id, const std::string &name, const std::string &password)
    : id(id), name(name), password(password) {
}

// 将输入的明文同样混淆后，与内部保存的密文比对
bool User::verifyPassword(const std::string &plainPassword) const {
    return this->password == encryptPassword(plainPassword);
}

// 修改密码：把新明文混淆后覆盖保存
void User::changePassword(const std::string &newPlainPassword) {
    this->password = encryptPassword(newPlainPassword);
}

Admin::Admin(long long id, const std::string &name, const std::string &password)
    : User(id, name, password) {
}

Customer::Customer(long long id, const std::string &name, const std::string &password, const std::string &phoneNumber)
    : User(id, name, password), phoneNumber(phoneNumber) {
}

// 顺序查找客户持卡列表，命中目标账号后将其移除，成功返回 true
bool Customer::removeAccount(long long id) {
    for (auto it = accountId.begin(); it != accountId.end(); it++) {
        if (*it == id) {
            accountId.erase(it);
            return true;
        }
    }
    return false;
}
