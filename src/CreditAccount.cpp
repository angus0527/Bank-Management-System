#include "../include/CreditAccount.h"

#include <sstream>

//(账户id，用户id，初始存款，开户日期，信用额度，欠款年利率）
CreditAccount::CreditAccount(long long id, long long ownerId, double initialBalance,
                             const Date &openDate, double limit, double rate)
    : Account(id, ownerId, initialBalance >= 0.0 ? initialBalance : 0.0, openDate),
      creditLimit(limit < 0.0 ? 0.0 : round2(limit)),
      dailyRate(rate < 0.0 ? 0.0 : rate),
      lastSettleDate(openDate) {
    balance = round2(initialBalance);
}

// 还款
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

// 取款
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
    //防止浮点数误差
    if (amount > available + 0.005) {
        std::cout << "操作失败：超出可用信用额度，当前可用额度 " << formatMoney(available)
                  << " 元，尝试支出 " << formatMoney(amount) << " 元。\n";
        return false;
    }
    balance = round2(balance - amount);
    return true;
}

// 欠款利息,冻结账户也会算利息
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

// 信用透支结息
double CreditAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED) {
        return 0.0;
    }
    double interest = calcInterest(today);
    balance = round2(balance - interest);
    lastSettleDate = today;
    return interest;
}

// 返回账户类型枚举
AccountType CreditAccount::getAccountType() const {
    return AccountType::CREDIT;
}

// 固定 14 列持久化格式：A(账户)|账号|X(信用)|户主id|余额|开户日|透支日利率|0|0|0|信用额度|当前欠款|上次计息日|状态(1正常/2冻结/3销户)
std::string CreditAccount::serialize() const {
    std::ostringstream oss;
    oss << "A|" << accountId << "|X|" << ownerId << "|"
        << formatMoney(balance) << "|" << openDate.toString() << "|"
        << dailyRate << "|0|0|0|"
        << formatMoney(creditLimit) << "|" << formatMoney(getDebt()) << "|"
        << lastSettleDate.toString() << "|" << static_cast<int>(status);
    return oss.str();
}

// 打印信用账户详情（额度、欠款、可用额度与日利率）
void CreditAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【账户类型】: 信用账户\n";
    std::cout << "【信用额度】: " << formatMoney(creditLimit) << " 元\n";
    std::cout << "【当前欠款】: " << formatMoney(getDebt()) << " 元\n";
    std::cout << "【可用额度】: " << formatMoney(getAvailableCredit()) << " 元\n";
    std::cout << "【透支日利率】: " << formatMoney(dailyRate * 100.0) << "%\n";
}
