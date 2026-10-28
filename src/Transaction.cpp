//
// Created by huang on 2026/10/28.
//

#include "../include/Transaction.h"

Transaction::Transaction(long long id, std::string datetime, TransactionType type, std::string fromAccountId,
                         std::string toAccountId, double amount, double balanceSnapshot,
                         std::string remark): id(id), datetime(datetime), type(type)
                                              , fromAccountId(fromAccountId), toAccountId(toAccountId), amount(amount),
                                              balanceSnapshot(balanceSnapshot), remark(remark) {
}


std::string Transaction::serialize() const {
    std::string s;
    switch (type) {
        case TransactionType::DEP:
            s = "存款";
            break;
        case TransactionType::WDR:
            s = "取款";
            break;
        case TransactionType::TRF:
            s = "转账";
            break;
        case TransactionType::INT:
            s = "结息";
            break;
    }
    std::stringstream ss;
    ss << "T|" << id << "|" << datetime << "|" << s << "|" << fromAccountId << "|" << toAccountId << "|" << amount <<
            "|" << balanceSnapshot << "|" << remark ;
    return ss.str();
}

void Transaction::display() const {
    std::string s;
    switch (type) {
        case TransactionType::DEP:
            s = "存款";
            break;
        case TransactionType::WDR:
            s = "取款";
            break;
        case TransactionType::TRF:
            s = "转账";
            break;
        case TransactionType::INT:
            s = "结息";
            break;
    }
    printf("[流水#%lld] %s %s %.2f 元，当前余额 %.2f 元\n",id, datetime.c_str(),s.c_str(), amount, balanceSnapshot);
}
