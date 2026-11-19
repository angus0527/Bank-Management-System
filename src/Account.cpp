#include "../include/Account.h"

std::string formatMoney(long long cents) {
    // 分转元，保留两位小数；负数（信用账户欠款）同样适用
    long long absCents = cents < 0 ? -cents : cents;
    std::string sign = cents < 0 ? "-" : "";
    int fen = static_cast<int>(absCents % 100);
    return sign + std::to_string(absCents / 100) + (fen < 10 ? ".0" : ".") + std::to_string(fen);
}

Account::Account(long long id, long long ownerId_, long long initialBalance, const Date &date)
    : accountId(id), ownerId(ownerId_), balance(initialBalance),
      status(AccountStatus::NORMAL), openDate(date) {
    if (balance < 0) {
        std::cout << "提示：初始金额不能为负，已按 0.00 元处理。\n";
        balance = 0;
    }
}

std::string Account::getStatusString() const {
    switch (status) {
        case AccountStatus::NORMAL:
            return "正常";
        case AccountStatus::FROZEN:
            return "冻结";
        case AccountStatus::CLOSED:
            return "销户";
        default:
            return "未知状态";
    }
}

bool Account::deposit(long long amount) {
    if (amount <= 0) {
        std::cout << "操作失败：存款金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止存款。\n";
        return false;
    }
    balance += amount;
    return true;
}

bool Account::withdraw(long long amount) {
    if (amount <= 0) {
        std::cout << "操作失败：取款金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止取款。\n";
        return false;
    }
    if (amount > balance) {
        std::cout << "操作失败：余额不足，当前余额 " << formatMoney(balance)
                  << " 元，尝试取款 " << formatMoney(amount) << " 元。\n";
        return false;
    }
    balance -= amount;
    return true;
}

void Account::displayInfo() const {
    std::cout << "----------------------------------------\n";
    std::cout << "【账号】: " << accountId << "\n";
    std::cout << "【户主id】: " << ownerId << "\n";
    std::cout << "【状态】: " << getStatusString() << "\n";
    std::cout << "【开户日期】: " << openDate.toString() << "\n";
    std::cout << "【当前余额】: " << formatMoney(balance) << " 元\n";
}

void Account::freeze() {
    status = AccountStatus::FROZEN;
}

void Account::unfreeze() {
    if (status == AccountStatus::FROZEN) {
        status = AccountStatus::NORMAL;
    }
}

bool Account::closeAccount() {
    if (balance != 0) {
        std::cout << "操作失败：账户尚有余额 " << formatMoney(balance) << " 元，禁止销户。\n";
        return false;
    }
    status = AccountStatus::CLOSED;
    return true;
}
