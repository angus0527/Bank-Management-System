#ifndef USER_H
#define USER_H

#include <string>
#include <vector>


class User {
protected:
    long long id;           // 用户id
    std::string name;       // 用户姓名
    std::string password;   // 用户密码

public:
    virtual ~User() {}

    User(long long id, const std::string &name, const std::string &password);

    // 密码加密
    static std::string encryptPassword(const std::string &plain);

    // 验证密码
    bool verifyPassword(const std::string &password) const;

    // 修改密码
    void changePassword(const std::string &newPassword);

    //返回Admin或者Customer
    virtual std::string role() const = 0;

    // 转为string保存data
    virtual std::string serialize() const = 0;

    std::string getName() const {
        return name;
    }

    void setName(const std::string &newName) {
        name = newName;
    }

    long long getId() const {
        return id;
    }

    std::string getPassword() const {
        return password;
    }
};

class Admin : public User {
public:
    Admin(long long id, const std::string &name, const std::string &password);

    std::string role() const override {
        return "Admin";
    }

    std::string serialize() const override;
};

class Customer : public User {
    std::string phoneNumber;             // 电话
    std::vector<long long> accountId;    // 名下的账户列表

public:
    Customer(long long id, const std::string &name, const std::string &password, const std::string &phoneNumber);

    std::string role() const override {
        return "Customer";
    }

    std::string serialize() const override;

    std::string getPhoneNumber() const {
        return phoneNumber;
    }

    void setPhoneNumber(const std::string &newPhone) {
        phoneNumber = newPhone;
    }

    void addAccount(long long id) {
        accountId.push_back(id);
    }

    bool removeAccount(long long id);

    const std::vector<long long> &getAccountIds() const {
        return accountId;
    }
};

#endif // USER_H
