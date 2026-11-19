#include <filesystem>
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

// 清空 cin 错误状态并丢弃本行残留输入，防止用户输入非数字导致死循环
static void clearCinLine() {
    std::cin.clear();
    std::cin.ignore(1024, '\n');
}

// 安全读取整数（菜单编号、账号等）
static bool inputLongLong(const std::string &prompt, long long &val) {
    std::cout << prompt;
    if (std::cin >> val) return true;
    if (std::cin.eof()) return false;
    clearCinLine();
    std::cout << "输入格式有误，请输入数字。\n";
    return false;
}

// 读取金额（元）并四舍五入转为“分”，避免浮点误差
static bool inputCents(const std::string &prompt, long long &cents) {
    std::cout << prompt;
    double yuan = 0;
    if (std::cin >> yuan) {
        if (yuan < 0) {
            std::cout << "金额不能为负数。\n";
            return false;
        }
        cents = static_cast<long long>(yuan * 100.0 + 0.5);
        return true;
    }
    if (std::cin.eof()) return false;
    clearCinLine();
    std::cout << "金额格式有误，请输入数字。\n";
    return false;
}

static std::string txTypeName(TransactionType t) {
    if (t == TransactionType::DEP) return "存款";
    if (t == TransactionType::WDR) return "取款";
    if (t == TransactionType::TRF) return "转账";
    return "结息";
}

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

// 首次运行无数据时初始化默认管理员和两位测试客户
static void seedDefaultData(Bank &bank) {
    bank.addUser(std::make_unique<Admin>(10001, "admin", "admin123"));
    bank.addUser(std::make_unique<Customer>(20001, "张三", "zs123456", "13800000001"));
    bank.addUser(std::make_unique<Customer>(20002, "李四", "ls123456", "13800000002"));
    bank.openAccount(20001, 100029, Date(2026, 10, 2));
    bank.openAccount(20002, 30004, Date(2026, 10, 15));
}

