#include "../include/CreditAccount.h"

CreditAccount::CreditAccount(long long id, long long ownerId, long long initialBalance,
                             const Date &openDate, long long limit, long long dailyRate)
    : Account(id, ownerId, initialBalance >= 0 ? initialBalance : 0, openDate),
      creditLimit(limit),
      dailyRateX10000(dailyRate),
      lastSettleDate(openDate) {
    if (creditLimit < 0) {
        throw InvalidAmount("信用额度不能为负");
    }
    if (dailyRateX10000 < 0) {
        throw InvalidAmount("日利率不能为负");
    }
    // 读档还原欠款账户时允许初始余额为负（表示透支欠款）
    balance = initialBalance;
}

void CreditAccount::deposit(long long amount) {
    if (amount <= 0) {
        throw InvalidAmount("还款/存入金额必须大于 0");
    }
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止还款");
    }
    balance += amount;
}

void CreditAccount::withdraw(long long amount) {
    if (amount <= 0) {
        throw InvalidAmount("透支/取现金额必须大于 0");
    }
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止消费透支");
    }
    long long available = getAvailableCredit();
    if (amount > available) {
        throw InsufficientBalance("超出可用信用额度：当前可用额度 " + formatMoney(available) +
                                  " 元，尝试支出 " + formatMoney(amount) + " 元");
    }
    balance -= amount;
}

long long CreditAccount::calcInterest(const Date &today) const {
    if (status == AccountStatus::CLOSED) {
        return 0;
    }
    long long debt = getDebt();
    if (debt <= 0) {
        return 0;
    }
    long long days = lastSettleDate.daysBetween(today);
    if (days <= 0) {
        return 0;
    }
    return debt * dailyRateX10000 * days / 10000LL;
}

long long CreditAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED) {
        return 0;
    }
    long long interest = calcInterest(today);
    balance -= interest;
    lastSettleDate = today;
    return interest;
}

AccountType CreditAccount::getAccountType() const {
    return AccountType::CREDIT;
}

std::string CreditAccount::serialize() const {
    return "A|" + std::to_string(accountId) + "|X|" + std::to_string(ownerId) + "|" +
           std::to_string(balance) + "|" + openDate.toString() + "|" +
           std::to_string(dailyRateX10000) + "|0|0|0|" +
           std::to_string(creditLimit) + "|" + std::to_string(getDebt()) + "|" +
           lastSettleDate.toString() + "|" + std::to_string(static_cast<int>(status));
}

void CreditAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【账户类型】: 信用账户\n";
    std::cout << "【信用额度】: " << formatMoney(creditLimit) << " 元\n";
    std::cout << "【当前欠款】: " << formatMoney(getDebt()) << " 元\n";
    std::cout << "【可用额度】: " << formatMoney(getAvailableCredit()) << " 元\n";
    std::cout << "【透支日利率】: " << formatMoney(dailyRateX10000) << "%\n";
}
