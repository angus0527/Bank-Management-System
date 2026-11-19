#include "../include/CreditAccount.h"

#include <sstream>

// 信用账户构造：基类 Account 要求非负初始值；若从磁盘还原已有透支欠款（负余额），先以 0 过基类构造再恢复实际余额
CreditAccount::CreditAccount(long long id, long long ownerId, double initialBalance,
                             const Date &openDate, double limit, double rate)
    : Account(id, ownerId, initialBalance >= 0.0 ? initialBalance : 0.0, openDate),
      creditLimit(limit < 0.0 ? 0.0 : round2(limit)),
      dailyRate(rate < 0.0 ? 0.0 : rate),
      lastSettleDate(openDate) {
    balance = round2(initialBalance);
}

// 还款 / 预存操作：直接增加余额（若原先处于透支负余额状态，相当于冲抵所欠本息）
bool CreditAccount::deposit(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：还款/存入金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止还款。\n";
        return false;
    }
    balance = round2(balance + amount);
    return true;
}

// 重写取款以支持额度内透支消费：当前可用额度 = 当前余额 + 信用额度
bool CreditAccount::withdraw(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：透支/取现金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止消费透支。\n";
        return false;
    }
    double available = getAvailableCredit();
    if (amount > available + 0.005) {
        std::cout << "操作失败：超出可用信用额度，当前可用额度 " << formatMoney(available)
                  << " 元，尝试支出 " << formatMoney(amount) << " 元。\n";
        return false;
    }
    balance = round2(balance - amount);
    return true;
}

// 欠款按日计息：本金(欠款) * 日利率 * 天数
double CreditAccount::calcInterest(const Date &today) const {
    if (status == AccountStatus::CLOSED) {
        return 0.0;
    }
    double debt = getDebt();
    if (debt <= 0.0) {
        return 0.0;
    }
    long long days = lastSettleDate.daysBetween(today);
    if (days <= 0) {
        return 0.0;
    }
    return round2(debt * dailyRate * static_cast<double>(days));
}

// 信用透支结息：算出的利息从 balance 扣除（即累计到当前欠款总额中），更新上次计息日
double CreditAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED) {
        return 0.0;
    }
    double interest = calcInterest(today);
    balance = round2(balance - interest);
    lastSettleDate = today;
    return interest;
}

AccountType CreditAccount::getAccountType() const {
    return AccountType::CREDIT;
}

// 序列化信用账户到 accounts.txt
std::string CreditAccount::serialize() const {
    std::ostringstream oss;
    oss << "A|" << accountId << "|X|" << ownerId << "|"
        << formatMoney(balance) << "|" << openDate.toString() << "|"
        << dailyRate << "|0|0|0|"
        << formatMoney(creditLimit) << "|" << formatMoney(getDebt()) << "|"
        << lastSettleDate.toString() << "|" << static_cast<int>(status);
    return oss.str();
}

void CreditAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【账户类型】: 信用账户\n";
    std::cout << "【信用额度】: " << formatMoney(creditLimit) << " 元\n";
    std::cout << "【当前欠款】: " << formatMoney(getDebt()) << " 元\n";
    std::cout << "【可用额度】: " << formatMoney(getAvailableCredit()) << " 元\n";
    std::cout << "【透支日利率】: " << formatMoney(dailyRate * 100.0) << "%\n";
}
