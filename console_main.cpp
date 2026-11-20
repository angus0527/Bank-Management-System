#include <fstream>
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

#ifdef _WIN32
#include <windows.h>
#endif

// 将交易类型枚举转为中文名称（导出 CSV 用）
static std::string txTypeName(TransactionType t) {
    if (t == TransactionType::DEP) return "存款";
    if (t == TransactionType::WDR) return "取款";
    if (t == TransactionType::TRF) return "转账";
    return "结息";
}

// 将账户类型枚举转为中文名称（管理员总览用）
static std::string accTypeName(AccountType t) {
    if (t == AccountType::SAVINGS) return "活期";
    if (t == AccountType::FIXED)   return "定期";
    return "信用";
}

// 判断账号是否属于当前登录客户
static bool hasAccount(const Customer *cust, long long aid) {
    for (long long id : cust->getAccountIds()) {
        if (id == aid) return true;
    }
    return false;
}

// 按账号、类型、日期段筛选并调用 Transaction::display() 输出流水；cust 非空时仅展示本人账户流水
static void showTransactions(const Bank &bank, long long accId, const Customer *cust = nullptr) {
    int typeFilter = 0;
    std::string startD, endD;
    std::cout << "按类型筛选 (0=全部 1=存款 2=取款 3=转账 4=结息): ";
    std::cin >> typeFilter;
    if (typeFilter < 0 || typeFilter > 4) {
        std::cout << "类型编号不合法。\n";
        return;
    }
    std::cout << "起始日期 (如 2026-10-01，输 0 不限): ";
    std::cin >> startD;
    if (startD == "0") startD.clear();
    std::cout << "结束日期 (如 2026-12-31，输 0 不限): ";
    std::cin >> endD;
    if (endD == "0") endD.clear();

    std::vector<const Transaction *> list;
    for (const Transaction *t : bank.queryTransactions(accId, typeFilter, startD, endD)) {
        if (cust != nullptr && !hasAccount(cust, t->getFromAccountId()) && !hasAccount(cust, t->getToAccountId())) {
            continue;
        }
        list.push_back(t);
    }

    std::cout << "\n--- 交易流水（共 " << list.size() << " 条）---\n";
    for (const Transaction *t : list) {
        t->display();
    }
}

