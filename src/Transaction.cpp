//
// Created by huang on 2026/10/28.
//

#include "../include/Transaction.h"

#include <iostream>

#include "../include/Account.h"

Transaction::Transaction(long long id, std::string datetime, TransactionType type,
                         long long fromAccountId, long long toAccountId, long long amount,
                         long long balanceSnapshot, std::string remark)
    : id(id), datetime(std::move(datetime)), type(type), fromAccountId(fromAccountId),
      toAccountId(toAccountId), amount(amount), balanceSnapshot(balanceSnapshot),
      remark(std::move(remark)) {
}

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

TransactionType Transaction::typeFromCode(const std::string &code) {
    if (code == "DEP") return TransactionType::DEP;
    if (code == "WDR") return TransactionType::WDR;
    if (code == "TRF") return TransactionType::TRF;
    if (code == "INT") return TransactionType::INT;
    throw std::invalid_argument("未知的交易类型码: \"" + code + "\"");
}

std::string Transaction::serialize() const {
    return "T|" + std::to_string(id) + "|" + datetime + "|" + typeCode(type) + "|" +
           std::to_string(fromAccountId) + "|" + std::to_string(toAccountId) + "|" +
           std::to_string(amount) + "|" + std::to_string(balanceSnapshot) + "|" + remark;
}

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
