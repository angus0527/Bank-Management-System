#include "../include/Account.h"

std::string formatMoney(long long cents) {
    // 分转元，保留两位小数；负数（信用账户欠款）同样适用
    long long absCents = cents < 0 ? -cents : cents;
    std::string sign = cents < 0 ? "-" : "";
    std::string yuan = std::to_string(absCents / 100);
    int fen = static_cast<int>(absCents % 100);
    char tail[4];
    std::snprintf(tail, sizeof(tail), ".%02d", fen);
    return sign + yuan + tail;
}

Account::Account(long long id, long long ownerId_, long long initialBalance, const Date &date)
    : accountId(id), ownerId(ownerId_), balance(initialBalance),
      status(AccountStatus::NORMAL), openDate(date) {
    if (initialBalance < 0) {
        throw InvalidAmount("初始金额不能为负");
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

void Account::deposit(long long amount) {
    if (amount <= 0) {
        throw InvalidAmount("存款金额必须大于 0");
    }
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止存款");
    }
    balance += amount;
}

void Account::withdraw(long long amount) {
    if (amount <= 0) {
        throw InvalidAmount("取款金额必须大于 0");
    }
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止取款");
    }
    if (amount > balance) {
        throw InsufficientBalance("余额不足：当前余额 " + formatMoney(balance) + " 元，尝试取款 " +
                                  formatMoney(amount) + " 元");
    }
    balance -= amount;
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

void Account::closeAccount() {
    if (balance != 0) {
        throw InvalidAmount("账户尚有余额 " + formatMoney(balance) + " 元，禁止销户");
    }
    status = AccountStatus::CLOSED;
}
