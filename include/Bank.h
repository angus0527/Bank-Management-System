//
// Created by angus on 2026/10/29.
//

#ifndef BANK_H
#define BANK_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Account.h"
#include "Date.h"
#include "Transaction.h"
#include "User.h"

// Bank 门面类：全部业务入口集中于此（README §2 约定 3），
// UI/控制台层只调用 Bank 的公共接口，不直接碰 Account/User 内部字段。
// core 层不依赖 storage/Qt：持久化由 Storage::load/save（B）负责，
// 操作日志由调用方（UI/控制台）通过 Logger 记录。
class Bank {
    std::map<long long, std::unique_ptr<User>> users;
    std::map<long long, std::unique_ptr<Account>> accounts;
    std::vector<Transaction> transactions;
    long long nextAccountId = 62001;     // 账号从 62001 起（README §5 示例）
    long long nextTransactionId = 1;

    // 追加一条流水（余额快照由调用方在交易完成后传入）
    void recordTx(TransactionType type, long long fromId, long long toId, long long amount,
                  long long balanceSnapshot, const std::string &remark);

public:
    // —— 鉴权（F01）——
    // 用户不存在或密码错误均抛 AuthFail（不区分提示，防试探）
    User *login(long long userId, const std::string &password);

    // —— 账户业务（F02~F09）——
    // 开户：owner 必须是 Customer；initialDeposit ≥ 0；返回新账号
    long long openAccount(long long ownerId, long long initialDeposit,
                          const Date &openDate = Date::today());

    void deposit(long long accountId, long long amount, const std::string &remark = "");

    void withdraw(long long accountId, long long amount, const std::string &remark = "");

    // 转账：先完成全部校验再变更余额，保证原子性（任一校验失败双方余额都不动）
    void transfer(long long fromAccountId, long long toAccountId, long long amount,
                  const std::string &remark = "");

    // 手动结息（F09），返回本次入账利息（分）；未到结息日返回 0
    long long settleInterest(long long accountId);

    // 管理员操作（Y01 预留）
    void freezeAccount(long long accountId);

    void unfreezeAccount(long long accountId);

    // 销户（F03）：余额不为 0 抛 InvalidAmount
    void closeAccount(long long accountId);

    // —— 查询（F08）——
    // accountId 为 0 时返回全部流水；否则返回与该账户相关的流水（按时间顺序）
    std::vector<const Transaction *> queryTransactions(long long accountId = 0) const;

    // —— 供 UI/控制台展示 ——
    Account *findAccount(long long accountId);       // 不存在抛 AccountNotFound
    User *findUser(long long userId);                // 不存在抛 AccountNotFound

    // —— storage 层装载/读取接口（B 的 TextFileStorage 使用）——
    void addUser(std::unique_ptr<User> user);        // 装载已有用户（id 冲突抛 InvalidAmount）
    void addAccount(std::unique_ptr<Account> account); // 直接装载，不走业务校验
    void addTransaction(const Transaction &t);
    void setNextAccountId(long long id);
    void setNextTransactionId(long long id);

    long long getNextAccountId() const {
        return nextAccountId;
    }

    long long getNextTransactionId() const {
        return nextTransactionId;
    }

    const std::map<long long, std::unique_ptr<Account>> &getAccounts() const {
        return accounts;
    }

    const std::map<long long, std::unique_ptr<User>> &getUsers() const {
        return users;
    }

    const std::vector<Transaction> &getTransactions() const {
        return transactions;
    }
};

#endif //BANK_H
