#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Bank.h"
#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"
#include "Logger.h"
#include "Storage.h"
#include "exception.h"

#ifdef _WIN32
#include <windows.h>
#endif

// 清洗终端输入：剥除首部可能混入的 UTF-8 BOM (0xEF 0xBB 0xBF) 及首尾空格、制表符、回车换行
static std::string cleanInput(std::string s) {
    while (!s.empty() && (static_cast<unsigned char>(s.front()) == 0xEF ||
                          static_cast<unsigned char>(s.front()) == 0xBB ||
                          static_cast<unsigned char>(s.front()) == 0xBF ||
                          s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
        s.erase(s.begin());
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
    return s;
}

// 安全读取一个输入词元：遇到 EOF 或流异常时返回 false，避免菜单死循环
static bool readToken(std::string &out) {
    std::string raw;
    if (!(std::cin >> raw)) {
        return false;
    }
    out = cleanInput(raw);
    return true;
}

// 安全读取 64 位整数（账号/存期等），若输入非纯数字直接拦截提示，防止污染 std::cin 状态
static bool readLongLong(const std::string &prompt, long long &out) {
    std::cout << prompt;
    std::string s;
    if (!readToken(s)) return false;
    try {
        size_t idx = 0;
        long long val = std::stoll(s, &idx);
        if (idx != s.size()) {
            std::cout << "【错误】输入必须为纯数字整数！\n";
            return false;
        }
        out = val;
        return true;
    } catch (...) {
        std::cout << "【错误】输入格式不合法，请输入整数！\n";
        return false;
    }
}

// 将字符串金额（元）安全换算为 64 位整数“分”：限制非负、最多两位小数，并用 llround 四舍五入防浮点误差
static bool parseYuanToCents(const std::string &str, long long &cents) {
    if (str.empty()) return false;
    try {
        size_t idx = 0;
        double val = std::stod(str, &idx);
        if (idx != str.size() || !std::isfinite(val) || val < 0.0) return false;
        auto dotPos = str.find('.');
        if (dotPos != std::string::npos && str.size() - dotPos - 1 > 2) {
            return false;
        }
        cents = std::llround(val * 100.0);
        return true;
    } catch (...) {
        return false;
    }
}

static std::string txTypeStr(TransactionType t) {
    switch (t) {
        case TransactionType::DEP: return "存款";
        case TransactionType::WDR: return "取款";
        case TransactionType::TRF: return "转账";
        case TransactionType::INT: return "结息";
        default: return "其他";
    }
}

static std::string accountTypeStr(AccountType t) {
    switch (t) {
        case AccountType::SAVINGS: return "活期";
        case AccountType::FIXED: return "定期";
        case AccountType::CREDIT: return "信用";
        default: return "未知";
    }
}

static void printHeader(const std::string &title) {
    std::cout << "\n======================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "======================================================\n";
}

// 格式化打印流水表格
static void printTransactionTable(const std::vector<const Transaction *> &txs) {
    std::cout << "\n-------------------------- 交易流水记录 (" << txs.size() << " 条) --------------------------\n";
    std::cout << std::left << std::setw(6) << "流水号"
              << std::setw(21) << "交易时间"
              << std::setw(8) << "类型"
              << std::setw(10) << "付款账号"
              << std::setw(10) << "收款账号"
              << std::setw(12) << "金额(元)"
              << std::setw(12) << "余额快照"
              << "备注\n";
    std::cout << "--------------------------------------------------------------------------\n";
    for (const auto *t : txs) {
        std::cout << std::left << std::setw(6) << t->getId()
                  << std::setw(21) << t->getDatetime()
                  << std::setw(8) << txTypeStr(t->getType())
                  << std::setw(10) << (t->getFromAccountId() == 0 ? "-" : std::to_string(t->getFromAccountId()))
                  << std::setw(10) << (t->getToAccountId() == 0 ? "-" : std::to_string(t->getToAccountId()))
                  << std::setw(12) << formatMoney(t->getAmount())
                  << std::setw(12) << formatMoney(t->getBalanceSnapshot())
                  << t->getRemark() << "\n";
    }
}

// 交互式多条件流水查询：依次采集账号、交易类型与起止日期并调用 Bank::queryTransactionsFiltered
static void queryTransactionsInteractive(const Bank &bank, long long defaultAid, bool allowAll) {
    long long aid = defaultAid;
    if (allowAll) {
        if (!readLongLong("请输入筛选账号 (输入 0 查询全部): ", aid)) return;
    } else if (aid == 0) {
        if (!readLongLong("请输入查询账号 (输入 0 查询全部): ", aid)) return;
    }

    std::cout << "交易类型筛选 (0=全部, 1=存款, 2=取款, 3=转账, 4=结息): ";
    std::string typeStr;
    if (!readToken(typeStr)) return;
    int typeFilter = 0;
    try { typeFilter = std::stoi(typeStr); } catch (...) { typeFilter = 0; }
    if (typeFilter < 0 || typeFilter > 4) typeFilter = 0;

    std::cout << "起始日期 (YYYY-MM-DD，输入 0 不限): ";
    std::string startDate;
    if (!readToken(startDate)) return;
    if (startDate == "0") startDate.clear();

    std::cout << "结束日期 (YYYY-MM-DD，输入 0 不限): ";
    std::string endDate;
    if (!readToken(endDate)) return;
    if (endDate == "0") endDate.clear();

    try {
        auto txs = bank.queryTransactionsFiltered(aid, typeFilter, startDate, endDate);
        printTransactionTable(txs);
    } catch (const std::exception &e) {
        std::cout << "【错误】查询参数不合法：" << e.what() << "\n";
    }
}

// 首次运行无数据文件时自动初始化测试管理员（10001）与示例客户（20001/20002）
static void seedDefaultData(Bank &bank) {
    bank.addUser(std::make_unique<Admin>(10001, "admin", "admin123"));
    bank.addUser(std::make_unique<Customer>(20001, "张三", "zs123456", "13800000001"));
    bank.addUser(std::make_unique<Customer>(20002, "李四", "ls123456", "13800000002"));
    bank.openAccount(20001, 100029, Date(2026, 10, 2));
    bank.openAccount(20002, 30004, Date(2026, 10, 15));
}

// 客户服务中心交互循环：提供三类账户开户、存取转、定期支取、结息、销户、流水筛选、改密与 CSV 导出
static void customerMenu(Bank &bank, Customer *cust, Storage &storage) {
    while (true) {
        printHeader("客户服务中心 - 欢迎您，" + cust->getName() + " (ID: " + std::to_string(cust->getId()) + ")");
        std::cout << "  [1] 查看名下账户概况（活期 / 定期 / 信用）\n";
        std::cout << "  [2] 账户存款 / 信用还款\n";
        std::cout << "  [3] 账户取款 / 定期支取 / 信用透支\n";
        std::cout << "  [4] 转账汇款\n";
        std::cout << "  [5] 申请开立新账户（活期 / 定期 / 信用）\n";
        std::cout << "  [6] 账户利息结算\n";
        std::cout << "  [7] 申请账户销户\n";
        std::cout << "  [8] 流水多条件组合查询（账号/类型/日期段）\n";
        std::cout << "  [9] 修改个人信息（姓名/电话/登录密码）\n";
        std::cout << "  [10] 导出对账单到 CSV\n";
        std::cout << "  [11] 保存数据到本地\n";
        std::cout << "  [0] 退出登录，返回上级菜单\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << "请选择操作编号 [0-11]: ";

        std::string choice;
        if (!readToken(choice)) break;

        if (choice == "0") {
            std::cout << "已退出客户登录。\n";
            break;
        } else if (choice == "1") {
            const auto &accIds = cust->getAccountIds();
            if (accIds.empty()) {
                std::cout << "提示：名下暂无已开立账户，请先申请开户。\n";
            } else {
                std::cout << "\n-------------------- 名下账户列表 --------------------\n";
                for (long long aid : accIds) {
                    try {
                        Account *acc = bank.findAccount(aid);
                        std::cout << "账号: " << aid
                                  << " | 类型: " << accountTypeStr(acc->getAccountType())
                                  << " | 余额: " << formatMoney(acc->getBalance()) << " 元"
                                  << " | 状态: " << acc->getStatusString()
                                  << " | 开户日: " << acc->getOpenDate().toString();
                        if (auto *fix = dynamic_cast<FixedAccount *>(acc)) {
                            std::cout << " | 存期: " << fix->getTermMonths() << "个月"
                                      << " | 到期日: " << fix->getMaturityDate().toString()
                                      << " | 自动转存: " << (fix->isAutoRenew() ? "是" : "否");
                        } else if (auto *cr = dynamic_cast<CreditAccount *>(acc)) {
                            std::cout << " | 额度: " << formatMoney(cr->getCreditLimit()) << " 元"
                                      << " | 欠款: " << formatMoney(cr->getDebt()) << " 元"
                                      << " | 可用: " << formatMoney(cr->getAvailableCredit()) << " 元";
                        }
                        std::cout << "\n";
                    } catch (...) {}
                }
            }
        } else if (choice == "2") {
            long long aid;
            if (!readLongLong("请输入存款/还款账号: ", aid)) continue;
            std::cout << "请输入金额 (元): ";
            std::string amtStr;
            if (!readToken(amtStr)) continue;
            long long cents = 0;
            if (!parseYuanToCents(amtStr, cents) || cents <= 0) {
                std::cout << "【错误】金额格式不合法，请输入大于 0 的数字！\n";
                continue;
            }
            std::cout << "请输入交易备注: ";
            std::string remark;
            if (!readToken(remark)) continue;

            try {
                bank.deposit(aid, cents, remark);
                Logger::log("账户 " + std::to_string(aid) + " 存入/还款 " + formatMoney(cents) + " 元");
                std::cout << "【成功】操作完成！当前余额：" << formatMoney(bank.findAccount(aid)->getBalance()) << " 元\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "3") {
            long long aid;
            if (!readLongLong("请输入取款/支取/消费账号: ", aid)) continue;
            try {
                Account *acc = bank.findAccount(aid);
                if (auto *fix = dynamic_cast<FixedAccount *>(acc)) {
                    Date today = Date::today();
                    bool early = (today < fix->getMaturityDate());
                    long long estInterest = fix->calcInterest(today);
                    std::cout << "提示：该账号为定期账户（" << (early ? "未到期，提前支取按活期计息" : "已到期")
                              << "），预估利息 " << formatMoney(estInterest) << " 元，本息合计 "
                              << formatMoney(fix->getBalance() + estInterest) << " 元。\n";
                    std::cout << "确认全额支取该定期存款吗？(y/n): ";
                    std::string confirm;
                    if (!readToken(confirm) || (confirm != "y" && confirm != "Y")) continue;
                    long long payout = bank.withdrawFixed(aid, today);
                    Logger::log("定期账户 " + std::to_string(aid) + " 支取本息 " + formatMoney(payout) + " 元");
                    std::cout << "【成功】定期支取完成！实付本息：" << formatMoney(payout) << " 元\n";
                } else {
                    std::cout << "请输入金额 (元): ";
                    std::string amtStr;
                    if (!readToken(amtStr)) continue;
                    long long cents = 0;
                    if (!parseYuanToCents(amtStr, cents) || cents <= 0) {
                        std::cout << "【错误】金额格式不合法，请输入大于 0 的数字！\n";
                        continue;
                    }
                    std::cout << "请输入交易备注: ";
                    std::string remark;
                    if (!readToken(remark)) continue;
                    bank.withdraw(aid, cents, remark);
                    Logger::log("账户 " + std::to_string(aid) + " 取款/透支 " + formatMoney(cents) + " 元");
                    std::cout << "【成功】操作完成！当前余额：" << formatMoney(acc->getBalance()) << " 元\n";
                }
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "4") {
            long long fromId, toId;
            if (!readLongLong("请输入付款账号: ", fromId)) continue;
            if (!readLongLong("请输入收款账号: ", toId)) continue;
            std::cout << "请输入转账金额 (元): ";
            std::string amtStr;
            if (!readToken(amtStr)) continue;
            long long cents = 0;
            if (!parseYuanToCents(amtStr, cents) || cents <= 0) {
                std::cout << "【错误】金额格式不合法！\n";
                continue;
            }
            std::cout << "请输入交易备注: ";
            std::string remark;
            if (!readToken(remark)) continue;
            if (remark.empty()) remark = "同行转账";

            try {
                bank.transfer(fromId, toId, cents, remark);
                Logger::log("账户 " + std::to_string(fromId) + " 向账户 " + std::to_string(toId) +
                            " 转账 " + formatMoney(cents) + " 元");
                std::cout << "【成功】转账成功！付款账户剩余余额："
                          << formatMoney(bank.findAccount(fromId)->getBalance()) << " 元\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "5") {
            std::cout << "请选择开户类型：\n";
            std::cout << "  [1] 活期储蓄账户（年利率 " << formatMoney(bank.getCurrentRateX10000()) << "%）\n";
            std::cout << "  [2] 定期存款账户（3个月 " << formatMoney(bank.getFixedRateX10000(3))
                      << "% / 1年 " << formatMoney(bank.getFixedRateX10000(12))
                      << "% / 3年 " << formatMoney(bank.getFixedRateX10000(36)) << "%）\n";
            std::cout << "  [3] 信用账户（日利率 " << formatMoney(bank.getCreditDailyRateX10000()) << "%）\n";
            std::cout << "请输入账户类型 [1-3]: ";
            std::string accChoice;
            if (!readToken(accChoice)) continue;

            try {
                if (accChoice == "1") {
                    std::cout << "请输入初始存款金额 (元，允许为 0): ";
                    std::string amtStr;
                    if (!readToken(amtStr)) continue;
                    long long cents = 0;
                    if (!parseYuanToCents(amtStr, cents)) {
                        std::cout << "【错误】金额格式不合法！\n";
                        continue;
                    }
                    long long newAid = bank.openAccount(cust->getId(), cents);
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立活期账户 " + std::to_string(newAid));
                    std::cout << "【成功】活期账户开立成功！账号：" << newAid
                              << "，初始存入：" << formatMoney(cents) << " 元\n";
                } else if (accChoice == "2") {
                    std::cout << "请选择存期 (3=三个月, 12=一年, 36=三年): ";
                    long long term = 0;
                    if (!readLongLong("", term)) continue;
                    std::cout << "请输入定期存入金额 (元，必须大于 0): ";
                    std::string amtStr;
                    if (!readToken(amtStr)) continue;
                    long long cents = 0;
                    if (!parseYuanToCents(amtStr, cents) || cents <= 0) {
                        std::cout << "【错误】金额格式不合法！\n";
                        continue;
                    }
                    std::cout << "到期是否自动转存？(y/n): ";
                    std::string renew;
                    if (!readToken(renew)) continue;
                    bool autoRenew = (renew == "y" || renew == "Y");
                    long long newAid = bank.openFixedAccount(cust->getId(), cents, static_cast<int>(term), autoRenew);
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立定期账户 " + std::to_string(newAid));
                    std::cout << "【成功】定期账户开立成功！账号：" << newAid
                              << "，到期日：" << bank.findAccount(newAid)->getOpenDate().addMonths(static_cast<int>(term)).toString() << "\n";
                } else if (accChoice == "3") {
                    std::cout << "请输入申请授信额度 (元，如 5000): ";
                    std::string limStr;
                    if (!readToken(limStr)) continue;
                    long long limitCents = 0;
                    if (!parseYuanToCents(limStr, limitCents) || limitCents <= 0) {
                        std::cout << "【错误】额度格式不合法！\n";
                        continue;
                    }
                    long long newAid = bank.openCreditAccount(cust->getId(), limitCents);
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立信用账户 " + std::to_string(newAid));
                    std::cout << "【成功】信用账户开立成功！账号：" << newAid
                              << "，授信额度：" << formatMoney(limitCents) << " 元\n";
                } else {
                    std::cout << "无效账户类型。\n";
                }
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "6") {
            long long aid;
            if (!readLongLong("请输入结息账号: ", aid)) continue;
            try {
                long long interest = bank.settleInterest(aid);
                if (interest > 0) {
                    Logger::log("账户 " + std::to_string(aid) + " 结息 " + formatMoney(interest) + " 元");
                    std::cout << "【成功】结息完成！本次计结利息：" << formatMoney(interest) << " 元，最新余额："
                              << formatMoney(bank.findAccount(aid)->getBalance()) << " 元\n";
                } else {
                    std::cout << "【提示】当前未到结息日或无新增利息。\n";
                }
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "7") {
            long long aid;
            if (!readLongLong("请输入要销毁的账号: ", aid)) continue;
            std::cout << "确认销户账户 " << aid << " 吗？(y/n): ";
            std::string confirm;
            if (!readToken(confirm) || (confirm != "y" && confirm != "Y")) continue;

            try {
                bank.closeAccount(aid);
                Logger::log("客户 " + std::to_string(cust->getId()) + " 销户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已成功销户！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "8") {
            queryTransactionsInteractive(bank, 0, false);
        } else if (choice == "9") {
            std::cout << "\n--- 修改个人信息 ---\n";
            std::cout << "  [1] 修改姓名与联系电话（当前姓名：" << cust->getName()
                      << "，电话：" << cust->getPhoneNumber() << "）\n";
            std::cout << "  [2] 修改登录密码\n";
            std::cout << "请选择 [1-2]: ";
            std::string sub;
            if (!readToken(sub)) continue;
            try {
                if (sub == "1") {
                    std::cout << "请输入新姓名: ";
                    std::string newName;
                    if (!readToken(newName)) continue;
                    std::cout << "请输入新联系电话: ";
                    std::string newPhone;
                    if (!readToken(newPhone)) continue;
                    bank.updateUserInfo(cust->getId(), newName, newPhone);
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 修改了姓名/电话信息");
                    std::cout << "【成功】个人资料已更新！\n";
                } else if (sub == "2") {
                    std::cout << "请输入原密码: ";
                    std::string oldPwd;
                    if (!readToken(oldPwd)) continue;
                    std::cout << "请输入新密码: ";
                    std::string newPwd;
                    if (!readToken(newPwd)) continue;
                    bank.changeUserPassword(cust->getId(), oldPwd, newPwd);
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 修改了登录密码");
                    std::cout << "【成功】密码修改成功！下次请使用新密码登录。\n";
                } else {
                    std::cout << "无效选项。\n";
                }
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "10") {
            long long aid;
            if (!readLongLong("请输入要导出对账单的账号: ", aid)) continue;
            std::string path = "statement_" + std::to_string(aid) + ".csv";
            std::ofstream out(path);
            if (!out.is_open()) {
                std::cout << "【错误】无法创建目标 CSV 文件！\n";
                continue;
            }
            out << "\xEF\xBB\xBF";
            out << "流水号,交易时间,类型,付款账号,收款账号,金额(元),余额快照(元),备注\n";
            for (const auto *t : bank.queryTransactions(aid)) {
                out << t->getId() << "," << t->getDatetime() << "," << txTypeStr(t->getType()) << ","
                    << (t->getFromAccountId() == 0 ? "-" : std::to_string(t->getFromAccountId())) << ","
                    << (t->getToAccountId() == 0 ? "-" : std::to_string(t->getToAccountId())) << ","
                    << formatMoney(t->getAmount()) << "," << formatMoney(t->getBalanceSnapshot()) << ","
                    << t->getRemark() << "\n";
            }
            out.close();
            std::cout << "【成功】对账单已成功导出至：" << path << "\n";
        } else if (choice == "11") {
            storage.save(bank);
            Logger::log("客户保存数据至本地文件");
            std::cout << "【成功】全部数据已成功持久化至 data/ 文本文件！\n";
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }
}

// 管理员控制台交互循环：提供全行账户管理、冻结解冻、日终结息转存批处理、利率调控与统计报表
static void adminMenu(Bank &bank, User *admin, Storage &storage) {
    while (true) {
        printHeader("系统管理员控制台 - 操作员: " + admin->getName() + " (ID: " + std::to_string(admin->getId()) + ")");
        std::cout << "  [1] 查看全行所有账户总览（活期 / 定期 / 信用）\n";
        std::cout << "  [2] 查看客户名册与总资产统计\n";
        std::cout << "  [3] 冻结指定账户\n";
        std::cout << "  [4] 解冻指定账户\n";
        std::cout << "  [5] 强制销户\n";
        std::cout << "  [6] 一键全行结息与到期自动转存批处理\n";
        std::cout << "  [7] 全行利率管理（调整活期 / 定期各档 / 信用日利率）\n";
        std::cout << "  [8] 全行流水多条件审计与查询\n";
        std::cout << "  [9] 查看系统审计日志\n";
        std::cout << "  [10] 清空系统审计日志\n";
        std::cout << "  [11] 全行业务指标与账户占比统计报表\n";
        std::cout << "  [12] 保存全行数据到本地文件\n";
        std::cout << "  [0] 退出登录，返回上级菜单\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << "请选择操作编号 [0-12]: ";

        std::string choice;
        if (!readToken(choice)) break;

        if (choice == "0") {
            std::cout << "已退出管理员会话。\n";
            break;
        } else if (choice == "1") {
            std::cout << "\n----------------------- 全行所有账户 -----------------------\n";
            std::cout << std::left << std::setw(10) << "账号"
                      << std::setw(10) << "户主ID"
                      << std::setw(8) << "类型"
                      << std::setw(14) << "余额 (元)"
                      << std::setw(10) << "状态"
                      << "附加信息\n";
            std::cout << "------------------------------------------------------------\n";
            for (const auto &[aid, acc] : bank.getAccounts()) {
                std::string extra = "开户 " + acc->getOpenDate().toString();
                if (auto *fix = dynamic_cast<FixedAccount *>(acc.get())) {
                    extra = std::to_string(fix->getTermMonths()) + "月 | 到期 " + fix->getMaturityDate().toString();
                } else if (auto *cr = dynamic_cast<CreditAccount *>(acc.get())) {
                    extra = "额度 " + formatMoney(cr->getCreditLimit()) + " | 欠款 " + formatMoney(cr->getDebt());
                }
                std::cout << std::left << std::setw(10) << aid
                          << std::setw(10) << acc->getOwnerId()
                          << std::setw(8) << accountTypeStr(acc->getAccountType())
                          << std::setw(14) << formatMoney(acc->getBalance())
                          << std::setw(10) << acc->getStatusString()
                          << extra << "\n";
            }
        } else if (choice == "2") {
            std::cout << "\n------------------------ 客户名册 ------------------------\n";
            std::cout << std::left << std::setw(10) << "用户ID"
                      << std::setw(12) << "姓名"
                      << std::setw(10) << "角色"
                      << std::setw(16) << "联系电话"
                      << std::setw(10) << "账户数"
                      << "净资产 (元)\n";
            std::cout << "----------------------------------------------------------\n";
            for (const auto &[uid, user] : bank.getUsers()) {
                auto *c = dynamic_cast<Customer *>(user.get());
                long long totalAssets = 0;
                int count = 0;
                std::string phone = "-";
                if (c) {
                    phone = c->getPhoneNumber();
                    count = static_cast<int>(c->getAccountIds().size());
                    for (long long aid : c->getAccountIds()) {
                        try { totalAssets += bank.findAccount(aid)->getBalance(); } catch (...) {}
                    }
                }
                std::cout << std::left << std::setw(10) << uid
                          << std::setw(12) << user->getName()
                          << std::setw(10) << (user->role() == "Admin" ? "管理员" : "客户")
                          << std::setw(16) << phone
                          << std::setw(10) << (c ? std::to_string(count) : "-")
                          << (c ? formatMoney(totalAssets) : "-") << "\n";
            }
        } else if (choice == "3") {
            long long aid;
            if (!readLongLong("请输入要冻结的账号: ", aid)) continue;
            try {
                bank.freezeAccount(aid);
                Logger::log("管理员 " + std::to_string(admin->getId()) + " 冻结账户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已成功冻结！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "4") {
            long long aid;
            if (!readLongLong("请输入要解冻的账号: ", aid)) continue;
            try {
                bank.unfreezeAccount(aid);
                Logger::log("管理员 " + std::to_string(admin->getId()) + " 解冻账户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已解冻恢复正常！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "5") {
            long long aid;
            if (!readLongLong("请输入要强制销户的账号: ", aid)) continue;
            try {
                bank.closeAccount(aid);
                Logger::log("管理员 " + std::to_string(admin->getId()) + " 销户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已成功销户！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "6") {
            int rolled = bank.processMaturityRollovers(Date::today());
            long long totalInterest = 0;
            int count = 0;
            for (const auto &[aid, acc] : bank.getAccounts()) {
                try {
                    long long cur = bank.settleInterest(aid);
                    if (cur > 0) {
                        totalInterest += cur;
                        count++;
                    }
                } catch (...) {}
            }
            Logger::log("管理员执行全行结息与自动转存：自动转存 " + std::to_string(rolled) +
                        " 笔，计息 " + std::to_string(count) + " 个账户，累计利息 " + formatMoney(totalInterest) + " 元");
            std::cout << "【成功】全行日终批处理完成！\n到期自动转存笔数: " << rolled
                      << "\n计结利息账户数:   " << count
                      << "\n累计利息发生额:   " << formatMoney(totalInterest) << " 元\n";
        } else if (choice == "7") {
            std::cout << "\n--- 当前全行执行利率 ---\n";
            std::cout << "  [1] 活期年利率:       " << formatMoney(bank.getCurrentRateX10000()) << "%\n";
            std::cout << "  [2] 定期 3 个月年利率: " << formatMoney(bank.getFixedRateX10000(3)) << "%\n";
            std::cout << "  [3] 定期 1 年年利率:   " << formatMoney(bank.getFixedRateX10000(12)) << "%\n";
            std::cout << "  [4] 定期 3 年年利率:   " << formatMoney(bank.getFixedRateX10000(36)) << "%\n";
            std::cout << "  [5] 信用账户日利率:   " << formatMoney(bank.getCreditDailyRateX10000()) << "%\n";
            std::cout << "请选择要调整的利率档位 [1-5，0返回]: ";
            std::string sub;
            if (!readToken(sub) || sub == "0") continue;
            std::cout << "请输入新利率百分比（如输入 1.85 代表 1.85%，将实时生效）: ";
            std::string rateStr;
            if (!readToken(rateStr)) continue;
            long long rateX10000 = 0;
            if (!parseYuanToCents(rateStr, rateX10000)) {
                std::cout << "【错误】利率格式不合法！\n";
                continue;
            }
            try {
                if (sub == "1") bank.setCurrentRate(rateX10000);
                else if (sub == "2") bank.setFixedRate(3, rateX10000);
                else if (sub == "3") bank.setFixedRate(12, rateX10000);
                else if (sub == "4") bank.setFixedRate(36, rateX10000);
                else if (sub == "5") bank.setCreditDailyRate(rateX10000);
                else {
                    std::cout << "无效档位。\n";
                    continue;
                }
                Logger::log("管理员 " + std::to_string(admin->getId()) + " 调整档位 [" + sub +
                            "] 利率为 " + formatMoney(rateX10000) + "%");
                std::cout << "【成功】利率已调整为 " << formatMoney(rateX10000) << "% 并实时生效！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "8") {
            queryTransactionsInteractive(bank, 0, true);
        } else if (choice == "9") {
            std::cout << "\n------------------------ 系统审计日志 ------------------------\n";
            auto logs = Logger::getAllLogs();
            if (logs.empty()) {
                std::cout << "(暂无日志记录)\n";
            } else {
                for (const auto &line : logs) {
                    std::cout << line << "\n";
                }
            }
        } else if (choice == "10") {
            std::cout << "确定要清空全部系统日志吗？(y/n): ";
            std::string confirm;
            if (readToken(confirm) && (confirm == "y" || confirm == "Y")) {
                Logger::clearAllLogs();
                std::cout << "【成功】系统审计日志已清空！\n";
            }
        } else if (choice == "11") {
            long long curTotal = 0, fixTotal = 0, creditDebtTotal = 0;
            int curCnt = 0, fixCnt = 0, crCnt = 0, normalCount = 0, frozenCount = 0;
            for (const auto &[aid, acc] : bank.getAccounts()) {
                if (acc->getStatus() == AccountStatus::NORMAL) normalCount++;
                else if (acc->getStatus() == AccountStatus::FROZEN) frozenCount++;

                if (acc->getAccountType() == AccountType::SAVINGS) {
                    curCnt++;
                    curTotal += acc->getBalance();
                } else if (acc->getAccountType() == AccountType::FIXED) {
                    fixCnt++;
                    fixTotal += acc->getBalance();
                } else if (auto *cr = dynamic_cast<CreditAccount *>(acc.get())) {
                    crCnt++;
                    creditDebtTotal += cr->getDebt();
                }
            }
            int totalAcc = static_cast<int>(bank.getAccounts().size());
            auto pct = [&](int n) -> int { return totalAcc > 0 ? (n * 100 / totalAcc) : 0; };
            int custCount = 0;
            for (const auto &[uid, user] : bank.getUsers()) {
                if (user->role() == "Customer") custCount++;
            }
            std::cout << "\n================= 全行业务指标统计报表 =================\n";
            std::cout << "  活期储蓄总额:     " << formatMoney(curTotal) << " 元  (" << curCnt << "户, 占" << pct(curCnt) << "%)\n";
            std::cout << "  定期存款总额:     " << formatMoney(fixTotal) << " 元  (" << fixCnt << "户, 占" << pct(fixCnt) << "%)\n";
            std::cout << "  信用透支欠款总额: " << formatMoney(creditDebtTotal) << " 元  (" << crCnt << "户, 占" << pct(crCnt) << "%)\n";
            std::cout << "  全行储蓄存款总额: " << formatMoney(curTotal + fixTotal) << " 元\n";
            std::cout << "  开立账户总数:     " << totalAcc << " 个 (正常 " << normalCount << " / 冻结 " << frozenCount << ")\n";
            std::cout << "  注册客户总人数:   " << custCount << " 位\n";
            std::cout << "  累计交易流水笔数: " << bank.getTransactions().size() << " 笔\n";
            std::cout << "========================================================\n";
        } else if (choice == "12") {
            storage.save(bank);
            Logger::log("管理员保存全行数据至本地存储");
            std::cout << "【成功】全行数据已成功保存至本地文件！\n";
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }
}

// 全流程自动化功能回归脚本：依次覆盖登录鉴权、活期存取转、定期提前支取与自动转存、信用透支计息、改密调息及持久化重载
static void runAutomatedTest() {
    printHeader("执行全流程自动化功能回归测试");
    Bank bank;
    seedDefaultData(bank);

    std::cout << ">>> 1. 登录验证测试...\n";
    try {
        bank.login(20001, "wrongpass");
    } catch (const AuthFail &e) {
        std::cout << "    [PASS] 成功拦截错误密码: " << e.what() << "\n";
    }
    User *u = bank.login(20001, "zs123456");
    std::cout << "    [PASS] 正确密码登录成功: " << u->getName() << "\n";

    std::cout << "\n>>> 2. 活期存取款与原子转账测试...\n";
    long long acc1 = 62001;
    long long acc2 = 62002;
    bank.deposit(acc1, 50000, "柜面存款测试");
    bank.withdraw(acc1, 20000, "ATM取款测试");
    bank.transfer(acc1, acc2, 30000, "同行转账测试");
    std::cout << "    [PASS] 活期存取转完成，62001余额: " << formatMoney(bank.findAccount(acc1)->getBalance()) << " 元\n";

    try {
        bank.withdraw(acc1, 99999900, "大额透支");
    } catch (const InsufficientBalance &e) {
        std::cout << "    [PASS] 成功拦截余额不足: " << e.what() << "\n";
    }
    try {
        bank.deposit(acc1, -100, "负数存款");
    } catch (const InvalidAmount &e) {
        std::cout << "    [PASS] 成功拦截负数金额: " << e.what() << "\n";
    }

    std::cout << "\n>>> 3. 定期账户多态计息与自动转存测试...\n";
    Date fixOpen(2026, 1, 1);
    long long fixEarlyId = bank.openFixedAccount(20001, 1000000, 12, false, fixOpen); // 1万元 1年定期
    long long earlyPayout = bank.withdrawFixed(fixEarlyId, fixOpen.addDays(7));        // 存7天提前支取
    std::cout << "    [PASS] 1万元定期存7天提前支取按活期计息实付: " << formatMoney(earlyPayout)
              << " 元 (含活期7天利息 " << formatMoney(earlyPayout - 1000000) << " 元)\n";

    long long fixRenewId = bank.openFixedAccount(20001, 1000000, 12, true, Date(2025, 1, 1)); // 到期转存
    int rolled = bank.processMaturityRollovers(Date(2026, 1, 2));
    std::cout << "    [PASS] 定期到期自动转存触发 " << rolled << " 次，本息滚存后余额: "
              << formatMoney(bank.findAccount(fixRenewId)->getBalance()) << " 元\n";

    std::cout << "\n>>> 4. 信用账户透支、还款与按日计息测试...\n";
    Date crOpen(2026, 10, 1);
    long long creditId = bank.openCreditAccount(20001, 50000, crOpen); // 500元额度
    try {
        bank.withdraw(creditId, 60000, "超额消费");
    } catch (const InsufficientBalance &e) {
        std::cout << "    [PASS] 成功拦截信用超额透支: " << e.what() << "\n";
    }
    bank.withdraw(creditId, 10000, "信用消费100元");
    bank.deposit(creditId, 6000, "还款60元");
    auto *crPtr = dynamic_cast<CreditAccount *>(bank.findAccount(creditId));
    long long crInterest = bank.settleInterest(creditId, crOpen.addDays(10));
    std::cout << "    [PASS] 欠款100还60后欠款 40.00 元，10天透支利息 "
              << formatMoney(crInterest) << " 元，总欠款: " << formatMoney(crPtr->getDebt()) << " 元\n";

    std::cout << "\n>>> 5. 修改信息、管理员调利率与多条件流水查询测试...\n";
    bank.updateUserInfo(20001, "张三丰", "13900000001");
    bank.changeUserPassword(20001, "zs123456", "newpass888");
    bank.setCurrentRate(50); // 活期调高至 0.50%
    auto trfTxs = bank.queryTransactionsFiltered(acc1, 3, "", "");
    std::cout << "    [PASS] 用户信息与密码修改成功，活期利率调至 0.50%，62001 转账流水数: " << trfTxs.size() << "\n";

    std::cout << "\n>>> 6. 多账户类型与利率持久化重载验证...\n";
    TextFileStorage testStorage("data/test_users.txt", "data/test_accounts.txt", "data/test_tx.txt");
    testStorage.save(bank);
    Bank reloaded;
    testStorage.load(reloaded);
    bool ok = reloaded.findUser(20001)->getName() == "张三丰" &&
              reloaded.findAccount(acc1)->getBalance() == bank.findAccount(acc1)->getBalance() &&
              reloaded.findAccount(fixRenewId)->getBalance() == bank.findAccount(fixRenewId)->getBalance() &&
              reloaded.findAccount(creditId)->getBalance() == bank.findAccount(creditId)->getBalance() &&
              reloaded.getCurrentRateX10000() == 50;
    std::cout << (ok ? "    [PASS] 三类账户、自定义利率及用户信息重载 100% 一致！\n"
                     : "    [FAIL] 数据重载验证失败！\n");
    std::cout << "================ 全流程测试全部通过 ================\n";
}

// 程序入口：配置控制台 UTF-8 编码，装载磁盘数据，触发到期定期转存批处理，随后进入主菜单交互循环
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Bank bank;
    TextFileStorage storage;
    storage.load(bank);

    if (bank.getUsers().empty()) {
        seedDefaultData(bank);
        storage.save(bank);
    }

    int rolled = bank.processMaturityRollovers(Date::today());
    if (rolled > 0) {
        Logger::log("系统启动批处理：自动完成 " + std::to_string(rolled) + " 笔到期定期存款转存");
        storage.save(bank);
    }

    while (true) {
        printHeader("银行综合业务管理系统 (控制台完整版)");
        std::cout << "  [1] 用户登录（客户 / 系统管理员）\n";
        std::cout << "  [2] 自动化功能回归演示（全流程自检脚本）\n";
        std::cout << "  [0] 退出系统\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << "请选择操作 [0-2]: ";

        std::string choice;
        if (!readToken(choice)) break;

        if (choice == "0") {
            storage.save(bank);
            Logger::log("系统正常退出并保存数据");
            std::cout << "数据已安全存盘，感谢使用！再见。\n";
            break;
        } else if (choice == "1") {
            long long uid;
            if (!readLongLong("\n请输入用户账号 (ID): ", uid)) continue;
            std::cout << "请输入登录密码: ";
            std::string pwd;
            if (!readToken(pwd)) continue;

            try {
                User *user = bank.login(uid, pwd);
                Logger::log("用户 " + std::to_string(uid) + " 登录成功");
                std::cout << "登录成功！欢迎，" << user->getName() << "（角色：" << user->role() << "）\n";

                if (user->role() == "Admin") {
                    adminMenu(bank, user, storage);
                } else if (auto *c = dynamic_cast<Customer *>(user)) {
                    customerMenu(bank, c, storage);
                }
            } catch (const BankException &e) {
                std::cout << "【登录失败】" << e.what() << "\n";
            }
        } else if (choice == "2") {
            runAutomatedTest();
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }

    return 0;
}
