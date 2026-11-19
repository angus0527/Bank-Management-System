#include "../include/CreditAccount.h"

// 信用账户构造：基类 Account 要求传入非负初始值；若从磁盘还原已有透支欠款（负余额），
// 先以 0 通过基类构造，校验授信额度与利率后，再在构造体中恢复实际欠款余额
CreditAccount::CreditAccount(long long id, long long ownerId, long long initialBalance,
                             const Date &openDate, long long limit, long long dailyRate)
    : Account(id, ownerId, initialBalance >= 0 ? initialBalance : 0, openDate),
      creditLimit(limit < 0 ? 0 : limit),
      dailyRateX10000(dailyRate < 0 ? 0 : dailyRate),
      lastSettleDate(openDate) {
    balance = initialBalance;
}

// 还款 / 预存操作：直接增加余额（若原先处于透支负余额状态，相当于冲抵所欠本息）
bool CreditAccount::deposit(long long amount) {
    if (amount <= 0) {
        std::cout << "操作失败：还款/存入金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止还款。\n";
        return false;
    }
    balance += amount;
    return true;
}

// 重写取款以支持额度内透支消费：当前可用额度 = 当前余额 + 信用额度
bool CreditAccount::withdraw(long long amount) {
    if (amount <= 0) {
        std::cout << "操作失败：透支/取现金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止消费透支。\n";
        return false;
    }
    long long available = getAvailableCredit();
    if (amount > available) {
        std::cout << "操作失败：超出可用信用额度，当前可用额度 " << formatMoney(available)
                  << " 元，尝试支出 " << formatMoney(amount) << " 元。\n";
        return false;
    }
    balance -= amount;
    return true;
}

// 欠款按日计息：仅当存在透支欠款（debt > 0）且距上次结息已过整数天时计息
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

// 信用透支结息：算出的利息从 balance 扣除（即累计到当前欠款总额中），更新上次计息日
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

// 序列化信用账户到 accounts.txt
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
