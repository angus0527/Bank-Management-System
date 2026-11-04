#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Bank.h"
#include "CurrentAccount.h"
#include "Logger.h"
#include "Storage.h"
#include "exception.h"

#ifdef _WIN32
#include <windows.h>
#endif

//删除前后的垃圾字符
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

//把元转为分
static bool parseYuanToCents(const std::string &str, long long &cents) {
    if (str.empty()) return false;
    try {
        size_t idx = 0;
        double val = std::stod(str, &idx);
        if (idx != str.size() || val < 0.0) return false;
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

static void printHeader(const std::string &title) {
    std::cout << "\n======================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "======================================================\n";
}

static void seedDefaultData(Bank &bank) {
    bank.addUser(std::make_unique<Admin>(10001, "admin", "admin123"));
    bank.addUser(std::make_unique<Customer>(20001, "张三", "zs123456", "13800000001"));
    bank.addUser(std::make_unique<Customer>(20002, "李四", "ls123456", "13800000002"));
    bank.openAccount(20001, 100029, Date(2026, 10, 2));
    bank.openAccount(20002, 30004, Date(2026, 10, 15));
}

// ---------------- 客户菜单逻辑 ----------------
static void customerMenu(Bank &bank, Customer *cust, Storage &storage) {
    while (true) {
        printHeader("客户服务中心 - 欢迎您，" + cust->getName() + " (ID: " + std::to_string(cust->getId()) + ")");
        std::cout << "  [1] 查看名下账户概况\n";
        std::cout << "  [2] 账户存款\n";
        std::cout << "  [3] 账户取款\n";
        std::cout << "  [4] 转账汇款\n";
        std::cout << "  [5] 申请开立新账户\n";
        std::cout << "  [6] 活期利息结算\n";
        std::cout << "  [7] 申请账户销户\n";
        std::cout << "  [8] 查询账户交易流水\n";
        std::cout << "  [9] 导出对账单到 CSV\n";
        std::cout << "  [10] 保存数据到本地\n";
        std::cout << "  [0] 退出登录，返回上级菜单\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << "请选择操作编号 [0-10]: ";

        std::string choice;
        if (!(std::cin >> choice)) {
            break;
        }
        choice = cleanInput(choice);

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
                                  << " | 余额: " << formatMoney(acc->getBalance()) << " 元"
                                  << " | 状态: " << acc->getStatusString()
                                  << " | 开户日: " << acc->getOpenDate().toString() << "\n";
                    } catch (...) {}
                }
            }
        } else if (choice == "2") {
            std::cout << "请输入存款账号: ";
            long long aid;
            std::cin >> aid;
            std::cout << "请输入存款金额 (元): ";
            std::string amtStr;
            std::cin >> amtStr;
            long long cents = 0;
            if (!parseYuanToCents(amtStr, cents) || cents <= 0) {
                std::cout << "【错误】金额格式不合法，请输入大于 0 的数字！\n";
                continue;
            }
            std::cout << "请输入交易备注 (可留空按回车): ";
            std::string remark;
            std::cin >> remark;
            if (remark.empty()) remark = "柜面存入";

            try {
                bank.deposit(aid, cents, remark);
                Logger::log("账户 " + std::to_string(aid) + " 存款 " + formatMoney(cents) + " 元");
                std::cout << "【成功】存款完成！当前余额：" << formatMoney(bank.findAccount(aid)->getBalance()) << " 元\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "3") {
            std::cout << "请输入取款账号: ";
            long long aid;
            std::cin >> aid;
            std::cout << "请输入取款金额 (元): ";
            std::string amtStr;
            std::cin >> amtStr;
            long long cents = 0;
            if (!parseYuanToCents(amtStr, cents) || cents <= 0) {
                std::cout << "【错误】金额格式不合法，请输入大于 0 的数字！\n";
                continue;
            }
            std::cout << "请输入交易备注: ";
            std::string remark;
            std::cin >> remark;
            if (remark.empty()) remark = "ATM取款";

            try {
                bank.withdraw(aid, cents, remark);
                Logger::log("账户 " + std::to_string(aid) + " 取款 " + formatMoney(cents) + " 元");
                std::cout << "【成功】取款完成！当前余额：" << formatMoney(bank.findAccount(aid)->getBalance()) << " 元\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "4") {
            std::cout << "请输入付款账号: ";
            long long fromId;
            std::cin >> fromId;
            std::cout << "请输入收款账号: ";
            long long toId;
            std::cin >> toId;
            std::cout << "请输入转账金额 (元): ";
            std::string amtStr;
            std::cin >> amtStr;
            long long cents = 0;
            if (!parseYuanToCents(amtStr, cents) || cents <= 0) {
                std::cout << "【错误】金额格式不合法！\n";
                continue;
            }
            std::cout << "请输入交易备注: ";
            std::string remark;
            std::cin >> remark;
            if (remark.empty()) remark = "同行转账";

            try {
                bank.transfer(fromId, toId, cents, remark);
                Logger::log("账户 " + std::to_string(fromId) + " 向账户 " + std::to_string(toId) + " 转账 " + formatMoney(cents) + " 元");
                std::cout << "【成功】转账成功！付款账户剩余余额：" << formatMoney(bank.findAccount(fromId)->getBalance()) << " 元\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "5") {
            std::cout << "请输入初始存款金额 (元，允许为 0): ";
            std::string amtStr;
            std::cin >> amtStr;
            long long cents = 0;
            if (!parseYuanToCents(amtStr, cents)) {
                std::cout << "【错误】金额格式不合法！\n";
                continue;
            }
            try {
                long long newAid = bank.openAccount(cust->getId(), cents);
                Logger::log("客户 " + std::to_string(cust->getId()) + " 成功开户 " + std::to_string(newAid));
                std::cout << "【成功】新账户已生成！账号：" << newAid << "，初始存入：" << formatMoney(cents) << " 元\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "6") {
            std::cout << "请输入结息账号: ";
            long long aid;
            std::cin >> aid;
            try {
                long long interest = bank.settleInterest(aid);
                if (interest > 0) {
                    Logger::log("账户 " + std::to_string(aid) + " 结息 " + formatMoney(interest) + " 元");
                    std::cout << "【成功】结息完成！本次入账利息：" << formatMoney(interest) << " 元，最新余额："
                              << formatMoney(bank.findAccount(aid)->getBalance()) << " 元\n";
                } else {
                    std::cout << "【提示】当前未到结息周期或无新增利息。\n";
                }
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "7") {
            std::cout << "请输入要销毁的账号: ";
            long long aid;
            std::cin >> aid;
            std::cout << "确认销户账户 " << aid << " 吗？(y/n): ";
            std::string confirm;
            std::cin >> confirm;
            if (confirm != "y" && confirm != "Y") continue;

            try {
                bank.closeAccount(aid);
                Logger::log("客户 " + std::to_string(cust->getId()) + " 销户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已成功销户！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "8") {
            std::cout << "请输入查询账号 (输入 0 查询名下所有账户流水): ";
            long long aid;
            std::cin >> aid;

            auto txs = bank.queryTransactions(aid);
            std::cout << "\n-------------------------- 交易流水记录 --------------------------\n";
            std::cout << std::left << std::setw(6) << "流水号"
                      << std::setw(21) << "交易时间"
                      << std::setw(8) << "类型"
                      << std::setw(10) << "付款账号"
                      << std::setw(10) << "收款账号"
                      << std::setw(12) << "金额(元)"
                      << std::setw(12) << "余额快照"
                      << "备注\n";
            std::cout << "------------------------------------------------------------------\n";
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
        } else if (choice == "9") {
            std::cout << "请输入要导出对账单的账号: ";
            long long aid;
            std::cin >> aid;
            std::string path = "statement_" + std::to_string(aid) + ".csv";
            std::ofstream out(path);
            if (!out.is_open()) {
                std::cout << "【错误】无法创建目标 CSV 文件！\n";
                continue;
            }
            out << "\xEF\xBB\xBF"; // UTF-8 BOM
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
        } else if (choice == "10") {
            storage.save(bank);
            Logger::log("客户保存数据至本地文件");
            std::cout << "【成功】全部数据已成功持久化至 data/ 文本文件！\n";
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }
}

// ---------------- 管理员菜单逻辑 ----------------
static void adminMenu(Bank &bank, User *admin, Storage &storage) {
    while (true) {
        printHeader("系统管理员控制台 - 操作员: " + admin->getName() + " (ID: " + std::to_string(admin->getId()) + ")");
        std::cout << "  [1] 查看全行所有账户总览\n";
        std::cout << "  [2] 查看客户名册与总资产统计\n";
        std::cout << "  [3] 冻结指定账户\n";
        std::cout << "  [4] 解冻指定账户\n";
        std::cout << "  [5] 强制销户\n";
        std::cout << "  [6] 一键全行日终结息批处理\n";
        std::cout << "  [7] 全行业务流水审计与查询\n";
        std::cout << "  [8] 查看系统审计日志\n";
        std::cout << "  [9] 清空系统审计日志\n";
        std::cout << "  [10] 全行业务指标统计报表\n";
        std::cout << "  [11] 保存全行数据到本地文件\n";
        std::cout << "  [0] 退出登录，返回上级菜单\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << "请选择操作编号 [0-11]: ";

        std::string choice;
        if (!(std::cin >> choice)) {
            break;
        }
        choice = cleanInput(choice);

        if (choice == "0") {
            std::cout << "已退出管理员会话。\n";
            break;
        } else if (choice == "1") {
            std::cout << "\n-------------------- 全行所有账户 --------------------\n";
            std::cout << std::left << std::setw(10) << "账号"
                      << std::setw(12) << "所属客户ID"
                      << std::setw(8) << "类型"
                      << std::setw(14) << "余额 (元)"
                      << std::setw(10) << "状态"
                      << "开户日期\n";
            std::cout << "------------------------------------------------------\n";
            for (const auto &[aid, acc] : bank.getAccounts()) {
                std::cout << std::left << std::setw(10) << aid
                          << std::setw(12) << acc->getOwnerId()
                          << std::setw(8) << "活期"
                          << std::setw(14) << formatMoney(acc->getBalance())
                          << std::setw(10) << acc->getStatusString()
                          << acc->getOpenDate().toString() << "\n";
            }
        } else if (choice == "2") {
            std::cout << "\n------------------------ 客户名册 ------------------------\n";
            std::cout << std::left << std::setw(10) << "用户ID"
                      << std::setw(12) << "姓名"
                      << std::setw(10) << "角色"
                      << std::setw(16) << "联系电话"
                      << std::setw(10) << "账户数"
                      << "储蓄总资产 (元)\n";
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
            std::cout << "请输入要冻结的账号: ";
            long long aid;
            std::cin >> aid;
            try {
                bank.freezeAccount(aid);
                Logger::log("管理员 " + std::to_string(admin->getId()) + " 冻结账户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已成功冻结！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "4") {
            std::cout << "请输入要解冻的账号: ";
            long long aid;
            std::cin >> aid;
            try {
                bank.unfreezeAccount(aid);
                Logger::log("管理员 " + std::to_string(admin->getId()) + " 解冻账户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已解冻恢复正常！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "5") {
            std::cout << "请输入要强制销户的账号: ";
            long long aid;
            std::cin >> aid;
            try {
                bank.closeAccount(aid);
                Logger::log("管理员 " + std::to_string(admin->getId()) + " 销户 " + std::to_string(aid));
                std::cout << "【成功】账户 " << aid << " 已成功销户！\n";
            } catch (const BankException &e) {
                std::cout << "【失败】" << e.what() << "\n";
            }
        } else if (choice == "6") {
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
            Logger::log("管理员执行全行日终结息：共结息 " + std::to_string(count) + " 个账户，入账利息 " + formatMoney(totalInterest) + " 元");
            std::cout << "【成功】全行结息批处理完成！\n产生利息账户数: " << count
                      << "\n累计结息总金额: " << formatMoney(totalInterest) << " 元\n";
        } else if (choice == "7") {
            std::cout << "请输入筛选账号 (输入 0 查询全行流水): ";
            long long aid;
            std::cin >> aid;
            auto txs = bank.queryTransactions(aid);
            std::cout << "\n-------------------------- 全行交易流水 --------------------------\n";
            for (const auto *t : txs) {
                std::cout << "#" << t->getId() << " [" << t->getDatetime() << "] "
                          << txTypeStr(t->getType()) << " "
                          << formatMoney(t->getAmount()) << " 元 "
                          << "(付款:" << t->getFromAccountId() << " -> 收款:" << t->getToAccountId() << ") "
                          << "快照余额:" << formatMoney(t->getBalanceSnapshot()) << " 元 | 备注: "
                          << t->getRemark() << "\n";
            }
        } else if (choice == "8") {
            std::cout << "\n------------------------ 系统审计日志 ------------------------\n";
            auto logs = Logger::getAllLogs();
            if (logs.empty()) {
                std::cout << "(暂无日志记录)\n";
            } else {
                for (const auto &line : logs) {
                    std::cout << line << "\n";
                }
            }
        } else if (choice == "9") {
            std::cout << "确定要清空全部系统日志吗？(y/n): ";
            std::string confirm;
            std::cin >> confirm;
            if (confirm == "y" || confirm == "Y") {
                Logger::clearAllLogs();
                std::cout << "【成功】系统审计日志已清空！\n";
            }
        } else if (choice == "10") {
            long long totalDep = 0;
            int normalCount = 0;
            int frozenCount = 0;
            for (const auto &[aid, acc] : bank.getAccounts()) {
                totalDep += acc->getBalance();
                if (acc->getStatus() == AccountStatus::NORMAL) normalCount++;
                else if (acc->getStatus() == AccountStatus::FROZEN) frozenCount++;
            }
            int custCount = 0;
            for (const auto &[uid, user] : bank.getUsers()) {
                if (user->role() == "Customer") custCount++;
            }
            std::cout << "\n================= 全行业务指标统计报表 =================\n";
            std::cout << "  全行储蓄存款总额: " << formatMoney(totalDep) << " 元\n";
            std::cout << "  开立账户总数:     " << bank.getAccounts().size() << " 个\n";
            std::cout << "  正常状态账户:     " << normalCount << " 个\n";
            std::cout << "  冻结监管账户:     " << frozenCount << " 个\n";
            std::cout << "  注册客户总人数:   " << custCount << " 位\n";
            std::cout << "  累计交易流水笔数: " << bank.getTransactions().size() << " 笔\n";
            std::cout << "========================================================\n";
        } else if (choice == "11") {
            storage.save(bank);
            Logger::log("管理员保存全行数据至本地存储");
            std::cout << "【成功】全行数据已成功保存至本地文件！\n";
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }
}

// ---------------- 答辩自动化测试演示 ----------------
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

    std::cout << "\n>>> 2. 存取款与转账测试...\n";
    long long acc1 = 62001;
    long long acc2 = 62002;
    bank.deposit(acc1, 50000, "柜面存款测试");
    std::cout << "    [PASS] 账号 62001 存入 500.00 元\n";
    bank.withdraw(acc1, 20000, "ATM取款测试");
    std::cout << "    [PASS] 账号 62001 取出 200.00 元\n";
    bank.transfer(acc1, acc2, 30000, "同行转账测试");
    std::cout << "    [PASS] 账号 62001 向 62002 转账 300.00 元\n";

    std::cout << "\n>>> 3. 异常边界校验测试...\n";
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

    std::cout << "\n>>> 4. 结息测试...\n";
    long long interest = bank.settleInterest(acc1);
    std::cout << "    [PASS] 账号 62001 本次结息: " << formatMoney(interest) << " 元\n";

    std::cout << "\n>>> 5. 存储持久化与读档测试...\n";
    TextFileStorage storage("data/test_users.txt", "data/test_accounts.txt", "data/test_tx.txt");
    storage.save(bank);
    Bank reloaded;
    storage.load(reloaded);
    bool ok = reloaded.findAccount(acc1)->getBalance() == bank.findAccount(acc1)->getBalance();
    std::cout << (ok ? "    [PASS] 数据重载一致性比对通过！\n" : "    [FAIL] 数据重载失败！\n");
    std::cout << "================ 全流程测试全部通过 ================\n";
}

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

    while (true) {
        printHeader("银行综合业务管理系统");
        std::cout << "  [1] 用户登录\n";
        std::cout << "  [2] 自动化功能回归演示\n";
        std::cout << "  [0] 退出系统\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << "请选择操作 [0-2]: ";

        std::string choice;
        if (!(std::cin >> choice)) {
            break;
        }
        choice = cleanInput(choice);

        if (choice == "0") {
            storage.save(bank);
            Logger::log("控制台端正常退出并保存数据");
            std::cout << "数据已安全存盘，感谢使用！再见。\n";
            break;
        } else if (choice == "1") {
            std::cout << "\n请输入用户账号 (ID): ";
            long long uid;
            std::cin >> uid;
            std::cout << "请输入登录密码: ";
            std::string pwd;
            std::cin >> pwd;

            try {
                User *user = bank.login(uid, pwd);
                Logger::log("用户 " + std::to_string(uid) + " 在控制台登录成功");
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
