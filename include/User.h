#ifndef USER_H
#define USER_H

#include <string>
#include <vector>

// 用户抽象基类：封装全行用户的公共身份属性与密码鉴权逻辑
// 派生出 Admin（管理员）与 Customer（普通储户）两个子类，通过纯虚函数 role() 实现权限多态
class User {
protected:
    long long id;           // 用户唯一编号（管理员 10001 起，储户 20001 起）
    std::string name;       // 用户姓名
    std::string password;   // 混淆后的登录密文（不存明文）

public:
    // 虚析构函数：保证通过基类指针释放派生类对象时正确触发子类析构，避免内存泄漏
    virtual ~User() {}

    // 构造函数：直接保存已混淆加密后的密码密文
    User(long long id, const std::string &name, const std::string &password);

    // 将明文密码做异或混淆，得到存储用密文
    static std::string encryptPassword(const std::string &plain);

    // 验证输入明文密码与内部存储的密文是否匹配
    bool verifyPassword(const std::string &password) const;

    // 修改登录密码（内部会自动完成混淆后再保存）
    void changePassword(const std::string &newPassword);

    // 纯虚函数：返回当前用户的身份角色标识（"Admin" 或 "Customer"）
    virtual std::string role() const = 0;

    // 纯虚函数：将用户信息格式化为单行文本，用于保存到 users.txt
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

// 系统管理员类（继承自 User）：具备全行账户风控、利率调控及日志审计等后台权限
class Admin : public User {
public:
    Admin(long long id, const std::string &name, const std::string &password);

    std::string role() const override {
        return "Admin";
    }

    std::string serialize() const override;
};

// 普通储户类（继承自 User）：在基类基础上扩展联系电话及名下持有的银行卡号列表（1:N 聚合关联）
class Customer : public User {
    std::string phoneNumber;             // 绑定联系电话
    std::vector<long long> accountId;    // 名下绑定的银行账号列表

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

    // 开户时将新账号绑定到该客户名下
    void addAccount(long long id) {
        accountId.push_back(id);
    }

    // 销户时从客户名下解绑并移除指定账号；找不到返回 false
    bool removeAccount(long long id);

    const std::vector<long long> &getAccountIds() const {
        return accountId;
    }
};

#endif // USER_H
