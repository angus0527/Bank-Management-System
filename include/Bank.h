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

// Bank 门面类：全部业务入口集中于此，
// UI/控制台层只调用 Bank 的公共接口，不直接碰 Account/User 内部字段。
// core 层不依赖 storage：持久化由 Storage::load/save 负责，
// 操作日志由调用方通过 Logger 记录。
class Bank {
    std::map<long long, std::unique_ptr<User>> users;
    std::map<long long, std::unique_ptr<Account>> accounts;
    std::vector<Transaction> transactions;
    long long nextAccountId = 62001;     // 账号从 62001 起
    long long nextTransactionId = 1;

    // 各档基准利率（万分之一为单位，可由管理员实时调整）
    long long currentRateX10000 = 35;      // 活期年利率 0.35%
    long long fixed3MRateX10000 = 125;     // 定期 3 个月年利率 1.25%
    long long fixed1YRateX10000 = 175;     // 定期 1 年年利率 1.75%
    long long fixed3YRateX10000 = 250;     // 定期 3 年年利率 2.50%
    long long creditDailyRateX10000 = 5;   // 信用账户日利率 0.05%

    // 追加一条流水（余额快照由调用方在交易完成后传入）
    void recordTx(TransactionType type, long long fromId, long long toId, long long amount,
                  long long balanceSnapshot, const std::string &remark);

public:
    // —— 鉴权与用户信息管理 ——
    // 用户不存在或密码错误均抛 AuthFail（不区分提示，防试探）
    User *login(long long userId, const std::string &password);

    void updateUserInfo(long long userId, const std::string &newName, const std::string &newPhone = "");

    void changeUserPassword(long long userId, const std::string &oldPassword, const std::string &newPassword);

    // —— 账户业务 ——
    // 开立活期账户：owner 必须是 Customer；initialDeposit ≥ 0；返回新账号
    long long openAccount(long long ownerId, long long initialDeposit,
                          const Date &openDate = Date::today());

    // 开立定期账户：termMonths 支持 3 / 12 / 36
    long long openFixedAccount(long long ownerId, long long initialDeposit, int termMonths,
                               bool autoRenew = false, const Date &openDate = Date::today());

    // 开立信用账户：creditLimit > 0
    long long openCreditAccount(long long ownerId, long long creditLimit,
                                const Date &openDate = Date::today());

    void deposit(long long accountId, long long amount, const std::string &remark = "");

    void withdraw(long long accountId, long long amount, const std::string &remark = "");

    // 定期账户全额支取（支持未到期提前支取按活期计息与到期支取），返回实付本息总额（分）
    long long withdrawFixed(long long accountId, const Date &today = Date::today());

    // 转账：先完成全部校验再变更余额，保证原子性（任一校验失败双方余额都不动）
    void transfer(long long fromAccountId, long long toAccountId, long long amount,
                  const std::string &remark = "");

    // 手动结息；未到结息日返回 0
    long long settleInterest(long long accountId, const Date &today = Date::today());

    // 到期自动转存批处理（返回完成自动转存的笔数）
    int processMaturityRollovers(const Date &today = Date::today());

    // 管理员操作：风控冻结/解冻、销户、利率管理
    void freezeAccount(long long accountId);

    void unfreezeAccount(long long accountId);

    // 销户：余额不为 0 抛 InvalidAmount
    void closeAccount(long long accountId);

    void setCurrentRate(long long rateX10000);
    void setFixedRate(int termMonths, long long rateX10000);
    void setCreditDailyRate(long long rateX10000);

    long long getCurrentRateX10000() const { return currentRateX10000; }
    long long getFixedRateX10000(int termMonths) const;
    long long getCreditDailyRateX10000() const { return creditDailyRateX10000; }

    // 仅赋值基准配置（供 storage 读档恢复使用）
    void initRates(long long cur, long long f3m, long long f1y, long long f3y, long long cr);

    // —— 查询 ——
    // accountId 为 0 时返回全部流水；否则返回与该账户相关的流水（按时间顺序）
    std::vector<const Transaction *> queryTransactions(long long accountId = 0) const;

    // 多条件组合筛选：accountId(0=全行), typeFilter(0=全部,1=DEP,2=WDR,3=TRF,4=INT), 日期段("YYYY-MM-DD"或空)
    std::vector<const Transaction *> queryTransactionsFiltered(long long accountId = 0,
                                                               int typeFilter = 0,
                                                               const std::string &startDate = "",
                                                               const std::string &endDate = "") const;

    // 供 UI/控制台展示
    Account *findAccount(long long accountId);       // 不存在抛 AccountNotFound
    User *findUser(long long userId);                // 不存在抛 AccountNotFound

    // —— storage 层装载/读取接口 ——
    void addUser(std::unique_ptr<User> user);        // 装载已有用户（id 冲突抛 InvalidAmount）
    void addAccount(std::unique_ptr<Account> account); // 直接装载，不走业务校验
    void addTransaction(const Transaction &t);
    void setNextAccountId(long long id);
    void setNextTransactionId(long long id);

    long long getNextAccountId() const { return nextAccountId; }
    long long getNextTransactionId() const { return nextTransactionId; }

    const std::map<long long, std::unique_ptr<Account>> &getAccounts() const { return accounts; }
    const std::map<long long, std::unique_ptr<User>> &getUsers() const { return users; }
    const std::vector<Transaction> &getTransactions() const { return transactions; }
};

#endif //BANK_H
