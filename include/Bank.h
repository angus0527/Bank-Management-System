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
    long long nextUserId = 20001;
    long long nextAccountId = 62001;
    long long nextTransactionId = 1;

    // 基准利率配置（直接用小数表示，如 0.0035 表示 0.35%）
    double currentRate = 0.0035;     // 活期年利率 0.35%
    double fixed3MRate = 0.0125;     // 定期3个月 1.25%
    double fixed1YRate = 0.0175;     // 定期1年   1.75%
    double fixed3YRate = 0.0250;     // 定期3年   2.50%
    double creditDailyRate = 0.0005; // 信用日利率 0.05%

    Customer *findCustomer(long long ownerId);

    void recordTx(TransactionType type, long long fromId, long long toId, double amount,
                  double balanceSnapshot, const std::string &remark);

public:
    // 登录、注册与个人信息管理
    User *login(long long userId, const std::string &password);
    long long registerCustomer(const std::string &name, const std::string &password, const std::string &phone);
    bool updateUserInfo(long long userId, const std::string &newName, const std::string &newPhone = "");
    bool changeUserPassword(long long userId, const std::string &oldPassword, const std::string &newPassword);

    // 三类账户开户（成功返回新账号，失败返回 -1）
    long long openAccount(long long ownerId, double initialDeposit, const Date &openDate = Date::today());
    long long openFixedAccount(long long ownerId, double initialDeposit, int termMonths,
                               bool autoRenew = false, const Date &openDate = Date::today());
    long long openCreditAccount(long long ownerId, double creditLimit, const Date &openDate = Date::today());

    // 核心交易业务
    bool deposit(long long accountId, double amount, const std::string &remark = "");
    bool withdraw(long long accountId, double amount, const std::string &remark = "");
    double withdrawFixed(long long accountId, const Date &today = Date::today());
    bool transfer(long long fromAccountId, long long toAccountId, double amount,
                  const std::string &remark = "");
    double settleInterest(long long accountId, const Date &today = Date::today());
    int processMaturityRollovers(const Date &today = Date::today());

    // 管理员风控与利率调节
    bool freezeAccount(long long accountId);
    bool unfreezeAccount(long long accountId);
    bool closeAccount(long long accountId);

    bool setCurrentRate(double rate);
    bool setFixedRate(int termMonths, double rate);
    bool setCreditDailyRate(double rate);
    double getCurrentRate() const { return currentRate; }
    double getFixedRate(int termMonths) const;
    double getCreditDailyRate() const { return creditDailyRate; }
    void initRates(double cur, double f3m, double f1y, double f3y, double cr);

    // 查询流水（支持按账号、类型 1~4、日期范围 YYYY-MM-DD 筛选）
    std::vector<const Transaction *> queryTransactions(long long accountId = 0,
                                                       int typeFilter = 0,
                                                       const std::string &startDate = "",
                                                       const std::string &endDate = "") const;

    Account *findAccount(long long accountId);
    User *findUser(long long userId);

    // storage 读写辅助接口
    bool addUser(std::unique_ptr<User> user);
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
