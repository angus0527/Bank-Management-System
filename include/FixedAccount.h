#ifndef FIXEDACCOUNT_H
#define FIXEDACCOUNT_H

#include "Account.h"

// 定期存款账户：支持 3 / 12 / 36 个月整存整取，提前支取按活期利率计息
class FixedAccount : public Account {
    int termMonths;              // 存期（3、12 或 36 个月）
    long long rateX10000;        // 定期年利率 × 10000
    long long currentRateX10000; // 提前支取用的活期年利率 × 10000
    Date maturityDate;           // 到期日期
    bool autoRenew;              // 到期是否自动转存

public:
    FixedAccount(long long id, long long ownerId, long long initialBalance,
                 const Date &openDate, int termMonths, long long rateX10000,
                 bool autoRenew = false, long long currentRateX10000 = 35);

    bool deposit(long long amount) override;
    bool withdraw(long long amount) override;
    long long calcInterest(const Date &today) const override;
    long long settleInterest(const Date &today) override;

    AccountType getAccountType() const override { return AccountType::FIXED; }
    std::string serialize() const override;
    void displayInfo() const override;

    int getTermMonths() const { return termMonths; }
    long long getRateX10000() const { return rateX10000; }
    void setRateX10000(long long rate) { rateX10000 = rate; }
    void setCurrentRateX10000(long long rate) { currentRateX10000 = rate; }
    const Date &getMaturityDate() const { return maturityDate; }
    void setMaturityDate(const Date &d) { maturityDate = d; }
    bool isAutoRenew() const { return autoRenew; }
};

#endif // FIXEDACCOUNT_H
