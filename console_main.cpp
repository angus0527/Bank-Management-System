//
// Created by angus on 2026/10/29.
// console_main：临时控制台入口（README §8，保底演示，答辩前不删）
// 第 2 周里程碑：登录→开户→存→取→转账→结息→保存→重开读档数据完好
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Bank.h"
#include "CurrentAccount.h"
#include "Logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

// ==================== 临时演示持久化（第 2 周里程碑用） ====================
// 说明：真正的持久化是 B 负责的 TextFileStorage（继承 Storage 抽象接口），
// 本文件里的读写仅用于第 2 周"保存→重开读档数据完好"的里程碑演示，
// 格式严格遵循 README §5，B 的 TextFileStorage 完成后本段将被替换删除。

static std::vector<std::string> splitLine(const std::string &line, char sep) {
    std::vector<std::string> fields;
    std::istringstream iss(line);
    std::string item;
    while (std::getline(iss, item, sep)) {
        fields.push_back(item);
    }
    return fields;
}

static void saveDemoData(const Bank &bank) {
    std::filesystem::create_directories("data");
    std::ofstream usersFile("data/users.txt");
    for (const auto &[id, user]: bank.getUsers()) {
        std::string role = user->role() == "Admin" ? "A" : "C";
        std::string phone = role == "C"
                                ? dynamic_cast<const Customer *>(user.get())->getPhoneNumber()
                                : "0000";
        // U|用户id|角色(C或A)|姓名|混淆后密码|电话（README §5）
        usersFile << "U|" << id << "|" << role << "|" << user->getName() << "|"
                  << user->getPassword() << "|" << phone << "\n";
    }
    std::ofstream accountsFile("data/accounts.txt");
    for (const auto &[id, account]: bank.getAccounts()) {
        accountsFile << account->serialize() << "\n";
    }
    std::ofstream txFile("data/transactions.txt");
    for (const Transaction &t: bank.getTransactions()) {
        txFile << t.serialize() << "\n";
    }
}

static void loadDemoData(Bank &bank) {
    std::ifstream usersFile("data/users.txt");
    std::string line;
    while (std::getline(usersFile, line)) {
        auto f = splitLine(line, '|');
        if (f.size() < 6) continue;
        long long id = std::stoll(f[1]);
        if (f[2] == "A") {
            bank.addUser(std::make_unique<Admin>(id, f[3], f[4], true));
        } else {
            bank.addUser(std::make_unique<Customer>(id, f[3], f[4], f[5], true));
        }
    }
    std::ifstream accountsFile("data/accounts.txt");
    while (std::getline(accountsFile, line)) {
        auto f = splitLine(line, '|');
        if (f.size() < 7 || f[0] != "A") continue;
        long long id = std::stoll(f[1]);
        long long ownerId = std::stoll(f[3]);
        long long balance = std::stoll(f[4]);
        Date openDate = Date::fromString(f[5]);
        long long rate = std::stoll(f[6]);
        if (f[2] == "C") {
            auto acc = std::make_unique<CurrentAccount>(id, ownerId, balance, openDate, rate);
            // 第 13 个尾字段为"上次结息日"，老格式无此字段则默认取开户日
            if (f.size() >= 13) {
                acc->setLastSettleDate(Date::fromString(f[12]));
            }
            bank.addAccount(std::move(acc));
        }
        // F(定期)/X(信用) 类型第 4 周由 FixedAccount/CreditAccount 接入
    }
    std::ifstream txFile("data/transactions.txt");
    while (std::getline(txFile, line)) {
        auto f = splitLine(line, '|');
        if (f.size() < 9 || f[0] != "T") continue;
        bank.addTransaction(Transaction(std::stoll(f[1]), f[2], Transaction::typeFromCode(f[3]),
                                        std::stoll(f[4]), std::stoll(f[5]), std::stoll(f[6]),
                                        std::stoll(f[7]), f[8]));
    }
}

// ==================== 演示流程 ====================

static void seedUsers(Bank &bank) {
    bank.addUser(std::make_unique<Admin>(10001, "admin", "admin123"));
    bank.addUser(std::make_unique<Customer>(20001, "张三", "zs123456", "13800000001"));
    bank.addUser(std::make_unique<Customer>(20002, "李四", "ls123456", "13800000002"));
}

