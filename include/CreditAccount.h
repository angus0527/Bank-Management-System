#ifndef CREDITACCOUNT_H
#define CREDITACCOUNT_H

#include "Account.h"

// 信用账户：支持信用额度内透支消费、欠款按日计息（默认 0.05%/日）、还款冲抵欠款
class CreditAccount : public Account {
    long long creditLimit;      // 信用额度（分）
    long long dailyRateX10000;  // 日利率 × 10000（5 即 0.05%/日）
    Date lastSettleDate;        // 上次计息日

public:
    CreditAccount(long long id, long long ownerId, long long initialBalance,
                  const Date &openDate, long long creditLimit = 500000,
                  long long dailyRateX10000 = 5);

    void deposit(long long amount) override;

    void withdraw(long long amount) override;

    long long calcInterest(const Date &today) const override;

    long long settleInterest(const Date &today) override;

    AccountType getAccountType() const override;

    std::string serialize() const override;

    void displayInfo() const override;

    long long getCreditLimit() const { return creditLimit; }

    void setCreditLimit(long long limit) { creditLimit = limit; }

    long long getDailyRateX10000() const { return dailyRateX10000; }

    void setDailyRateX10000(long long rate) { dailyRateX10000 = rate; }

    long long getDebt() const { return balance < 0 ? -balance : 0; }

    long long getAvailableCredit() const { return balance + creditLimit; }

    const Date &getLastSettleDate() const { return lastSettleDate; }

    void setLastSettleDate(const Date &date) { lastSettleDate = date; }
};

#endif // CREDITACCOUNT_H
