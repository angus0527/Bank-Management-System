#include "../include/User.h"

static std::string obfuscatePassword(const std::string &plain) {
    std::string result = plain;
    for (char &c: result) {
        c ^= 0x5A;
    }
    return result;
}

User::User(long long id, const std::string &name, const std::string &password,
           bool isAlreadyObfuscated): id(id), name(name) {
    if (isAlreadyObfuscated) {
        this->password = password;
    } else {
        this->password = obfuscatePassword(password);
    }
};

bool User::verifyPassword(const std::string &password) const {
    return this->password == obfuscatePassword(password);
}

void User::changePassword(const std::string &newPassword) {
    this->password = obfuscatePassword(newPassword);
}

Admin::Admin(long long id, const std::string &name, const std::string &password, bool isAlreadyObfuscated) : User(
    id, name, password, isAlreadyObfuscated) {
}

Customer::Customer(long long id, const std::string &name, const std::string &password, const std::string &phoneNumber,
                   bool isAlreadyObfuscated): User(id, name, password, isAlreadyObfuscated), phoneNumber(phoneNumber) {
}

bool Customer::removeAccount(long long id) {
    for (auto it = accountId.begin(); it != accountId.end(); it++) {
        if (*it == id) {
            accountId.erase(it);
            return true;
        }
    }
    return false;
}