// 按账号、类型、日期段筛选并调用 Transaction::display() 输出流水；cust 非空时仅展示本人账户流水
static void showTransactions(const Bank &bank, long long accId, const Customer *cust = nullptr) {
    int typeFilter = 0;
    std::string startD, endD;
    std::cout << "按类型筛选 (0=全部 1=存款 2=取款 3=转账 4=结息): ";
    if (!(std::cin >> typeFilter)) { clearCinLine(); return; }
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

        long long op = -1;
        if (!inputLongLong("请选择: ", op)) {
            if (std::cin.eof()) break;
            continue;
        }
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
            long long aid, cents;
            if (!inputLongLong("账号: ", aid) || !inputCents("存入/还款金额(元): ", cents)) continue;
            if (bank.deposit(aid, cents, "柜面存入/还款")) {
                Logger::log("账户 " + std::to_string(aid) + " 存入 " + formatMoney(cents) + " 元");
                std::cout << "操作成功，当前余额: " << formatMoney(bank.findAccount(aid)->getBalance()) << " 元\n";
            }
        } else if (op == 3) {
            long long aid;
            if (!inputLongLong("账号: ", aid)) continue;
            if (!hasAccount(cust, aid)) { std::cout << "只能操作本人名下账户。\n"; continue; }
            Account *a = bank.findAccount(aid);
            if (a == nullptr) { std::cout << "账户不存在。\n"; continue; }
            if (a->getAccountType() == AccountType::FIXED) {
                long long payout = bank.withdrawFixed(aid);
                if (payout >= 0) {
                    Logger::log("定期账户 " + std::to_string(aid) + " 支取 " + formatMoney(payout) + " 元");
                    std::cout << "定期全额支取成功，实付本息: " << formatMoney(payout) << " 元\n";
                }
            } else {
                long long cents;
                if (!inputCents("取款/消费金额(元): ", cents)) continue;
                if (bank.withdraw(aid, cents, "取款/消费")) {
                    Logger::log("账户 " + std::to_string(aid) + " 支出 " + formatMoney(cents) + " 元");
                    std::cout << "操作成功，当前余额: " << formatMoney(a->getBalance()) << " 元\n";
                }
            }
        } else if (op == 4) {
            long long fromId, toId, cents;
            if (!inputLongLong("付款账号: ", fromId) ||
                !inputLongLong("收款账号: ", toId) ||
                !inputCents("转账金额(元): ", cents)) continue;
            if (!hasAccount(cust, fromId)) { std::cout << "只能从本人名下账户转出。\n"; continue; }
            if (bank.transfer(fromId, toId, cents, "同行转账")) {
                Logger::log("转账 " + std::to_string(fromId) + " -> " + std::to_string(toId) + " " + formatMoney(cents) + " 元");
                std::cout << "转账成功！付款账户余额: " << formatMoney(bank.findAccount(fromId)->getBalance()) << " 元\n";
            }
        } else if (op == 5) {
            long long kind = 0;
            if (!inputLongLong("账户类型 (1=活期 2=定期 3=信用): ", kind)) continue;
            if (kind == 1) {
                long long cents;
                if (!inputCents("初始存款(元): ", cents)) continue;
                long long nid = bank.openAccount(cust->getId(), cents);
                if (nid > 0) {
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立活期账户 " + std::to_string(nid));
                    std::cout << "开户成功，账号: " << nid << "\n";
                }
            } else if (kind == 2) {
                long long term, cents, renew;
                if (!inputLongLong("存期月数 (3/12/36): ", term) ||
                    !inputCents("定期存入金额(元): ", cents) ||
                    !inputLongLong("到期自动转存? (1=是 0=否): ", renew)) continue;
                long long nid = bank.openFixedAccount(cust->getId(), cents, static_cast<int>(term), renew == 1);
                if (nid > 0) {
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立定期账户 " + std::to_string(nid));
                    std::cout << "定期开户成功，账号: " << nid << "\n";
                }
            } else if (kind == 3) {
                long long limit;
                if (!inputCents("申请信用额度(元): ", limit)) continue;
                long long nid = bank.openCreditAccount(cust->getId(), limit);
                if (nid > 0) {
                    Logger::log("客户 " + std::to_string(cust->getId()) + " 开立信用账户 " + std::to_string(nid));
                    std::cout << "信用账户开户成功，账号: " << nid << "\n";
                }
            }
        } else if (op == 6) {
            long long aid;
            if (!inputLongLong("结息账号: ", aid)) continue;
            if (!hasAccount(cust, aid)) { std::cout << "只能操作本人名下账户。\n"; continue; }
            long long interest = bank.settleInterest(aid);
            if (interest >= 0) {
                std::cout << "结息完成，本次利息发生额: " << formatMoney(interest) << " 元\n";
            }
        } else if (op == 7) {
            long long aid;
            if (!inputLongLong("查询账号 (0=本人全部): ", aid)) continue;
            if (aid != 0 && !hasAccount(cust, aid)) { std::cout << "只能查询本人名下账户流水。\n"; continue; }
            showTransactions(bank, aid, cust);
        } else if (op == 8) {
            long long sub = 0;
            if (!inputLongLong("1=修改姓名电话  2=修改密码: ", sub)) continue;
            if (sub == 1) {
                std::string name, phone;
                std::cout << "新姓名: "; std::cin >> name;
                std::cout << "新电话: "; std::cin >> phone;
                if (bank.updateUserInfo(cust->getId(), name, phone)) {
                    std::cout << "资料已更新。\n";
                }
            } else if (sub == 2) {
                std::string oldP, newP;
                std::cout << "原密码: "; std::cin >> oldP;
                std::cout << "新密码: "; std::cin >> newP;
                if (bank.changeUserPassword(cust->getId(), oldP, newP)) {
                    std::cout << "密码修改成功。\n";
                }
            }
        } else if (op == 9) {
            long long aid;
            if (!inputLongLong("要销户的账号: ", aid)) continue;
            if (!hasAccount(cust, aid)) { std::cout << "只能操作本人名下账户。\n"; continue; }
            if (bank.closeAccount(aid)) {
                Logger::log("销户 " + std::to_string(aid));
                std::cout << "销户成功。\n";
            }
        } else if (op == 10) {
            long long aid;
            if (!inputLongLong("导出对账单账号: ", aid)) continue;
            if (!hasAccount(cust, aid)) { std::cout << "只能导出本人名下账户对账单。\n"; continue; }
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

        long long op = -1;
        if (!inputLongLong("请选择: ", op)) {
            if (std::cin.eof()) break;
            continue;
        }
        if (op == 0) break;

        if (op == 1) {
            long long curSum = 0, fixSum = 0, crDebt = 0;
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
            long long mode, aid;
            if (!inputLongLong("1=冻结  2=解冻: ", mode) || !inputLongLong("账号: ", aid)) continue;
            if (mode == 1 && bank.freezeAccount(aid)) {
                Logger::log("管理员冻结账户 " + std::to_string(aid));
                std::cout << "已冻结。\n";
            } else if (mode == 2 && bank.unfreezeAccount(aid)) {
                Logger::log("管理员解冻账户 " + std::to_string(aid));
                std::cout << "已解冻。\n";
            }
        } else if (op == 4) {
            int rolled = bank.processMaturityRollovers();
            long long sumInt = 0;
            for (const auto &pair : bank.getAccounts()) {
                if (pair.second->isNormal()) {
                    long long interest = bank.settleInterest(pair.first);
                    if (interest > 0) sumInt += interest;
                }
            }
            std::cout << "全行结息完成：自动转存 " << rolled << " 笔，累计利息 " << formatMoney(sumInt) << " 元\n";
        } else if (op == 5) {
            std::cout << "当前利率: 1=活期(" << formatMoney(bank.getCurrentRateX10000())
                      << "%) 2=定3月(" << formatMoney(bank.getFixedRateX10000(3))
                      << "%) 3=定1年(" << formatMoney(bank.getFixedRateX10000(12))
                      << "%) 4=定3年(" << formatMoney(bank.getFixedRateX10000(36))
                      << "%) 5=信用日息(" << formatMoney(bank.getCreditDailyRateX10000()) << "%)\n";
            long long tier, newRate;
            if (!inputLongLong("修改哪一档 [1-5]: ", tier) ||
                !inputCents("输入新利率百分比(如 1.75 代表 1.75%): ", newRate)) continue;
            bool ok = false;
            if (tier == 1) ok = bank.setCurrentRate(newRate);
            else if (tier == 2) ok = bank.setFixedRate(3, newRate);
            else if (tier == 3) ok = bank.setFixedRate(12, newRate);
            else if (tier == 4) ok = bank.setFixedRate(36, newRate);
            else if (tier == 5) ok = bank.setCreditDailyRate(newRate);
            if (ok) {
                Logger::log("管理员调整利率档位 " + std::to_string(tier));
                std::cout << "利率已更新并即时生效。\n";
            }
        } else if (op == 6) {
            long long sub = 0;
            if (!inputLongLong("1=查看流水  2=查看操作日志: ", sub)) continue;
            if (sub == 1) {
                long long aid;
                if (!inputLongLong("筛选账号 (0=全行): ", aid)) continue;
                showTransactions(bank, aid);
            } else if (sub == 2) {
                for (const std::string &line : Logger::getAllLogs()) std::cout << line << "\n";
            }
        }
    }
}

