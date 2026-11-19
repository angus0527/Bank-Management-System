#include "../include/Account.h"

#include <cmath>
#include <iomanip>
#include <sstream>

double round2(double value) {
    return std::round(value * 100.0) / 100.0;
}

std::string formatMoney(double yuan) {
    if (std::abs(yuan) < 0.005) {
        return "0.00";
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << round2(yuan);
    return oss.str();
}

Account::Account(long long id, long long ownerId_, double initialBalance, const Date &date)
    : accountId(id), ownerId(ownerId_), balance(round2(initialBalance)),
      status(AccountStatus::NORMAL), openDate(date) {
    if (balance < 0.0) {
        std::cout << "提示：初始金额不能为负，已按 0.00 元处理。\n";
        balance = 0.0;
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

bool Account::deposit(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：存款金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止存款。\n";
        return false;
    }
    balance = round2(balance + amount);
    return true;
}

bool Account::withdraw(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：取款金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止取款。\n";
        return false;
    }
    if (amount > balance + 0.005) {
        std::cout << "操作失败：余额不足，当前余额 " << formatMoney(balance)
                  << " 元，尝试取款 " << formatMoney(amount) << " 元。\n";
        return false;
    }
    balance = round2(balance - amount);
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
    if (std::abs(balance) > 0.005) {
        std::cout << "操作失败：账户尚有余额 " << formatMoney(balance) << " 元，禁止销户。\n";
        return false;
    }
    balance = 0.0;
    status = AccountStatus::CLOSED;
    return true;
}
