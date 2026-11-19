#include "../include/FixedAccount.h"

#include <cmath>
#include <sstream>

FixedAccount::FixedAccount(long long id, long long ownerId, double initialBalance,
                           const Date &openDate, int termMonths_, double rate_,
                           bool autoRenew_, double currentRate_)
    : Account(id, ownerId, initialBalance, openDate),
      termMonths((termMonths_ == 3 || termMonths_ == 12 || termMonths_ == 36) ? termMonths_ : 12),
      rate(rate_),
      currentRate(currentRate_),
      maturityDate(openDate.addMonths(termMonths)),
      autoRenew(autoRenew_) {
    if (termMonths_ != 3 && termMonths_ != 12 && termMonths_ != 36) {
        std::cout << "提示：定期存期仅支持 3、12 或 36 个月，已默认按 12 个月处理。\n";
    }
}

bool FixedAccount::deposit(double /*amount*/) {
    std::cout << "操作失败：定期账户不支持中途追加存款。\n";
    return false;
}

bool FixedAccount::withdraw(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：支取金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户已" << getStatusString() << "，禁止支取。\n";
        return false;
    }
    if (std::abs(amount - balance) > 0.005) {
        std::cout << "操作失败：定期存款必须一次性全额支取，当前余额为 " << formatMoney(balance) << " 元。\n";
        return false;
    }
    balance = 0.0;
    return true;
}

// 多态计息：到期按 本金 * 定期年利率 * 月数 / 12；未到期提前支取按 本金 * 活期年利率 * 天数 / 360
double FixedAccount::calcInterest(const Date &today) const {
    if (balance <= 0.0 || status == AccountStatus::CLOSED) return 0.0;
    if (today >= maturityDate) {
        return round2(balance * rate * static_cast<double>(termMonths) / 12.0);
    }
    long long days = openDate.daysBetween(today);
    if (days <= 0) return 0.0;
    return round2(balance * currentRate * static_cast<double>(days) / 360.0);
}

// 到期结息并顺延到下一周期；未到期不提前结息
double FixedAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED || balance <= 0.0 || today < maturityDate) return 0.0;
    double interest = calcInterest(today);
    balance = round2(balance + interest);
    openDate = maturityDate;
    maturityDate = openDate.addMonths(termMonths);
    return interest;
}

std::string FixedAccount::serialize() const {
    std::ostringstream oss;
    oss << "A|" << accountId << "|F|" << ownerId << "|"
        << formatMoney(balance) << "|" << openDate.toString() << "|"
        << rate << "|" << termMonths << "|"
        << maturityDate.toString() << "|" << (autoRenew ? "1" : "0") << "|0|0|"
        << currentRate << "|" << static_cast<int>(status);
    return oss.str();
}

void FixedAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【类型】: 定期（" << termMonths << "个月，到期日 " << maturityDate.toString()
              << (autoRenew ? "，自动转存）\n" : "，不自动转存）\n");
}
