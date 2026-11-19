#ifndef CREDITACCOUNT_H
#define CREDITACCOUNT_H

#include "Account.h"

// 信用账户：支持信用额度内透支消费、欠款按日计息（默认 0.0005 即 0.05%/日）、还款冲抵欠款
class CreditAccount : public Account {
    double creditLimit;    // 信用额度（元）
    double dailyRate;      // 日利率（如 0.0005 表示 0.05%/日）
    Date lastSettleDate;   // 上次计息日

public:
    CreditAccount(long long id, long long ownerId, double initialBalance,
                  const Date &openDate, double creditLimit = 5000.0,
                  double dailyRate = 0.0005);

    bool deposit(double amount) override;

    bool withdraw(double amount) override;

    double calcInterest(const Date &today) const override;

    double settleInterest(const Date &today) override;

    AccountType getAccountType() const override;

    std::string serialize() const override;

    void displayInfo() const override;

    double getCreditLimit() const { return creditLimit; }

    void setCreditLimit(double limit) { creditLimit = limit; }

    double getDailyRate() const { return dailyRate; }

    void setDailyRate(double rate) { dailyRate = rate; }

    double getDebt() const { return balance < -0.005 ? -balance : 0.0; }

    double getAvailableCredit() const { return balance + creditLimit; }

    const Date &getLastSettleDate() const { return lastSettleDate; }

    void setLastSettleDate(const Date &date) { lastSettleDate = date; }
};

#endif // CREDITACCOUNT_H
