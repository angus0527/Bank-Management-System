#include "../include/ConsoleUI.h"

#include <iostream>
#include <vector>
//一些控制台用的辅助函数
// 将交易类型枚举转为中文名称
std::string txTypeName(TransactionType t) {
    if (t == TransactionType::DEP) return "存款";
    if (t == TransactionType::WDR) return "取款";
    if (t == TransactionType::TRF) return "转账";
    return "结息";
}

// 将账户类型枚举转为中文名称
std::string accTypeName(AccountType t) {
    if (t == AccountType::SAVINGS) return "活期";
    if (t == AccountType::FIXED)   return "定期";
    return "信用";
}

// 判断账号是否属于当前登录客户
bool hasAccount(const Customer *cust, long long aid) {
    for (long long id : cust->getAccountIds()) {
        if (id == aid) return true;
    }
    return false;
}

// 读取流水查询条件
bool readTxFilter(int &typeFilter, std::string &startD, std::string &endD) {
    std::cout << "按类型筛选 (0=全部 1=存款 2=取款 3=转账 4=结息): ";
    std::cin >> typeFilter;
    if (typeFilter < 0 || typeFilter > 4) {
        std::cout << "类型编号不合法。\n";
        return false;
    }
    std::cout << "起始日期 (如 2026-10-01，输 0 不限): ";
    std::cin >> startD;
    if (startD == "0") startD.clear();
    std::cout << "结束日期 (如 2026-12-31，输 0 不限): ";
    std::cin >> endD;
    if (endD == "0") endD.clear();
    return true;
}

// 客户查询交易流水
void showCustomerTransactions(const Bank &bank, const Customer *cust, long long accId) {
    int typeFilter = 0;
    std::string startD, endD;
    if (!readTxFilter(typeFilter, startD, endD)) return;

    std::vector<const Transaction *> list;
    for (const Transaction *t : bank.queryTransactions(accId, typeFilter, startD, endD)) {
        //用户要么是转账方要么是收款方
        if (hasAccount(cust, t->getFromAccountId()) || hasAccount(cust, t->getToAccountId())) {
            list.push_back(t);
        }
    }

    std::cout << "\n--- 我的交易流水（共 " << list.size() << " 条）---\n";
    for (const Transaction *t : list) {
        t->display();
    }
}

// 管理员查询交易流水
void showAdminTransactions(const Bank &bank, long long accId) {
    int typeFilter = 0;
    std::string startD, endD;
    if (!readTxFilter(typeFilter, startD, endD)) return;

    std::vector<const Transaction *> list = bank.queryTransactions(accId, typeFilter, startD, endD);
    std::cout << "\n--- 全行交易流水（共 " << list.size() << " 条）---\n";
    for (const Transaction *t : list) {
        t->display();
    }
}
