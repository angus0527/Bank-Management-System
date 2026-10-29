//
// Created by huang on 2026/10/28.
//

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdexcept>
#include <string>

enum class TransactionType {
    DEP = 1, WDR = 2, TRF = 3, INT = 4 // 存款，取款，转账，结息
};

// 一条交易流水（README §5 transactions.txt 格式）
// 金额、快照一律 long long "分"；账号 0 表示"无"（如存款的付款方）
class Transaction {
    long long id;
    std::string datetime;         // "2026-10-29 19:00:05"
    TransactionType type;
    long long fromAccountId;      // 付款账号（无则 0）
    long long toAccountId;        // 收款账号（无则 0）
    long long amount;             // 金额（分）
    long long balanceSnapshot;    // 交易后余额快照（分）
    std::string remark;

public:
    Transaction(long long id, std::string datetime, TransactionType type,
                long long fromAccountId, long long toAccountId, long long amount,
                long long balanceSnapshot, std::string remark);

    // "T|流水号|时间|DEP|付款账号|收款账号|金额(分)|余额快照(分)|备注"
    std::string serialize() const;

    void display() const;

    // 类型 ↔ 存储格式中的英文码（DEP/WDR/TRF/INT），供 storage 层读写用
    static std::string typeCode(TransactionType type);

    static TransactionType typeFromCode(const std::string &code); // 非法码抛 invalid_argument

    long long getId() const {
        return id;
    }

    std::string getDatetime() const {
        return datetime;
    }

    TransactionType getType() const {
        return type;
    }

    long long getFromAccountId() const {
        return fromAccountId;
    }

    long long getToAccountId() const {
        return toAccountId;
    }

    long long getAmount() const {
        return amount;
    }

    long long getBalanceSnapshot() const {
        return balanceSnapshot;
    }

    std::string getRemark() const {
        return remark;
    }
};

#endif //TRANSACTION_H
