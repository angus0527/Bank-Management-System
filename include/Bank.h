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

//统领整个程序的对象
class Bank {
    std::map<long long, std::unique_ptr<User>> users; //<uid,用户指针> 存所有的用户
    std::map<long long, std::unique_ptr<Account>> accounts; //<aid,账户指针> 存所有的账户
    std::vector<Transaction> transactions; //存所有的流水
    long long nextUserId = 20001;//为用户分配的id
    long long nextTransactionId = 1;//为流水分配的id

    double currentRate = 0.0035;     // 活期年利率 0.35%
    double fixed3MRate = 0.0125;     // 定期3个月 1.25%
    double fixed1YRate = 0.0175;     // 定期1年   1.75%
    double fixed3YRate = 0.0250;     // 定期3年   2.50%
    double creditDailyRate = 0.0005; // 信用日利率 0.05%

    Customer *findCustomer(long long ownerId);

    // 生成16位银行卡号
    long long generateAccountId() const;

    // 记一笔流水
    void recordTx(TransactionType type, long long fromId, long long toId, double amount,
                  double balanceSnapshot, const std::string &remark);

public:
    //登录
    User *login(long long userId, const std::string &password);
    //注册
    long long registerCustomer(const std::string &name, const std::string &password, const std::string &phone);
    // 修改姓名
    bool updateUserName(long long userId, const std::string &newName);
    // 修改电话
    bool updateCustomerPhone(long long userId, const std::string &newPhone);
    //改密码
    bool changeUserPassword(long long userId, const std::string &oldPassword, const std::string &newPassword);

    //活期账户开户
    long long openCurrentAccount(long long ownerId, double initialDeposit, const Date &openDate = Date::today());

    //定期账户开户
    long long openFixedAccount(long long ownerId, double initialDeposit, int termMonths,
                               bool autoRenew = false, const Date &openDate = Date::today());

    //信用卡开户
    long long openCreditAccount(long long ownerId, double creditLimit, const Date &openDate = Date::today());

    // 存款
    bool deposit(long long accountId, double amount, const std::string &remark = "");
    //取款
    bool withdraw(long long accountId, double amount, const std::string &remark = "");

    //定期账户的取款
    double withdrawFixed(long long accountId, const Date &today = Date::today());

    //转账
    bool transfer(long long fromAccountId, long long toAccountId, double amount,
                  const std::string &remark = "");

    //结息
    double settleInterest(long long accountId, const Date &today = Date::today());

    // 批量处理到期自动转存
    int processMaturityRollovers(const Date &today = Date::today());

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

    // 查询流水
    std::vector<const Transaction *> queryTransactions(long long accountId = 0,
                                                       int typeFilter = 0,
                                                       const std::string &startDate = "",
                                                       const std::string &endDate = "") const;

    Account *findAccount(long long accountId);
    User *findUser(long long userId);

    bool addUser(std::unique_ptr<User> user);
    void addAccount(std::unique_ptr<Account> account);
    void addTransaction(const Transaction &t);

    long long getNextTransactionId() const { return nextTransactionId; }
    const std::map<long long, std::unique_ptr<Account>> &getAccounts() const { return accounts; }
    const std::map<long long, std::unique_ptr<User>> &getUsers() const { return users; }
    const std::vector<Transaction> &getTransactions() const { return transactions; }
};

#endif // BANK_H
