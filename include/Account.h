#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <iostream>
#include <string>

#include "Date.h"

enum class AccountStatus {
    NORMAL = 1,
    FROZEN = 2,
    CLOSED = 3
};

enum class AccountType {
    SAVINGS = 1, // 储蓄卡
    CREDIT = 2,  // 信用卡
    FIXED = 3    // 定期
};

// 保留两位小数的辅助函数
double round2(double value);

// 金额to string
std::string formatMoney(double yuan);

class Account {
protected:
    long long accountId;   // 账号
    long long ownerId;     // 用户 id
    double balance;        // 余额（元）
    AccountStatus status;  // 状态
    Date openDate;         // 开户日期

public:
    Account(long long id, long long ownerId, double initialBalance, const Date &openDate);

    virtual ~Account() = default;

    // 存款
    virtual bool deposit(double amount);
    //取款
    virtual bool withdraw(double amount);

    // 计算到 today 利息
    virtual double calcInterest(const Date &today) const = 0;

    // 结息，把利息计入余额，调用上面的calcInterest
    virtual double settleInterest(const Date &today) = 0;

    //控制台展示信息
    virtual void displayInfo() const;


    virtual AccountType getAccountType() const = 0;

    // 将账户信息转为单行的string，保存到 accounts.txt
    virtual std::string serialize() const = 0;

    bool freeze();

    bool unfreeze();

    // 销户
    bool closeAccount();

    long long getAccountId() const { return accountId; }

    long long getOwnerId() const { return ownerId; }

    double getBalance() const { return balance; }

    AccountStatus getStatus() const { return status; }

    const Date &getOpenDate() const { return openDate; }

    std::string getStatusString() const;

    bool isNormal() const { return status == AccountStatus::NORMAL; }

    void setStatus(AccountStatus newStatus) { status = newStatus; }

    void setBalance(double newBalance) { balance = round2(newBalance); }
};

#endif // ACCOUNT_H
