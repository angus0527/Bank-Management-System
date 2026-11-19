#ifndef FIXEDACCOUNT_H
#define FIXEDACCOUNT_H

#include "Account.h"

// 定期存款账户：支持 3 / 12 / 36 个月整存整取，提前支取按活期利率计息
class FixedAccount : public Account {
    int termMonths;        // 存期（3、12 或 36 个月）
    double rate;           // 定期年利率（如 0.0175 表示 1.75%）
    double currentRate;    // 提前支取用的活期年利率（如 0.0035 表示 0.35%）
    Date maturityDate;     // 到期日期
    bool autoRenew;        // 到期是否自动转存

public:
    FixedAccount(long long id, long long ownerId, double initialBalance,
                 const Date &openDate, int termMonths, double rate,
                 bool autoRenew = false, double currentRate = 0.0035);

    bool deposit(double amount) override;
    bool withdraw(double amount) override;
    double calcInterest(const Date &today) const override;
    double settleInterest(const Date &today) override;

    AccountType getAccountType() const override { return AccountType::FIXED; }
    std::string serialize() const override;
    void displayInfo() const override;

    int getTermMonths() const { return termMonths; }
    double getRate() const { return rate; }
    void setRate(double newRate) { rate = newRate; }
    void setCurrentRate(double newRate) { currentRate = newRate; }
    const Date &getMaturityDate() const { return maturityDate; }
    void setMaturityDate(const Date &d) { maturityDate = d; }
    bool isAutoRenew() const { return autoRenew; }
};

#endif // FIXEDACCOUNT_H
