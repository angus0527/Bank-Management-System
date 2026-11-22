#include "../include/User.h"

// 密码加密
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

// 验证密码
bool User::verifyPassword(const std::string &plainPassword) const {
    return this->password == encryptPassword(plainPassword);
}

// 修改密码
void User::changePassword(const std::string &newPlainPassword) {
    this->password = encryptPassword(newPlainPassword);
}

Admin::Admin(long long id, const std::string &name, const std::string &password)
    : User(id, name, password) {
}

// 转为string保存data
std::string Admin::serialize() const {
    return "U|" + std::to_string(id) + "|A|" + name + "|" + password + "|0000";
}

Customer::Customer(long long id, const std::string &name, const std::string &password, const std::string &phoneNumber)
    : User(id, name, password), phoneNumber(phoneNumber) {
}

// 转为string保存data
std::string Customer::serialize() const {
    return "U|" + std::to_string(id) + "|C|" + name + "|" + password + "|" + phoneNumber;
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
