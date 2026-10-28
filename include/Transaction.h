//
// Created by huang on 2026/10/28.
//

#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <string>
#include <sstream>
enum class TransactionType {
    DEP = 1, WDR = 2, TRF = 3, INT = 4 //存款，取款，转账，结息
};

class Transaction {
    long long id;
    std::string datetime;
    TransactionType type;
    std::string fromAccountId;
    std::string toAccountId;
    double amount;
    double balanceSnapshot;
    std::string remark;

public:
    Transaction(long long id,
                std::string datetime,
                TransactionType type,
                std::string fromAccountId,
                std::string toAccountId,
                double amount,
                double balanceSnapshot,
                std::string remark);

    std::string serialize() const;

    void display() const;

    long long getId() const {
        return id;
    }

    std::string getDatetime() const {
        return datetime;
    }

    TransactionType getType() const {
        return type;
    }

    std::string getFromAccountId() const {
        return fromAccountId;
    }

    std::string getToAccountId() const {
        return toAccountId;
    }

    double getAmount() const {
        return amount;
    }

    double getBalanceSnapshot() const {
        return balanceSnapshot;
    }

    std::string getRemark() const {
        return remark;
    }
};


#endif //TRANSACTION_H
