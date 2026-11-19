#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

enum class TransactionType {
    DEP = 1, WDR = 2, TRF = 3, INT = 4 // 存款，取款，转账，结息
};

// 一条交易流水（金额、快照单位为“元”；账号 0 表示“无”）
class Transaction {
    long long id;
    std::string datetime;         // "2026-10-29 19:00:05"
    TransactionType type;
    long long fromAccountId;      // 付款账号（无则 0）
    long long toAccountId;        // 收款账号（无则 0）
    double amount;                // 金额（元）
    double balanceSnapshot;       // 交易后余额快照（元）
    std::string remark;

public:
    Transaction(long long id, std::string datetime, TransactionType type,
                long long fromAccountId, long long toAccountId, double amount,
                double balanceSnapshot, std::string remark);

    // "T|流水号|时间|DEP|付款账号|收款账号|金额(元)|余额快照(元)|备注"
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