static void printTitle(const std::string &title) {
    std::cout << "\n========== " << title << " ==========\n";
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // 控制台切 UTF-8，中文不乱码（周记①问题四）
#endif
    Logger::clearAllLogs();
    Logger::log("系统启动（console_main 第 2 周演示）");

    Bank bank;
    seedUsers(bank);

    // ---- 1. 登录（F01）----
    printTitle("1. 登录");
    try {
        bank.login(20001, "错误密码");
    } catch (const AuthFail &e) {
        std::cout << "捕获 AuthFail: " << e.what() << "\n";
    }
    User *zhangsan = bank.login(20001, "zs123456");
    std::cout << "登录成功: " << zhangsan->getName() << "（" << zhangsan->role() << "）\n";
    Logger::log("张三(20001) 登录成功");

    // ---- 2. 开户（F02）：张三存 1000 元，开户日回拨 30 天用于计息演示 ----
    printTitle("2. 开户");
    long long acc1 = bank.openAccount(20001, 100000, Date::today().addDays(-30));
    long long acc2 = bank.openAccount(20002, 0);
    std::cout << "张三开户成功: " << acc1 << "，李四开户成功: " << acc2 << "\n";
    Logger::log("张三(20001) 开户 62001，初始金额 1000.00 元");

    // ---- 3. 存款 / 取款 / 转账（F05~F07）----
    printTitle("3. 存 / 取 / 转账");
    bank.deposit(acc1, 50000, "工资入账");
    std::cout << "张三存款 500.00 元\n";
    bank.withdraw(acc1, 20000, "ATM 取款");
    std::cout << "张三取款 200.00 元\n";
    bank.transfer(acc1, acc2, 30000, "还李四饭钱");
    std::cout << "张三转账 300.00 元给李四\n";
    bank.findAccount(acc1)->displayInfo();

    // ---- 4. 异常体系演示（README §9 T02~T05 用例，分别按子类捕获）----
    printTitle("4. 异常体系");
    try {
        bank.withdraw(acc1, 999900);
    } catch (const InsufficientBalance &e) {
        std::cout << "捕获 InsufficientBalance: " << e.what() << "\n";
    }
    try {
        bank.deposit(acc1, -500);
    } catch (const InvalidAmount &e) {
        std::cout << "捕获 InvalidAmount: " << e.what() << "\n";
    }
    try {
        bank.findAccount(99999);
    } catch (const AccountNotFound &e) {
        std::cout << "捕获 AccountNotFound: " << e.what() << "\n";
    }
    bank.freezeAccount(acc2);
    try {
        bank.transfer(acc1, acc2, 100);
    } catch (const AccountFrozen &e) {
        std::cout << "捕获 AccountFrozen: " << e.what() << "\n";
    }
    bank.unfreezeAccount(acc2);
    std::cout << "解冻后账户 " << acc2 << " 状态: " << bank.findAccount(acc2)->getStatusString() << "\n";

    // ---- 5. 活期结息（F09：年利率/360 按日计息，手动触发）----
    printTitle("5. 结息");
    long long interest1 = bank.settleInterest(acc1); // 距开户 30 天
    long long interest2 = bank.settleInterest(acc2);
    std::cout << "张三账户 " << acc1 << " 结息入账 " << formatMoney(interest1) << " 元\n";
    std::cout << "李四账户 " << acc2 << " 结息入账 " << formatMoney(interest2) << " 元\n";
    std::cout << "同日再次结息: " << formatMoney(bank.settleInterest(acc1))
              << " 元（按日计息，当天不重复）\n";
    bank.findAccount(acc1)->displayInfo();

    // ---- 6. 流水查询（F08）----
    printTitle("6. 张三的流水");
    for (const Transaction *t: bank.queryTransactions(acc1)) {
        t->display();
    }

    // ---- 7. 保存 → 重开读档验证（F10 里程碑）----
    printTitle("7. 保存并重开程序验证");
    saveDemoData(bank);
    Logger::log("系统存盘退出");

    Bank reloaded; // 模拟"重开程序"
    loadDemoData(reloaded);
    bool ok = reloaded.findUser(20001)->getName() == "张三" &&
              reloaded.findAccount(acc1)->getBalance() == bank.findAccount(acc1)->getBalance() &&
              reloaded.findAccount(acc2)->getBalance() == bank.findAccount(acc2)->getBalance() &&
              reloaded.getTransactions().size() == bank.getTransactions().size();
    std::cout << "读档后账户信息:\n";
    reloaded.findAccount(acc1)->displayInfo();
    reloaded.findAccount(acc2)->displayInfo();
    std::cout << (ok ? ">>> 读档验证通过：账户、余额、流水数据完好 <<<"
                     : ">>> 读档验证失败！<<<") << "\n";
    try {
        reloaded.login(20001, "zs123456");
        std::cout << "读档后密码校验通过\n";
    } catch (const AuthFail &e) {
        std::cout << "读档后密码校验失败: " << e.what() << "\n";
    }

    // ---- 8. 操作日志（B 的 Logger 模块）----
    printTitle("8. 操作日志");
    for (const auto &logLine: Logger::getAllLogs()) {
        std::cout << logLine << "\n";
    }
    return 0;
}
