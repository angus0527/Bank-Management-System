//
// Created by huang on 2026/10/28.
//

#include "../include/CurrentAccount.h"

CurrentAccount::CurrentAccount(const std::string &id, const std::string &owner, double initialBalance,
                               const std::string &date, double rate= 0.003) : Account(id, owner, initialBalance, date),
                                                                       annualRate(rate) {
}

bool CurrentAccount::withdraw(double amount) {
    if (status != AccountStatus::NORMAL || amount <= 0 || amount > balance) {
        return false;
    } else {
        balance -= amount;
        return true;
    }
}

bool CurrentAccount::deposit(double amount) {
    if (status == AccountStatus::CLOSED || amount <= 0) {
        return false;
    } else {
        balance += amount;
        return true;
    }
}

double CurrentAccount::calculateInterest() {
    return balance * annualRate / 360;
}

void CurrentAccount::settlePeriod() {
    balance += calculateInterest();
}

AccountType CurrentAccount::getAccountType() const {
    return AccountType::SAVINGS;
}

std::string CurrentAccount::serialize() const {
    std::stringstream ss;
    ss << "A|" << accountId << "|C|" << ownerId << "|" << balance << "|" << openDate << "|" << annualRate;
    return ss.str();
}

void CurrentAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【年利率】 " << (annualRate * 100) << "%\n";
}


