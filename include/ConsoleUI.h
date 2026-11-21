#ifndef CONSOLEUI_H
#define CONSOLEUI_H

#include <string>

#include "Account.h"
#include "Bank.h"
#include "Transaction.h"
#include "User.h"

// 将交易类型枚举转为中文名称（导出 CSV 用）
std::string txTypeName(TransactionType t);

// 将账户类型枚举转为中文名称（管理员总览用）
std::string accTypeName(AccountType t);

// 判断账号是否属于当前登录客户
bool hasAccount(const Customer *cust, long long aid);

// 读取流水查询条件（交易类型、起始日期、结束日期）
bool readTxFilter(int &typeFilter, std::string &startD, std::string &endD);

// 客户查询交易流水（只显示本人名下账户相关的流水）
void showCustomerTransactions(const Bank &bank, const Customer *cust, long long accId);

// 管理员查询交易流水（可查看全行或指定账号的所有流水）
void showAdminTransactions(const Bank &bank, long long accId);

#endif // CONSOLEUI_H
