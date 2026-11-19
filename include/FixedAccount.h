#ifndef FIXEDACCOUNT_H
#define FIXEDACCOUNT_H

#include "Account.h"

// 定期储蓄账户（整存整取）：支持 3/12/36 个月多档利率、到期自动转存、提前支取按活期计息
class FixedAccount : public Account {
    long long rateX10000;         // 定期年利率 × 10000
    long long currentRateX10000;  // 提前支取按此活期年利率 × 10000 计息
    int termMonths;               // 存期（月）：3 / 12 / 36
    Date maturityDate;            // 到期日
    bool autoRenew;               // 到期自动转存标志

public:
    FixedAccount(long long id, long long ownerId, long long initialBalance,
                 const Date &openDate, int termMonths, long long rateX10000,
                 bool autoRenew = false, long long currentRateX10000 = 35);

    FixedAccount(long long id, long long ownerId, long long initialBalance,
                 const Date &openDate, int termMonths, const Date &maturityDate,
                 long long rateX10000, bool autoRenew, long long currentRateX10000);

    void deposit(long long amount) override;

    void withdraw(long long amount) override;

    long long calcInterest(const Date &today) const override;

    long long settleInterest(const Date &today) override;

    // 定期全额支取前的结息：未到期按活期利率×实际天数计息，已到期按定期整期计息
    long long settleOnWithdraw(const Date &today);

    AccountType getAccountType() const override;

    std::string serialize() const override;

    void displayInfo() const override;

    long long getRateX10000() const { return rateX10000; }

    void setRateX10000(long long rate) { rateX10000 = rate; }

    long long getCurrentRateX10000() const { return currentRateX10000; }

    void setCurrentRateX10000(long long rate) { currentRateX10000 = rate; }

    int getTermMonths() const { return termMonths; }

    const Date &getMaturityDate() const { return maturityDate; }

    bool isAutoRenew() const { return autoRenew; }

    void setAutoRenew(bool renew) { autoRenew = renew; }
};

#endif // FIXEDACCOUNT_H
