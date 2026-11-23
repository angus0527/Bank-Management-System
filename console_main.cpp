#include <fstream>
#include <iostream>
#include <string>

#include "Bank.h"
#include "ConsoleUI.h"
#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "Date.h"
#include "FixedAccount.h"
#include "Logger.h"
#include "Storage.h"

#ifdef _WIN32
#include <windows.h>
#endif

// 客户操作菜单
static void customerMenu(Bank &bank, Customer *cust) {
    while (true) {
        std::cout << "\n====== 客户中心 [" << cust->getName() << " ID:" << cust->getId() << "] ======\n";
        std::cout << "1. 查看我的账户        2. 存款 / 信用还款\n";
        std::cout << "3. 取款 / 定期支取     4. 转账\n";
        std::cout << "5. 申请开户            6. 账户结息\n";
        std::cout << "7. 查询交易流水        8. 修改资料 \n";
        std::cout << "9. 账户销户            10. 导出对账单\n";
        std::cout << "0. 退出登录\n";
        std::cout << "请选择: ";

        int op = -1;
        if (!(std::cin >> op)) {
            if (std::cin.eof()) break;
            std::cin.clear();
            std::cin.ignore(1024, '\n');
            std::cout << "输入格式有误，请输入数字。\n";
            continue;
        }
        if (op == 0) break;

        switch (op) {
            case 1: {
                if (cust->getAccountIds().empty()) {
                    std::cout << "名下暂无账户。\n";
                    break;
                }
                for (long long aid : cust->getAccountIds()) {
                    Account *a = bank.findAccount(aid);
                    if (a != nullptr) a->displayInfo();
                }
                break;
            }
            case 2: {
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
                break;
            }
            case 3: {
                long long aid;
                std::cout << "账号: ";
                std::cin >> aid;
                if (!cust->hasAccount(aid)) {
                    std::cout << "只能操作本人名下账户。\n";
                    break;
                }
                Account *a = bank.findAccount(aid);
                if (a == nullptr) {
                    std::cout << "账户不存在。\n";
                    break;
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
                break;
            }
            case 4: {
                long long fromId, toId;
                double amount;
                std::cout << "付款账号: ";
                std::cin >> fromId;
                std::cout << "收款账号: ";
                std::cin >> toId;
                std::cout << "转账金额(元): ";
                std::cin >> amount;
                if (!cust->hasAccount(fromId)) {
                    std::cout << "只能从本人名下账户转出。\n";
                    break;
                }
                if (bank.transfer(fromId, toId, amount, "同行转账")) {
                    Logger::log("转账 " + std::to_string(fromId) + " -> " + std::to_string(toId) + " " + formatMoney(amount) + " 元");
                    std::cout << "转账成功！付款账户余额: " << formatMoney(bank.findAccount(fromId)->getBalance()) << " 元\n";
                }
                break;
            }
            case 5: {
                int kind = 0;
                std::cout << "账户类型 (1=活期 2=定期 3=信用): ";
                std::cin >> kind;
                if (kind == 1) {
                    double amount;
                    std::cout << "初始存款(元): ";
                    std::cin >> amount;
                    long long nid = bank.openCurrentAccount(cust->getId(), amount);
                    if (nid > 0) {
                        Logger::log("客户 " + std::to_string(cust->getId()) + " 开立活期账户 " + std::to_string(nid));
                        std::cout << "开户成功，账号: " << nid << "\n";
                    }
                } else if (kind == 2) {
                    int term, renew;
                    double amount;
                    std::cout << "存期月数 (3/12/36): ";
                    std::cin >> term;
                    if (term != 3 && term != 12 && term != 36) {
                        std::cout << "操作失败：定期存期只支持 3 / 12 / 36 个月。\n";
                        break;
                    }
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
                break;
            }
            case 6: {
                long long aid;
                std::cout << "结息账号: ";
                std::cin >> aid;
                if (!cust->hasAccount(aid)) {
                    std::cout << "只能操作本人名下账户。\n";
                    break;
                }
                double interest = bank.settleInterest(aid);
                if (interest >= 0.0) {
                    std::cout << "结息完成，本次利息发生额: " << formatMoney(interest) << " 元\n";
                }
                break;
            }
            case 7: {
                long long aid;
                std::cout << "查询账号 (0=本人全部): ";
                std::cin >> aid;
                if (aid != 0 && !cust->hasAccount(aid)) {
                    std::cout << "只能查询本人名下账户流水。\n";
                    break;
                }
                showCustomerTransactions(bank, cust, aid);
                break;
            }
            case 8: {
                int sub = 0;
                std::cout << "1=修改姓名  2=修改电话  3=修改密码: ";
                std::cin >> sub;
                switch (sub) {
                    case 1: {
                        std::string name;
                        std::cout << "新姓名: ";
                        std::cin >> name;
                        if (bank.updateUserName(cust->getId(), name)) {
                            std::cout << "姓名已更新。\n";
                        }
                        break;
                    }
                    case 2: {
                        std::string phone;
                        std::cout << "新电话: ";
                        std::cin >> phone;
                        if (bank.updateCustomerPhone(cust->getId(), phone)) {
                            std::cout << "电话已更新。\n";
                        }
                        break;
                    }
                    case 3: {
                        std::string oldP, newP;
                        std::cout << "原密码: ";
                        std::cin >> oldP;
                        std::cout << "新密码: ";
                        std::cin >> newP;
                        if (bank.changeUserPassword(cust->getId(), oldP, newP)) {
                            std::cout << "密码修改成功。\n";
                        }
                        break;
                    }
                    default:
                        std::cout << "无效选项。\n";
                        break;
                }
                break;
            }
            case 9: {
                long long aid;
                std::cout << "要销户的账号: ";
                std::cin >> aid;
                if (!cust->hasAccount(aid)) {
                    std::cout << "只能操作本人名下账户。\n";
                    break;
                }
                if (bank.closeAccount(aid)) {
                    Logger::log("销户 " + std::to_string(aid));
                    std::cout << "销户成功。\n";
                }
                break;
            }
            case 10: {
                long long aid;
                std::cout << "导出对账单账号: ";
                std::cin >> aid;
                if (!cust->hasAccount(aid)) {
                    std::cout << "只能导出本人名下账户对账单。\n";
                    break;
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
                break;
            }
            default:
                std::cout << "无效选项，请重新输入。\n";
                break;
        }
    }
}

// 管理员控制台菜单
static void adminMenu(Bank &bank, User *admin) {
    while (true) {
        std::cout << "\n====== 管理员后台 [" << admin->getName() << "] ======\n";
        std::cout << "1. 账户总览与资金统计  2. 客户名册\n";
        std::cout << "3. 冻结 / 解冻账户     4. 全行结息\n";
        std::cout << "5. 调整各档利率        6. 查看交易流水\n";
        std::cout << "7. 查看操作日志        0. 退出登录\n";
        std::cout << "请选择: ";

        int op = -1;
        if (!(std::cin >> op)) {
            if (std::cin.eof()) break;
            std::cin.clear();
            std::cin.ignore(1024, '\n');
            std::cout << "输入格式有误，请输入数字。\n";
            continue;
        }
        if (op == 0) break;

        switch (op) {
            case 1: {
                double curSum = 0.0, fixSum = 0.0, crDebt = 0.0;
                int curN = 0, fixN = 0, crN = 0, closedN = 0;
                for (const auto &[aid, acc] : bank.getAccounts()) {
                    std::cout << "账号:" << aid << " | 户主:" << acc->getOwnerId()
                              << " | " << accTypeName(acc->getAccountType())
                              << " | 余额:" << formatMoney(acc->getBalance()) << "元 | " << acc->getStatusString() << "\n";
                    if (acc->getStatus() == AccountStatus::CLOSED) {
                        closedN++;
                        continue;
                    }
                    if (acc->getAccountType() == AccountType::SAVINGS) { curN++; curSum += acc->getBalance(); }
                    else if (acc->getAccountType() == AccountType::FIXED) { fixN++; fixSum += acc->getBalance(); }
                    else if (const CreditAccount *c = dynamic_cast<const CreditAccount *>(acc.get())) { crN++; crDebt += c->getDebt(); }
                }
                std::cout << "--- 在用统计: 活期 " << curN << "户(" << formatMoney(curSum) << "元), "
                          << "定期 " << fixN << "户(" << formatMoney(fixSum) << "元), "
                          << "信用 " << crN << "户(总欠款" << formatMoney(crDebt) << "元), "
                          << "已销户 " << closedN << "户 ---\n";
                break;
            }
            case 2: {
                for (const auto &[uid, u] : bank.getUsers()) {
                    std::cout << "ID:" << uid << " | " << u->getName() << " | " << u->role();
                    if (const Customer *c = dynamic_cast<const Customer *>(u.get())) {
                        std::cout << " | 电话:" << c->getPhoneNumber() << " | 卡数:" << c->getAccountIds().size();
                    }
                    std::cout << "\n";
                }
                break;
            }
            case 3: {
                int mode;
                long long aid;
                std::cout << "1=冻结  2=解冻: ";
                std::cin >> mode;
                if (mode != 1 && mode != 2) {
                    std::cout << "无效选项，请输入 1 或 2。\n";
                    break;
                }
                std::cout << "账号: ";
                std::cin >> aid;
                if (mode == 1 && bank.freezeAccount(aid)) {
                    Logger::log("管理员冻结账户 " + std::to_string(aid));
                    std::cout << "已冻结。\n";
                } else if (mode == 2 && bank.unfreezeAccount(aid)) {
                    Logger::log("管理员解冻账户 " + std::to_string(aid));
                    std::cout << "已解冻。\n";
                }
                break;
            }
            case 4: {
                int rolled = 0;
                double sumInt = 0.0;
                Date today = Date::today();
                for (const auto &[aid, acc] : bank.getAccounts()) {
                    if (!acc->isNormal()) continue;
                    FixedAccount *fix = dynamic_cast<FixedAccount *>(acc.get());
                    bool willRoll = (fix != nullptr && fix->isAutoRenew() && fix->getBalance() > 0.0 &&
                                     today >= fix->getMaturityDate());
                    double interest = bank.settleInterest(aid, today);
                    if (interest > 0.0) {
                        sumInt = round2(sumInt + interest);
                        if (willRoll) rolled++;
                    }
                }
                std::cout << "全行结息完成：自动转存 " << rolled << " 笔，累计利息 " << formatMoney(sumInt) << " 元\n";
                break;
            }
            case 5: {
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
                switch (tier) {
                    case 1: ok = bank.setCurrentRate(newRate); break;
                    case 2: ok = bank.setFixedRate(3, newRate); break;
                    case 3: ok = bank.setFixedRate(12, newRate); break;
                    case 4: ok = bank.setFixedRate(36, newRate); break;
                    case 5: ok = bank.setCreditDailyRate(newRate); break;
                    default: std::cout << "档位编号不合法。\n"; break;
                }
                if (ok) {
                    Logger::log("管理员调整利率档位 " + std::to_string(tier));
                    std::cout << "利率已更新并即时生效。\n";
                }
                break;
            }
            case 6: {
                long long aid;
                std::cout << "筛选账号 (0=全行): ";
                std::cin >> aid;
                showAdminTransactions(bank, aid);
                break;
            }
            case 7: {
                for (const std::string &line : Logger::getAllLogs()) {
                    std::cout << line << "\n";
                }
                break;
            }
            default:
                std::cout << "无效选项，请重新输入。\n";
                break;
        }
    }
}


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Bank bank;
    TextFileStorage storage;
    storage.load(bank);
    bank.processMaturityRollovers(Date::today());

    bool running = true;
    while (running) {
        std::cout << "\n========= 银行账户管理系统 =========\n";
        std::cout << "1. 登录系统\n2. 注册新客户\n0. 保存并退出\n";
        std::cout << "请选择: ";
        int op = -1;
        if (!(std::cin >> op)) {
            if (std::cin.eof()) break;
            std::cin.clear();
            std::cin.ignore(1024, '\n');
            std::cout << "输入格式有误，请输入数字。\n";
            continue;
        }

        switch (op) {
            case 0: {
                storage.save(bank);
                Logger::log("系统退出并保存");
                std::cout << "数据已保存，再见！\n";
                running = false;
                break;
            }
            case 1: {
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
                break;
            }
            case 2: {
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
                break;
            }
            default:
                std::cout << "无效选项，请重新输入。\n";
                break;
        }
    }
    if (running) {
        storage.save(bank);
    }
    return 0;
}
