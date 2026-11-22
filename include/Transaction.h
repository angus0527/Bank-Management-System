#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

enum class TransactionType {
    DEP = 1, WDR = 2, TRF = 3, INT = 4 // 存款，取款，转账，结息
};


class Transaction {
    long long id;
    std::string datetime;         // "2026-10-29 19:00:05"
    TransactionType type;         //DEP = 1, WDR = 2, TRF = 3, INT = 4  存款，取款，转账，结息
    long long fromAccountId;      // 付款账号（0表示没有）
    long long toAccountId;        // 收款账号（0表示没有）
    double amount;                // 金额
    double balanceSnapshot;       // 交易后余额
    std::string remark;

public:
    Transaction(long long id, std::string datetime, TransactionType type,
                long long fromAccountId, long long toAccountId, double amount,
                double balanceSnapshot, std::string remark);

    std::string serialize() const;

    void display() const;

    static std::string typeCode(TransactionType type);

    static TransactionType typeFromCode(const std::string &code);

    long long getId() const { return id; }
    std::string getDatetime() const { return datetime; }
    TransactionType getType() const { return type; }
    long long getFromAccountId() const { return fromAccountId; }
    long long getToAccountId() const { return toAccountId; }
    double getAmount() const { return amount; }
    double getBalanceSnapshot() const { return balanceSnapshot; }
    std::string getRemark() const { return remark; }
};

#endif // TRANSACTION_H
