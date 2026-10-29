//
// Created by huang on 2026/10/28.
//

#ifndef CURRENTACCOUNT_H
#define CURRENTACCOUNT_H

#include "Account.h"

// 活期储蓄账户：年利率 / 360 按日计息，手动"结息"触发（README F09）
// 利率以 long long 存"年利率×10000"（35 即 0.35%），全程整数运算避免 double 精度问题
class CurrentAccount : public Account {
    long long rateX10000;  // 年利率 × 10000
    Date lastSettleDate;   // 上次结息日（serialize 的第 13 个尾字段）

public:
    explicit CurrentAccount(long long id, long long ownerId, long long initialBalance,
                            const Date &openDate, long long rateX10000 = 35);

    long long calcInterest(const Date &today) const override;

    long long settleInterest(const Date &today) override;

    AccountType getAccountType() const override;

    std::string serialize() const override;

    void displayInfo() const override;

    long long getRateX10000() const {
        return rateX10000;
    }

    // 管理员利率管理（Y02）与 storage 读档用
    void setRateX10000(long long rate) {
        rateX10000 = rate;
    }

    const Date &getLastSettleDate() const {
        return lastSettleDate;
    }

    void setLastSettleDate(const Date &date) {
        lastSettleDate = date;
    }
};

#endif //CURRENTACCOUNT_H
