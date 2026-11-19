#include "../include/FixedAccount.h"

FixedAccount::FixedAccount(long long id, long long ownerId, long long initialBalance,
                           const Date &openDate, int termMonths_, long long rate,
                           bool autoRenew_, long long currentRate)
    : Account(id, ownerId, initialBalance, openDate),
      termMonths(termMonths_),
      rateX10000(rate),
      currentRateX10000(currentRate),
      maturityDate(openDate.addMonths(termMonths_)),
      autoRenew(autoRenew_) {
    if (termMonths != 3 && termMonths != 12 && termMonths != 36) {
        throw InvalidAmount("定期存期只支持 3、12 或 36 个月");
    }
}

void FixedAccount::deposit(long long /*amount*/) {
    throw InvalidAmount("定期账户不支持中途追加存款");
}

void FixedAccount::withdraw(long long amount) {
    if (amount <= 0) {
        throw InvalidAmount("支取金额必须大于 0");
    }
    if (status != AccountStatus::NORMAL) {
        throw AccountFrozen("账户已" + getStatusString() + "，禁止支取");
    }
    if (amount != balance) {
        throw InvalidAmount("定期存款必须一次性全额支取，当前余额为 " + formatMoney(balance) + " 元");
    }
    balance = 0;
}

// 多态计息：到期按定期利率算整期利息；未到期提前支取按活期利率×实际天数算
long long FixedAccount::calcInterest(const Date &today) const {
    if (balance <= 0 || status == AccountStatus::CLOSED) return 0;
    if (today >= maturityDate) {
        return balance * rateX10000 * termMonths / (10000LL * 12LL);
    }
    long long days = openDate.daysBetween(today);
    if (days <= 0) return 0;
    return balance * currentRateX10000 * days / (10000LL * 360LL);
}

// 到期结息并顺延到下一周期；未到期不提前结息
long long FixedAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED || balance <= 0 || today < maturityDate) return 0;
    long long interest = calcInterest(today);
    balance += interest;
    openDate = maturityDate;
    maturityDate = openDate.addMonths(termMonths);
    return interest;
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
    std::cout << "【类型】: 定期（" << termMonths << "个月，到期日 " << maturityDate.toString()
              << (autoRenew ? "，自动转存）\n" : "，不自动转存）\n");
}
