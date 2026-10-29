//
// Created by huang on 2026/10/28.
//

#include "../include/CurrentAccount.h"

// 开户时尚未结息，上次结息日即开户日
CurrentAccount::CurrentAccount(long long id, long long ownerId, long long initialBalance,
                               const Date &openDate, long long rate)
    : Account(id, ownerId, initialBalance, openDate), rateX10000(rate), lastSettleDate(openDate) {
}

// 按日计息：余额 × 年利率/10000 × 天数/360，整数先乘后除、向下取整到分
long long CurrentAccount::calcInterest(const Date &today) const {
    long long days = lastSettleDate.daysBetween(today);
    if (days <= 0 || balance <= 0) {
        return 0;
    }
    return balance * rateX10000 * days / (10000LL * 360LL);
}

long long CurrentAccount::settleInterest(const Date &today) {
    long long interest = calcInterest(today);
    balance += interest;
    lastSettleDate = today;
    return interest;
}

AccountType CurrentAccount::getAccountType() const {
    return AccountType::SAVINGS;
}

// README §5 accounts.txt 格式：12 个公共字段 + 活期附加的"上次结息日"尾字段
// （老格式无第 13 字段时读档方默认取开户日，向后兼容）
std::string CurrentAccount::serialize() const {
    std::string s = "A|" + std::to_string(accountId) + "|C|" + std::to_string(ownerId) + "|" +
                    std::to_string(balance) + "|" + openDate.toString() + "|" +
                    std::to_string(rateX10000) + "|0|0|0|0|0|" + lastSettleDate.toString();
    return s;
}

void CurrentAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【账户类型】: 活期\n";
    std::cout << "【年利率】: " << formatMoney(rateX10000) << "%\n";
    std::cout << "【上次结息日】: " << lastSettleDate.toString() << "\n";
}
