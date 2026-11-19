#include "../include/FixedAccount.h"

FixedAccount::FixedAccount(long long id, long long ownerId, long long initialBalance,
                           const Date &openDate, int termMonths_, long long rate,
                           bool autoRenew_, long long currentRate)
    : Account(id, ownerId, initialBalance, openDate),
      rateX10000(rate),
      currentRateX10000(currentRate),
      termMonths(termMonths_),
      maturityDate(openDate.addMonths(termMonths_)),
      autoRenew(autoRenew_) {
    if (termMonths != 3 && termMonths != 12 && termMonths != 36) {
        throw InvalidAmount("定期存期仅支持 3 个月、12 个月（1 年）或 36 个月（3 年）");
    }
    if (rateX10000 < 0 || currentRateX10000 < 0) {
        throw InvalidAmount("利率不能为负");
    }
}

FixedAccount::FixedAccount(long long id, long long ownerId, long long initialBalance,
                           const Date &openDate, int termMonths_, const Date &maturityDate_,
                           long long rate, bool autoRenew_, long long currentRate)
    : Account(id, ownerId, initialBalance, openDate),
      rateX10000(rate),
      currentRateX10000(currentRate),
      termMonths(termMonths_),
      maturityDate(maturityDate_),
      autoRenew(autoRenew_) {
}

void FixedAccount::deposit(long long /*amount*/) {
    throw InvalidAmount("定期存款账户（整存整取）不支持中途追加存款");
}

void FixedAccount::withdraw(long long amount) {
    if (amount <= 0) {
        throw InvalidAmount("支取金额必须大于 0");
    }
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止支取");
    }
    if (amount > balance) {
        throw InsufficientBalance("余额不足：当前余额 " + formatMoney(balance) + " 元，尝试支取 " +
                                  formatMoney(amount) + " 元");
    }
    if (amount != balance) {
        throw InvalidAmount("定期存款为整存整取，请一次性全额支取（当前本息合计 " +
                            formatMoney(balance) + " 元）");
    }
    balance = 0;
}

long long FixedAccount::calcInterest(const Date &today) const {
    if (balance <= 0 || status == AccountStatus::CLOSED) {
        return 0;
    }
    if (today >= maturityDate) {
        return balance * rateX10000 * termMonths / (10000LL * 12LL);
    }
    long long days = openDate.daysBetween(today);
    if (days <= 0) {
        return 0;
    }
    return balance * currentRateX10000 * days / (10000LL * 360LL);
}

long long FixedAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED || balance <= 0 || today < maturityDate) {
        return 0;
    }
    long long interest = balance * rateX10000 * termMonths / (10000LL * 12LL);
    balance += interest;
    // 推进到下一存期起算点（支持到期自动转存及防止同周期重复结息）
    openDate = maturityDate;
    maturityDate = openDate.addMonths(termMonths);
    return interest;
}

long long FixedAccount::settleOnWithdraw(const Date &today) {
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止支取");
    }
    if (balance <= 0) {
        return 0;
    }
    long long interest = calcInterest(today);
    balance += interest;
    openDate = today;
    if (today >= maturityDate) {
        maturityDate = today.addMonths(termMonths);
    }
    return interest;
}

AccountType FixedAccount::getAccountType() const {
    return AccountType::FIXED;
}

std::string FixedAccount::serialize() const {
    return "A|" + std::to_string(accountId) + "|F|" + std::to_string(ownerId) + "|" +
           std::to_string(balance) + "|" + openDate.toString() + "|" +
           std::to_string(rateX10000) + "|" + std::to_string(termMonths) + "|" +
           maturityDate.toString() + "|" + (autoRenew ? "1" : "0") + "|0|0|" +
           std::to_string(currentRateX10000) + "|" + std::to_string(static_cast<int>(status));
}

void FixedAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【账户类型】: 定期（整存整取 " << termMonths << " 个月）\n";
    std::cout << "【年利率】: " << formatMoney(rateX10000) << "%\n";
    std::cout << "【到期日期】: " << maturityDate.toString() << "\n";
    std::cout << "【到期自动转存】: " << (autoRenew ? "是" : "否") << "\n";
}
