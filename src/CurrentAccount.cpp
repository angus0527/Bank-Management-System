#include "../include/CurrentAccount.h"

#include <sstream>

// 开户时尚未结息，上次结息日即开户日
CurrentAccount::CurrentAccount(long long id, long long ownerId, double initialBalance,
                               const Date &openDate, double rate)
    : Account(id, ownerId, initialBalance, openDate), rate(rate), lastSettleDate(openDate) {
}

// 按日计息：本金 * 年利率 * 天数 / 360
double CurrentAccount::calcInterest(const Date &today) const {
    long long days = lastSettleDate.daysBetween(today);
    if (days <= 0 || balance <= 0.0) {
        return 0.0;
    }
    return round2(balance * rate * static_cast<double>(days) / 360.0);
}

// 活期结息：把应计利息并入余额，并把上次结息日更新为 today
double CurrentAccount::settleInterest(const Date &today) {
    double interest = calcInterest(today);
    balance = round2(balance + interest);
    lastSettleDate = today;
    return interest;
}

// 返回账户类型枚举（活期）
AccountType CurrentAccount::getAccountType() const {
    return AccountType::SAVINGS;
}

// 固定 14 列持久化格式：A|账号|C|户主id|余额|开户日|年利率|0|0|0|0|0|上次结息日|状态
std::string CurrentAccount::serialize() const {
    std::ostringstream oss;
    oss << "A|" << accountId << "|C|" << ownerId << "|"
        << formatMoney(balance) << "|" << openDate.toString() << "|"
        << rate << "|0|0|0|0|0|" << lastSettleDate.toString() << "|"
        << static_cast<int>(status);
    return oss.str();
}

// 打印活期账户的详细信息（含年利率与上次结息日）
void CurrentAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【账户类型】: 活期\n";
    std::cout << "【年利率】: " << formatMoney(rate * 100.0) << "%\n";
    std::cout << "【上次结息日】: " << lastSettleDate.toString() << "\n";
}