// 客户操作菜单
static void customerMenu(Bank &bank, Customer *cust) {
    while (true) {
        std::cout << "\n====== 客户中心 [" << cust->getName() << " ID:" << cust->getId() << "] ======\n";
        std::cout << "1. 查看我的账户        2. 存款 / 信用还款\n";
        std::cout << "3. 取款 / 定期支取     4. 转账\n";
        std::cout << "5. 申请开户            6. 账户结息\n";
        std::cout << "7. 查询交易流水        8. 修改资料 / 改密码\n";
        std::cout << "9. 账户销户            10. 导出对账单(CSV)\n";
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int op = -1;
        if (!(std::cin >> op)) break;
        if (op == 0) break;

        if (op == 1) {
            if (cust->getAccountIds().empty()) {
                std::cout << "名下暂无账户。\n";
                continue;
            }
            for (long long aid : cust->getAccountIds()) {
                Account *a = bank.findAccount(aid);
                if (a != nullptr) a->displayInfo();
            }
        } else if (op == 2) {
            long long aid;
            double amount;
            std::cout << "账号: ";
            std::cin >> aid;
            std::cout << "存入/还款金额(元): ";
            std::cin >> amount;
            if (bank.deposit(aid, amount, "柜面存入/还款")) {
                Logger::log("账户 " + std::to_string(aid) + " 存入 " + formatMoney(amount) + " 元");
                std::cout << "操作成功，当前余额: " << formatMoney(bank.findAccount(aid)->getBalance()) << " 元\n";
            }
        } else if (op == 3) {
            long long aid;
            std::cout << "账号: ";
            std::cin >> aid;
            if (!hasAccount(cust, aid)) {
                std::cout << "只能操作本人名下账户。\n";
                continue;
            }
            Account *a = bank.findAccount(aid);
            if (a == nullptr) {
                std::cout << "账户不存在。\n";
                continue;
            }
            if (a->getAccountType() == AccountType::FIXED) {
                double payout = bank.withdrawFixed(aid);
                if (payout >= 0.0) {
                    Logger::log("定期账户 " + std::to_string(aid) + " 支取 " + formatMoney(payout) + " 元");
                    std::cout << "定期全额支取成功，实付本息: " << formatMoney(payout) << " 元\n";
                }
            } else {
                double amount;
                std::cout << "取款/消费金额(元): ";
                std::cin >> amount;
                if (bank.withdraw(aid, amount, "取款/消费")) {
                    Logger::log("账户 " + std::to_string(aid) + " 支出 " + formatMoney(amount) + " 元");
                    std::cout << "操作成功，当前余额: " << formatMoney(a->getBalance()) << " 元\n";
                }
            }
        } else if (op == 4) {
            long long fromId, toId;
            double amount;
            std::cout << "付款账号: ";
            std::cin >> fromId;
            std::cout << "收款账号: ";
            std::cin >> toId;
            std::cout << "转账金额(元): ";
            std::cin >> amount;
            if (!hasAccount(cust, fromId)) {
                std::cout << "只能从本人名下账户转出。\n";
                continue;
            }
            if (bank.transfer(fromId, toId, amount, "同行转账")) {
                Logger::log("转账 " + std::to_string(fromId) + " -> " + std::to_string(toId) + " " + formatMoney(amount) + " 元");
                std::cout << "转账成功！付款账户余额: " << formatMoney(bank.findAccount(fromId)->getBalance()) << " 元\n";
            }
        } else if (op == 5) {
            int kind = 0;
            std::cout << "账户类型 (1=活期 2=定期 3=信用): ";
            std::cin >> kind;
            if (kind == 1) {
                double amount;
                std::cout << "初始存款(元): ";
                std::cin >> amount;
                long long nid = bank.openAccount(cust->getId(), amount);
                if (nid > 0) {
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立活期账户 " + std::to_string(nid));
                    std::cout << "开户成功，账号: " << nid << "\n";
                }
            } else if (kind == 2) {
                int term, renew;
                double amount;
                std::cout << "存期月数 (3/12/36): ";
                std::cin >> term;
                std::cout << "定期存入金额(元): ";
                std::cin >> amount;
                std::cout << "到期自动转存? (1=是 0=否): ";
                std::cin >> renew;
                long long nid = bank.openFixedAccount(cust->getId(), amount, term, renew == 1);
                if (nid > 0) {
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立定期账户 " + std::to_string(nid));
                    std::cout << "定期开户成功，账号: " << nid << "\n";
                }
            } else if (kind == 3) {
                double limit;
                std::cout << "申请信用额度(元): ";
                std::cin >> limit;
                long long nid = bank.openCreditAccount(cust->getId(), limit);
                if (nid > 0) {
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立信用账户 " + std::to_string(nid));
                    std::cout << "信用账户开户成功，账号: " << nid << "\n";
                }
            } else {
                std::cout << "无效的账户类型。\n";
            }
        } else if (op == 6) {
            long long aid;
            std::cout << "结息账号: ";
            std::cin >> aid;
            if (!hasAccount(cust, aid)) {
                std::cout << "只能操作本人名下账户。\n";
                continue;
            }
            double interest = bank.settleInterest(aid);
            if (interest >= 0.0) {
                std::cout << "结息完成，本次利息发生额: " << formatMoney(interest) << " 元\n";
            }
        } else if (op == 7) {
            long long aid;
            std::cout << "查询账号 (0=本人全部): ";
            std::cin >> aid;
            if (aid != 0 && !hasAccount(cust, aid)) {
                std::cout << "只能查询本人名下账户流水。\n";
                continue;
            }
            showTransactions(bank, aid, cust);
        } else if (op == 8) {
            int sub = 0;
            std::cout << "1=修改姓名电话  2=修改密码: ";
            std::cin >> sub;
            if (sub == 1) {
                std::string name, phone;
                std::cout << "新姓名: ";
                std::cin >> name;
                std::cout << "新电话: ";
                std::cin >> phone;
                if (bank.updateUserInfo(cust->getId(), name, phone)) {
                    std::cout << "资料已更新。\n";
                }
            } else if (sub == 2) {
                std::string oldP, newP;
                std::cout << "原密码: ";
                std::cin >> oldP;
                std::cout << "新密码: ";
                std::cin >> newP;
                if (bank.changeUserPassword(cust->getId(), oldP, newP)) {
                    std::cout << "密码修改成功。\n";
                }
            }
        } else if (op == 9) {
            long long aid;
            std::cout << "要销户的账号: ";
            std::cin >> aid;
            if (!hasAccount(cust, aid)) {
                std::cout << "只能操作本人名下账户。\n";
                continue;
            }
            if (bank.closeAccount(aid)) {
                Logger::log("销户 " + std::to_string(aid));
                std::cout << "销户成功。\n";
            }
        } else if (op == 10) {
            long long aid;
            std::cout << "导出对账单账号: ";
            std::cin >> aid;
            if (!hasAccount(cust, aid)) {
                std::cout << "只能导出本人名下账户对账单。\n";
                continue;
            }
            std::string fname = "statement_" + std::to_string(aid) + ".csv";
            std::ofstream out(fname);
            out << "流水号,时间,类型,付款,收款,金额,余额快照,备注\n";
            for (const Transaction *t : bank.queryTransactions(aid)) {
                out << t->getId() << "," << t->getDatetime() << "," << txTypeName(t->getType()) << ","
                    << t->getFromAccountId() << "," << t->getToAccountId() << ","
                    << formatMoney(t->getAmount()) << "," << formatMoney(t->getBalanceSnapshot()) << ","
                    << t->getRemark() << "\n";
            }
            std::cout << "已导出到 " << fname << "\n";
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }
}

// 管理员控制台菜单
static void adminMenu(Bank &bank, User *admin) {
    while (true) {
        std::cout << "\n====== 管理员后台 [" << admin->getName() << "] ======\n";
        std::cout << "1. 账户总览与资金统计  2. 客户名册\n";
        std::cout << "3. 冻结 / 解冻账户     4. 全行一键结息\n";
        std::cout << "5. 调整各档利率        6. 查看流水 / 操作日志\n";
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int op = -1;
        if (!(std::cin >> op)) break;
        if (op == 0) break;

        if (op == 1) {
            double curSum = 0.0, fixSum = 0.0, crDebt = 0.0;
            int curN = 0, fixN = 0, crN = 0;
            for (const auto &pair : bank.getAccounts()) {
                long long aid = pair.first;
                const Account *acc = pair.second.get();
                std::cout << "账号:" << aid << " | 户主:" << acc->getOwnerId()
                          << " | " << accTypeName(acc->getAccountType())
                          << " | 余额:" << formatMoney(acc->getBalance()) << "元 | " << acc->getStatusString() << "\n";
                if (acc->getAccountType() == AccountType::SAVINGS) { curN++; curSum += acc->getBalance(); }
                else if (acc->getAccountType() == AccountType::FIXED) { fixN++; fixSum += acc->getBalance(); }
                else if (const CreditAccount *c = dynamic_cast<const CreditAccount *>(acc)) { crN++; crDebt += c->getDebt(); }
            }
            std::cout << "--- 统计: 活期 " << curN << "户(" << formatMoney(curSum) << "元), "
                      << "定期 " << fixN << "户(" << formatMoney(fixSum) << "元), "
                      << "信用 " << crN << "户(总欠款" << formatMoney(crDebt) << "元) ---\n";
        } else if (op == 2) {
            for (const auto &pair : bank.getUsers()) {
                long long uid = pair.first;
                const User *u = pair.second.get();
                std::cout << "ID:" << uid << " | " << u->getName() << " | " << u->role();
                if (const Customer *c = dynamic_cast<const Customer *>(u)) {
                    std::cout << " | 电话:" << c->getPhoneNumber() << " | 卡数:" << c->getAccountIds().size();
                }
                std::cout << "\n";
            }
        } else if (op == 3) {
            int mode;
            long long aid;
            std::cout << "1=冻结  2=解冻: ";
            std::cin >> mode;
            std::cout << "账号: ";
            std::cin >> aid;
            if (mode == 1 && bank.freezeAccount(aid)) {
                Logger::log("管理员冻结账户 " + std::to_string(aid));
                std::cout << "已冻结。\n";
            } else if (mode == 2 && bank.unfreezeAccount(aid)) {
                Logger::log("管理员解冻账户 " + std::to_string(aid));
                std::cout << "已解冻。\n";
            }
        } else if (op == 4) {
            int rolled = bank.processMaturityRollovers();
            double sumInt = 0.0;
            for (const auto &pair : bank.getAccounts()) {
                if (pair.second->isNormal()) {
                    double interest = bank.settleInterest(pair.first);
                    if (interest > 0.0) sumInt += interest;
                }
            }
            std::cout << "全行结息完成：自动转存 " << rolled << " 笔，累计利息 " << formatMoney(sumInt) << " 元\n";
        } else if (op == 5) {
            std::cout << "当前利率: 1=活期(" << formatMoney(bank.getCurrentRate() * 100.0)
                      << "%) 2=定3月(" << formatMoney(bank.getFixedRate(3) * 100.0)
                      << "%) 3=定1年(" << formatMoney(bank.getFixedRate(12) * 100.0)
                      << "%) 4=定3年(" << formatMoney(bank.getFixedRate(36) * 100.0)
                      << "%) 5=信用日息(" << formatMoney(bank.getCreditDailyRate() * 100.0) << "%)\n";
            int tier;
            double pct;
            std::cout << "修改哪一档 [1-5]: ";
            std::cin >> tier;
            std::cout << "输入新利率百分比(如 1.75 代表 1.75%): ";
            std::cin >> pct;
            double newRate = pct / 100.0;
            bool ok = false;
            if (tier == 1) ok = bank.setCurrentRate(newRate);
            else if (tier == 2) ok = bank.setFixedRate(3, newRate);
            else if (tier == 3) ok = bank.setFixedRate(12, newRate);
            else if (tier == 4) ok = bank.setFixedRate(36, newRate);
            else if (tier == 5) ok = bank.setCreditDailyRate(newRate);
            else std::cout << "档位编号不合法。\n";
            if (ok) {
                Logger::log("管理员调整利率档位 " + std::to_string(tier));
                std::cout << "利率已更新并即时生效。\n";
            }
        } else if (op == 6) {
            int sub = 0;
            std::cout << "1=查看流水  2=查看操作日志: ";
            std::cin >> sub;
            if (sub == 1) {
                long long aid;
                std::cout << "筛选账号 (0=全行): ";
                std::cin >> aid;
                showTransactions(bank, aid);
            } else if (sub == 2) {
                for (const std::string &line : Logger::getAllLogs()) std::cout << line << "\n";
            }
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }
}

// 程序主入口：初始化读档、自动处理到期定期转存、进入登录/注册主菜单
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Bank bank;
    TextFileStorage storage;
    storage.load(bank);
    bank.processMaturityRollovers(Date::today());

    while (true) {
        std::cout << "\n========= 银行账户管理系统 =========\n";
        std::cout << "1. 登录系统\n2. 注册新客户\n0. 保存并退出\n";
        std::cout << "请选择: ";
        int op = -1;
        if (!(std::cin >> op)) break;

        if (op == 0) {
            storage.save(bank);
            Logger::log("系统退出并保存");
            std::cout << "数据已保存，再见！\n";
            break;
        } else if (op == 1) {
            long long uid;
            std::string pwd;
            std::cout << "用户ID: ";
            std::cin >> uid;
            std::cout << "密码: ";
            std::cin >> pwd;
            User *u = bank.login(uid, pwd);
            if (u != nullptr) {
                Logger::log("用户 " + std::to_string(uid) + " 登录");
                if (u->role() == "Admin") adminMenu(bank, u);
                else if (Customer *c = dynamic_cast<Customer *>(u)) customerMenu(bank, c);
            }
        } else if (op == 2) {
            std::string name, pwd, phone;
            std::cout << "姓名: ";
            std::cin >> name;
            std::cout << "密码: ";
            std::cin >> pwd;
            std::cout << "电话: ";
            std::cin >> phone;
            long long nid = bank.registerCustomer(name, pwd, phone);
            if (nid > 0) {
                storage.save(bank);
                Logger::log("新客户注册 ID:" + std::to_string(nid) + " 姓名:" + name);
                std::cout << "注册成功！您的用户ID为: " << nid << "（请牢记，用于登录）\n";
            }
        } else {
            std::cout << "无效选项，请重新输入。\n";
        }
    }
    return 0;
}
