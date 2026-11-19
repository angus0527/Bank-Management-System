#include "../include/User.h"

// 对密码逐字节与固定掩码 0x5A 做异或混淆
// 利用异或的自反性（(P ^ K) ^ K == P），避免密码以明文形式裸露在内存和文本文件中
static std::string obfuscatePassword(const std::string &plain) {
    std::string result = plain;
    for (char &c : result) {
        c ^= 0x5A;
    }
    return result;
}

// 基类构造：按 isAlreadyObfuscated 标记区分“首次创建（需要混淆）”和“文件读档（直接存储密文）”
User::User(long long id, const std::string &name, const std::string &password,
           bool isAlreadyObfuscated) : id(id), name(name) {
    if (isAlreadyObfuscated) {
        this->password = password;
    } else {
        this->password = obfuscatePassword(password);
    }
}

// 将外部输入的明文同样混淆后，与内部保存的密文做字符串比对
bool User::verifyPassword(const std::string &password) const {
    return this->password == obfuscatePassword(password);
}

// 传入新的明文密码，混淆后覆盖旧密码
void User::changePassword(const std::string &newPassword) {
    this->password = obfuscatePassword(newPassword);
}

Admin::Admin(long long id, const std::string &name, const std::string &password, bool isAlreadyObfuscated)
    : User(id, name, password, isAlreadyObfuscated) {
}

Customer::Customer(long long id, const std::string &name, const std::string &password, const std::string &phoneNumber,
                   bool isAlreadyObfuscated)
    : User(id, name, password, isAlreadyObfuscated), phoneNumber(phoneNumber) {
}

// 顺序查找客户持卡列表，命中目标账号后将其擦除，成功返回 true
bool Customer::removeAccount(long long id) {
    for (auto it = accountId.begin(); it != accountId.end(); it++) {
        if (*it == id) {
            accountId.erase(it);
            return true;
        }
    }
    return false;
}
