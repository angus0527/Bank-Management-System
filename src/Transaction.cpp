#include "../include/Transaction.h"

#include <iostream>

#include "../include/Account.h"

// 构造一条交易流水记录
Transaction::Transaction(long long id, std::string datetime, TransactionType type,
                         long long fromAccountId, long long toAccountId, double amount,
                         double balanceSnapshot, std::string remark)
    : id(id), datetime(std::move(datetime)), type(type), fromAccountId(fromAccountId),
      toAccountId(toAccountId), amount(round2(amount)), balanceSnapshot(round2(balanceSnapshot)),
      remark(std::move(remark)) {
}

// 将交易类型枚举转为存盘用的 3 字母英文码
std::string Transaction::typeCode(TransactionType type) {
    switch (type) {
        case TransactionType::DEP:
            return "DEP";
        case TransactionType::WDR:
            return "WDR";
        case TransactionType::TRF:
            return "TRF";
        case TransactionType::INT:
            return "INT";
    }
    return "???";
}

// 将存盘的英文类型码还原为交易类型枚举
TransactionType Transaction::typeFromCode(const std::string &code) {
    if (code == "DEP") return TransactionType::DEP;
    if (code == "WDR") return TransactionType::WDR;
    if (code == "TRF") return TransactionType::TRF;
    if (code == "INT") return TransactionType::INT;
    return TransactionType::DEP;
}

// 格式化为单行文本，用于保存到 transactions.txt
std::string Transaction::serialize() const {
    return "T|" + std::to_string(id) + "|" + datetime + "|" + typeCode(type) + "|" +
           std::to_string(fromAccountId) + "|" + std::to_string(toAccountId) + "|" +
           formatMoney(amount) + "|" + formatMoney(balanceSnapshot) + "|" + remark;
}

// 在控制台打印本条交易流水详情
void Transaction::display() const {
    static const char *names[] = {"存款", "取款", "转账", "结息"};
    std::cout << "[流水#" << id << "] " << datetime << " " << names[static_cast<int>(type) - 1]
              << " " << formatMoney(amount) << " 元";
    if (type == TransactionType::TRF) {
        std::cout << "（" << fromAccountId << " → " << toAccountId << "）";
    } else if (type == TransactionType::WDR) {
        std::cout << "（账户 " << fromAccountId << "）";
    } else {
        std::cout << "（账户 " << toAccountId << "）";
    }
    std::cout << "，交易后余额 " << formatMoney(balanceSnapshot) << " 元"
              << (remark.empty() ? "" : "，" + remark) << "\n";
}
