#include "../include/Transaction.h"

#include <iostream>

#include "../include/Account.h"

// 将交易类型枚举转为整数
int typeToInt(TransactionType type) {
    return static_cast<int>(type);
}

//(流水id，日期，流水类型，转出账户，转入账户，数额，流水快照，注释)
Transaction::Transaction(long long id, std::string datetime, TransactionType type,
                         long long fromAccountId, long long toAccountId, double amount,
                         double balanceSnapshot, std::string remark)
    : id(id), datetime(std::move(datetime)), type(type), fromAccountId(fromAccountId),
      toAccountId(toAccountId), amount(round2(amount)), balanceSnapshot(round2(balanceSnapshot)),
      remark(std::move(remark)) {
}

// 将交易类型枚举转为string
std::string Transaction::typeToCode(TransactionType type) {
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

// 将string还原为交易类型枚举
TransactionType Transaction::typeFromCode(const std::string &code) {
    if (code == "DEP") return TransactionType::DEP;
    if (code == "WDR") return TransactionType::WDR;
    if (code == "TRF") return TransactionType::TRF;
    if (code == "INT") return TransactionType::INT;
    return TransactionType::DEP;
}

// T(流水)|流水号|时间|类型码(DEP存款/WDR取款/TRF转账/INT结息)|付款账号|收款账号|金额|交易后余额|备注
std::string Transaction::serialize() const {
    return "T|" + std::to_string(id) + "|" + datetime + "|" + typeToCode(type) + "|" +
           std::to_string(fromAccountId) + "|" + std::to_string(toAccountId) + "|" +
           formatMoney(amount) + "|" + formatMoney(balanceSnapshot) + "|" + remark;
}

// 在控制台打印本条交易流水详情
void Transaction::display() const {
    static const char *names[] = {"存款", "取款", "转账", "结息"};
    std::cout << "[流水#" << id << "] " << datetime << " " << names[typeToInt(type) - 1]
              << " " << formatMoney(amount) << " 元";
    if (type == TransactionType::TRF) {
        std::cout << "（" << fromAccountId << " → " << toAccountId << "）";
    } else if (type == TransactionType::WDR) {
        std::cout << "（账户 " << fromAccountId << "）";
    } else {
        long long showId = (toAccountId != 0) ? toAccountId : fromAccountId;
        std::cout << "（账户 " << showId << "）";
    }
    std::cout << "，交易后余额 " << formatMoney(balanceSnapshot) << " 元"
              << (remark.empty() ? "" : "，" + remark) << "\n";
}
