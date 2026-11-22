#include "../include/User.h"

// 密码加密
std::string User::encryptPassword(const std::string &plain) {
    std::string result = plain;
    for (char &c : result) {
        c ^= 0x5A;
    }
    return result;
}

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

// U(用户)|id|A(管理员)|name|password|0000(电话占位)
std::string Admin::serialize() const {
    return "U|" + std::to_string(id) + "|A|" + name + "|" + password + "|0000";
}

Customer::Customer(long long id, const std::string &name, const std::string &password, const std::string &phoneNumber)
    : User(id, name, password), phoneNumber(phoneNumber) {
}

// U(用户)|id|C(普通客户)|name|password|phoneNumber
std::string Customer::serialize() const {
    return "U|" + std::to_string(id) + "|C|" + name + "|" + password + "|" + phoneNumber;
}

//删除账户
bool Customer::removeAccount(long long id) {
    for (auto it = accountId.begin(); it != accountId.end(); it++) {
        if (*it == id) {
            accountId.erase(it);
            return true;
        }
    }
    return false;
}
