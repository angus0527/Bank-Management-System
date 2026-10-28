//
// Created by huang on 2026/10/28.
//

#ifndef CURRENTACCOUNT_H
#define CURRENTACCOUNT_H
#include "Account.h"


class CurrentAccount : public Account {
    double annualRate;

public:
    CurrentAccount(const std::string &id, const std::string &owner, double initialBalance, const std::string &date,
                   double rate);

    bool withdraw(double amount) override;

    bool deposit(double amount) override;

    double calculateInterest() override;

    void settlePeriod() override;

    AccountType getAccountType() const override;

    std::string serialize() const override;

    void displayInfo() const override;

    double getAnnualRate() const {
        return annualRate;
    }
};


#endif //CURRENTACCOUNT_H
