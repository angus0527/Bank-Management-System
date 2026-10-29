#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <iostream>
#include <string>

#include "Date.h"
#include "exception.h"

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

// 金额（分）→ "1234.56" 显示字符串（README §2 约定 1：金额一律用 long long 存"分"）
std::string formatMoney(long long cents);

// 账户抽象基类（README §4.1）
// 金额单位一律为"分"；业务失败不返回 bool，一律抛 exception.h 中的异常（约定 4）
class Account {
protected:
    long long accountId;   // 账号（与 User 体系的 long long id 统一）
    long long ownerId;     // 所属用户 id
    long long balance;     // 余额（分）
    AccountStatus status;  // 状态: 正常/冻结/销户
    Date openDate;         // 开户日期

public:
    Account(long long id, long long ownerId, long long initialBalance, const Date &openDate);

    virtual ~Account() = default;

    // 存款/取款：金额非法抛 InvalidAmount，账户冻结/销户抛 AccountFrozen，
    // 余额不足抛 InsufficientBalance（信用账户第 4 周重写 withdraw 支持透支）
    virtual void deposit(long long amount);

    virtual void withdraw(long long amount);

    // 计算截至 today 应计利息（分），不改余额；各账户类型多态实现（计划最大展示点）
    virtual long long calcInterest(const Date &today) const = 0;

    // 结息：把应计利息并入余额，返回本次入账利息（分）
    virtual long long settleInterest(const Date &today) = 0;

    virtual void displayInfo() const;

    virtual AccountType getAccountType() const = 0;

    // 将账户完整属性格式化为单行文本，用于保存到 accounts.txt（README §5 格式）
    virtual std::string serialize() const = 0;

    void freeze();

    void unfreeze();

    // 销户（F03：余额不为 0 抛 InvalidAmount，禁止销户）
    void closeAccount();

    long long getAccountId() const { return accountId; }

    long long getOwnerId() const { return ownerId; }

    long long getBalance() const { return balance; }

    AccountStatus getStatus() const { return status; }

    const Date &getOpenDate() const { return openDate; }

    std::string getStatusString() const;

    bool isNormal() const { return status == AccountStatus::NORMAL; }

    // 供 storage 层读档与管理员操作使用
    void setStatus(AccountStatus newStatus) { status = newStatus; }

    void setBalance(long long newBalance) { balance = newBalance; }
};

#endif //ACCOUNT_H
