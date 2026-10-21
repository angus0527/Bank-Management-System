#ifndef ACCOUNT_H
#define ACCOUNT_H
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

enum class AccountStatus {
    NORMAL = 1,
    FROZEN = 2,
    CLOSED = 3
};

enum class AccountType {
    SAVINGS = 1, // 活期储蓄账户
    CREDIT = 2,  // 信用卡/透支账户
    FIXED = 3    // 定期存款账户
};

class Account {
protected:
    std::string accountId; // 账号
    std::string ownerId; // 所属用户
    double balance; // 余额
    AccountStatus status; // 状态: 正常/冻结/销户
    std::string openDate; // 开户日期

public:
    Account(const std::string& id, const std::string& owner, double initialBalance, const std::string& date);
    virtual ~Account();

    virtual bool withdraw(double amount) = 0;

    virtual bool deposit(double amount) = 0;

    virtual double calculateInterest() = 0;

    virtual void settlePeriod() = 0;

    virtual void displayInfo() const;

    virtual AccountType getAccountType() const = 0;

    //将账户完整属性格式化为单行文本字符串，用于保存到 accounts.txt
    virtual std::string serialize() const = 0;

    void freeze();

    void unfreeze();

    bool closeAccount();

    std::string getAccountId() const { return accountId; }

    std::string getOwnerId() const { return ownerId; }

    double getBalance() const { return balance; }

    AccountStatus getStatus() const { return status; }

    std::string getOpenDate() const { return openDate; }

    std::string getStatusString() const;

    void setBalance(double newBalance) { balance = newBalance; }
};
#endif //ACCOUNT_H