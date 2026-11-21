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
    SAVINGS = 1, // 活期储蓄账户
    CREDIT = 2,  // 信用卡/透支账户
    FIXED = 3    // 定期存款账户
};

// 四舍五入保留两位小数（到分）
double round2(double value);

// 金额（元）→ "1234.56" 显示字符串
std::string formatMoney(double yuan);

// 账户抽象基类（金额单位统一为“元”）
class Account {
protected:
    long long accountId;   // 账号
    long long ownerId;     // 所属用户 id
    double balance;        // 余额（元）
    AccountStatus status;  // 状态: 正常/冻结/销户
    Date openDate;         // 开户日期

public:
    Account(long long id, long long ownerId, double initialBalance, const Date &openDate);

    virtual ~Account() = default;

    // 存款/取款：成功返回 true，失败输出提示并返回 false
    virtual bool deposit(double amount);

    virtual bool withdraw(double amount);

    // 计算截至 today 应计利息（元），不改余额；各账户类型多态实现
    virtual double calcInterest(const Date &today) const = 0;

    // 结息：把应计利息并入余额，返回本次利息（元）
    virtual double settleInterest(const Date &today) = 0;

    virtual void displayInfo() const;

    virtual AccountType getAccountType() const = 0;

    // 将账户完整属性格式化为单行文本，用于保存到 accounts.txt
    virtual std::string serialize() const = 0;

    bool freeze();

    bool unfreeze();

    // 销户（要求余额为 0）
    bool closeAccount();

    long long getAccountId() const { return accountId; }

    long long getOwnerId() const { return ownerId; }

    double getBalance() const { return balance; }

    AccountStatus getStatus() const { return status; }

    const Date &getOpenDate() const { return openDate; }

    std::string getStatusString() const;

    bool isNormal() const { return status == AccountStatus::NORMAL; }

    // 供 storage 层读档与内部更新使用
    void setStatus(AccountStatus newStatus) { status = newStatus; }

    void setBalance(double newBalance) { balance = round2(newBalance); }
};

#endif // ACCOUNT_H
