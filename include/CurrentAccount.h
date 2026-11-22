#ifndef CURRENTACCOUNT_H
#define CURRENTACCOUNT_H

#include "Account.h"

class CurrentAccount : public Account {
    double rate;           // 年利率
    Date lastSettleDate;   // 上次结息日

public:
    CurrentAccount(long long id, long long ownerId, double initialBalance,
                            const Date &openDate, double rate = 0.0035);

    double calcInterest(const Date &today) const override;

    double settleInterest(const Date &today) override;

    AccountType getAccountType() const override;

    std::string serialize() const override;

    void displayInfo() const override;

    double getRate() const { return rate; }

    void setRate(double newRate) { rate = newRate; }

    const Date &getLastSettleDate() const { return lastSettleDate; }

    void setLastSettleDate(const Date &date) { lastSettleDate = date; }
};

#endif // CURRENTACCOUNT_H
