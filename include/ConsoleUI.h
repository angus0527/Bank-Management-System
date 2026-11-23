#ifndef CONSOLEUI_H
#define CONSOLEUI_H

#include <string>

#include "Account.h"
#include "Bank.h"
#include "Transaction.h"
#include "User.h"

// 将交易类型枚举转为中文名称
std::string txTypeName(TransactionType t);

// 将账户类型枚举转为中文名称
std::string accTypeName(AccountType t);

// 读取流水查询条件（交易类型、起始日期、结束日期）
bool readTxFilter(int &typeFilter, std::string &startD, std::string &endD);

// 客户查询交易流水
void showCustomerTransactions(const Bank &bank, const Customer *cust, long long accId);

// 管理员查询交易流水
void showAdminTransactions(const Bank &bank, long long accId);

#endif // CONSOLEUI_H
