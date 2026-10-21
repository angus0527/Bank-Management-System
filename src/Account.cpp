#include "Account.h"

Account::Account(const std::string& id, const std::string& owner, double initialBalance, const std::string& date)
    : accountId(id), ownerId(owner), balance(initialBalance), status(AccountStatus::NORMAL), openDate(date) {
}
Account::~Account() {
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

void Account::displayInfo() const {
    std::cout << "----------------------------------------\n";
    std::cout << "【账号】: " << accountId << "\n";
    std::cout << "【户主】: " << ownerId << "\n";
    std::cout << "【状态】: " << getStatusString() << "\n";
    std::cout << "【开户日期】: " << openDate << "\n";
    std::cout << "【当前余额】: " << std::fixed << std::setprecision(2) << balance << " 元\n";
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
    if (balance > 0.0001 || balance < -0.0001) {
        return false; // 尚有未结清款项，禁止销户
    }
    status = AccountStatus::CLOSED;
    return true;
}