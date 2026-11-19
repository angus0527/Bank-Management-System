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

// Bank 门面类：统一封装登录、开户、存取转、结息、风控与流水查询接口
class Bank {
    std::map<long long, std::unique_ptr<User>> users;
    std::map<long long, std::unique_ptr<Account>> accounts;
    std::vector<Transaction> transactions;
    long long nextAccountId = 62001;
    long long nextTransactionId = 1;

    // 基准利率配置（万分之一为单位，支持管理员调整）
    long long currentRateX10000 = 35;      // 活期年利率 0.35%
    long long fixed3MRateX10000 = 125;     // 定期3个月 1.25%
    long long fixed1YRateX10000 = 175;     // 定期1年   1.75%
    long long fixed3YRateX10000 = 250;     // 定期3年   2.50%
    long long creditDailyRateX10000 = 5;   // 信用日利率 0.05%

    void recordTx(TransactionType type, long long fromId, long long toId, long long amount,
                  long long balanceSnapshot, const std::string &remark);

public:
    // 登录与个人信息管理
    User *login(long long userId, const std::string &password);
    void updateUserInfo(long long userId, const std::string &newName, const std::string &newPhone = "");
    void changeUserPassword(long long userId, const std::string &oldPassword, const std::string &newPassword);

    // 三类账户开户
    long long openAccount(long long ownerId, long long initialDeposit, const Date &openDate = Date::today());
    long long openFixedAccount(long long ownerId, long long initialDeposit, int termMonths,
                               bool autoRenew = false, const Date &openDate = Date::today());
    long long openCreditAccount(long long ownerId, long long creditLimit, const Date &openDate = Date::today());

    // 核心交易业务
    void deposit(long long accountId, long long amount, const std::string &remark = "");
    void withdraw(long long accountId, long long amount, const std::string &remark = "");
    long long withdrawFixed(long long accountId, const Date &today = Date::today());
    void transfer(long long fromAccountId, long long toAccountId, long long amount,
                  const std::string &remark = "");
    long long settleInterest(long long accountId, const Date &today = Date::today());
    int processMaturityRollovers(const Date &today = Date::today());

    // 管理员风控与利率调节
    void freezeAccount(long long accountId);
    void unfreezeAccount(long long accountId);
    void closeAccount(long long accountId);

    void setCurrentRate(long long rateX10000);
    void setFixedRate(int termMonths, long long rateX10000);
    void setCreditDailyRate(long long rateX10000);
    long long getCurrentRateX10000() const { return currentRateX10000; }
    long long getFixedRateX10000(int termMonths) const;
    long long getCreditDailyRateX10000() const { return creditDailyRateX10000; }
    void initRates(long long cur, long long f3m, long long f1y, long long f3y, long long cr);

    // 查询流水（支持按账号、类型 1~4、日期范围 YYYY-MM-DD 筛选）
    std::vector<const Transaction *> queryTransactions(long long accountId = 0,
                                                       int typeFilter = 0,
                                                       const std::string &startDate = "",
                                                       const std::string &endDate = "") const;

    Account *findAccount(long long accountId);
    User *findUser(long long userId);

    // storage 读写辅助接口
    void addUser(std::unique_ptr<User> user);
    void addAccount(std::unique_ptr<Account> account);
    void addTransaction(const Transaction &t);
    void setNextAccountId(long long id);
    void setNextTransactionId(long long id);

    long long getNextAccountId() const { return nextAccountId; }
    long long getNextTransactionId() const { return nextTransactionId; }
    const std::map<long long, std::unique_ptr<Account>> &getAccounts() const { return accounts; }
    const std::map<long long, std::unique_ptr<User>> &getUsers() const { return users; }
    const std::vector<Transaction> &getTransactions() const { return transactions; }
};

#endif // BANK_H