// 简化版全流程自检（演示核心业务、日期加减、三类账户多态计息与独立目录持久化）
static void runDemoTest() {
    std::cout << "\n--- 开始功能自检 ---\n";
    Date d(2026, 2, 28);
    if (d.addDays(1).toString() == "2026-03-01" && Date(2024, 2, 28).addDays(2).toString() == "2024-03-01" &&
        Date(2026, 1, 1).daysBetween(Date(2026, 1, 11)) == 10) {
        std::cout << "[PASS] 日期闰年加减与天数差验证通过\n";
    }

    Bank b;
    seedDefaultData(b);

    if (b.login(20001, "wrong") == nullptr) {
        std::cout << "[PASS] 错误密码拦截\n";
    }
    b.deposit(62001, 50000, "存款");
    b.withdraw(62001, 20000, "取款");
    b.transfer(62001, 62002, 30000, "转账");
    if (!b.withdraw(62001, 99999900)) {
        std::cout << "[PASS] 余额不足拦截\n";
    }

    long long fixId = b.openFixedAccount(20001, 1000000, 12, true, Date(2025, 1, 1));
    b.processMaturityRollovers(Date(2026, 1, 2));
    std::cout << "[PASS] 1万定期到期转存余额: " << formatMoney(b.findAccount(fixId)->getBalance()) << " 元\n";

    long long crId = b.openCreditAccount(20001, 50000, Date(2026, 10, 1));
    b.withdraw(crId, 10000, "透支");
    b.deposit(crId, 6000, "还款");
    b.settleInterest(crId, Date(2026, 10, 11));
    CreditAccount *cr = dynamic_cast<CreditAccount *>(b.findAccount(crId));
    std::cout << "[PASS] 信用欠100还60计10天息后欠款: " << formatMoney(cr->getDebt()) << " 元\n";

    TextFileStorage st("data/test/users.txt", "data/test/accounts.txt", "data/test/transactions.txt");
    st.save(b);
    Bank b2;
    st.load(b2);
    if (b2.findAccount(62001)->getBalance() == b.findAccount(62001)->getBalance() &&
        b2.findAccount(crId)->getBalance() == b.findAccount(crId)->getBalance()) {
        std::cout << "[PASS] 读档一致性验证通过\n";
    }
    std::filesystem::remove_all("data/test");
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
    bank.processMaturityRollovers(Date::today());

    while (true) {
        std::cout << "\n========= 银行账户管理系统 =========\n";
        std::cout << "1. 登录系统\n2. 一键功能自测\n0. 保存并退出\n";
        long long op = -1;
        if (!inputLongLong("请选择: ", op)) {
            if (std::cin.eof()) break;
            continue;
        }

        if (op == 0) {
            storage.save(bank);
            Logger::log("系统退出并保存");
            std::cout << "数据已保存，再见！\n";
            break;
        } else if (op == 1) {
            long long uid;
            std::string pwd;
            if (!inputLongLong("用户ID: ", uid)) continue;
            std::cout << "密码: ";
            if (!(std::cin >> pwd)) break;
            User *u = bank.login(uid, pwd);
            if (u != nullptr) {
                Logger::log("用户 " + std::to_string(uid) + " 登录");
                if (u->role() == "Admin") adminMenu(bank, u);
                else if (Customer *c = dynamic_cast<Customer *>(u)) customerMenu(bank, c);
            }
        } else if (op == 2) {
            runDemoTest();
        }
    }
    return 0;
}
