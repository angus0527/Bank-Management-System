#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "Bank.h"
#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"

// 按指定分隔符切分单行文本记录（用于解析 '|' 分隔的文本字段）
static std::vector<std::string> split(const std::string &line, char sep) {
    std::vector<std::string> res;
    std::istringstream iss(line);
    std::string item;
    while (std::getline(iss, item, sep)) {
        res.push_back(item);
    }
    return res;
}

// 跨工作目录路径探测：先检查当前工作目录下是否存在，若不存在（如在 cmake-build-debug 运行）则向上级目录回溯查找
static std::string resolvePath(const std::string &path) {
    if (std::filesystem::exists(path)) {
        return path;
    }
    std::filesystem::path p("..");
    p /= path;
    if (std::filesystem::exists(p)) {
        return p.string();
    }
    return path;
}

// 将文本中的状态编码还原为 AccountStatus 枚举
static AccountStatus parseStatusField(const std::string &s) {
    if (s == "2") return AccountStatus::FROZEN;
    if (s == "3") return AccountStatus::CLOSED;
    return AccountStatus::NORMAL;
}

TextFileStorage::TextFileStorage(std::string usersFile, std::string accountsFile,
                                 std::string txFile, std::string ratesFile)
    : usersPath(resolvePath(usersFile)),
      accountsPath(resolvePath(accountsFile)),
      transactionsPath(resolvePath(txFile)),
      ratesPath(resolvePath(ratesFile)) {}

// 序列化保存全行用户：格式为 U|用户ID|角色(C/A)|姓名|混淆密码|电话
void TextFileStorage::saveUsers(const Bank &bank) const {
    std::ofstream out(usersPath);
    if (!out.is_open()) return;

    for (const auto &pair : bank.getUsers()) {
        long long id = pair.first;
        const User *user = pair.second.get();
        std::string role = (user->role() == "Admin") ? "A" : "C";
        std::string phone = "0000";
        if (role == "C") {
            const Customer *cust = dynamic_cast<const Customer *>(user);
            if (cust) {
                phone = cust->getPhoneNumber();
            }
        }
        out << "U|" << id << "|" << role << "|" << user->getName() << "|"
            << user->getPassword() << "|" << phone << "\n";
    }
}

// 多态调用每个账户对象的 serialize() 方法写入 accounts.txt
void TextFileStorage::saveAccounts(const Bank &bank) const {
    std::ofstream out(accountsPath);
    if (!out.is_open()) return;

    for (const auto &pair : bank.getAccounts()) {
        out << pair.second->serialize() << "\n";
    }
}

// 顺序保存全行交易流水到 transactions.txt
void TextFileStorage::saveTransactions(const Bank &bank) const {
    std::ofstream out(transactionsPath);
    if (!out.is_open()) return;

    for (const auto &tx : bank.getTransactions()) {
        out << tx.serialize() << "\n";
    }
}

// 解析 users.txt 并按角色标识还原 Admin / Customer 对象（文件中存的本身就是加密后密文）
void TextFileStorage::loadUsers(Bank &bank) {
    std::ifstream in(usersPath);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = split(line, '|');
        if (f.size() < 6 || f[0] != "U") continue;

        long long id = std::stoll(f[1]);
        if (f[2] == "A") {
            bank.addUser(std::make_unique<Admin>(id, f[3], f[4]));
        } else {
            bank.addUser(std::make_unique<Customer>(id, f[3], f[4], f[5]));
        }
    }
}

// 解析 accounts.txt（固定 14 列格式），按类型标识还原 C 活期 / F 定期 / X 信用账户
void TextFileStorage::loadAccounts(Bank &bank) {
    std::ifstream in(accountsPath);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = split(line, '|');
        if (f.size() < 14 || f[0] != "A") continue;

        long long id = std::stoll(f[1]);
        long long ownerId = std::stoll(f[3]);
        double balance = std::stod(f[4]);
        Date openDate = Date::fromString(f[5]);
        double rate = std::stod(f[6]);

        std::unique_ptr<Account> acc;
        if (f[2] == "C") {
            auto cur = std::make_unique<CurrentAccount>(id, ownerId, balance, openDate, rate);
            cur->setLastSettleDate(Date::fromString(f[12]));
            acc = std::move(cur);
        } else if (f[2] == "F") {
            int termMonths = std::stoi(f[7]);
            Date maturityDate = Date::fromString(f[8]);
            bool autoRenew = (f[9] == "1");
            double curRate = std::stod(f[12]);
            auto fix = std::make_unique<FixedAccount>(id, ownerId, balance, openDate, termMonths,
                                                      rate, autoRenew, curRate);
            fix->setMaturityDate(maturityDate);
            acc = std::move(fix);
        } else if (f[2] == "X") {
            double creditLimit = std::stod(f[10]);
            auto cr = std::make_unique<CreditAccount>(id, ownerId, balance, openDate, creditLimit, rate);
            cr->setLastSettleDate(Date::fromString(f[12]));
            acc = std::move(cr);
        }

        if (acc) {
            AccountStatus st = parseStatusField(f[13]);
            acc->setStatus(st);
            bank.addAccount(std::move(acc));
            // 仅将未销户的账号回填进所属 Customer 的持卡列表，保证读档后名下卡号完整
            if (st != AccountStatus::CLOSED) {
                const auto &users = bank.getUsers();
                auto it = users.find(ownerId);
                if (it != users.end()) {
                    if (Customer *cust = dynamic_cast<Customer *>(it->second.get())) {
                        cust->addAccount(id);
                    }
                }
            }
        }
    }
}

// 解析 transactions.txt，恢复全部历史流水记录并自动顺延流水发号器
void TextFileStorage::loadTransactions(Bank &bank) {
    std::ifstream in(transactionsPath);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = split(line, '|');
        if (f.size() < 9 || f[0] != "T") continue;

        long long id = std::stoll(f[1]);
        TransactionType type = Transaction::typeFromCode(f[3]);
        long long fromAcc = std::stoll(f[4]);
        long long toAcc = std::stoll(f[5]);
        double amount = std::stod(f[6]);
        double snapshot = std::stod(f[7]);
        std::string remark = f[8];

        bank.addTransaction(Transaction(id, f[2], type, fromAcc, toAcc, amount, snapshot, remark));
    }
}

// 全量持久化：自动创建缺失的父目录，依次保存用户、账户、流水及全行基准利率
void TextFileStorage::save(const Bank &bank) {
    std::filesystem::path p(usersPath);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    saveUsers(bank);
    saveAccounts(bank);
    saveTransactions(bank);

    std::ofstream rOut(ratesPath);
    if (rOut.is_open()) {
        rOut << "R|" << bank.getCurrentRate() << "|"
             << bank.getFixedRate(3) << "|"
             << bank.getFixedRate(12) << "|"
             << bank.getFixedRate(36) << "|"
             << bank.getCreditDailyRate() << "\n";
    }
}

// 全量装载：先恢复基准利率配置，再依次恢复用户、账户与流水（顺序不可颠倒）
void TextFileStorage::load(Bank &bank) {
    std::ifstream rIn(ratesPath);
    if (rIn.is_open()) {
        std::string line;
        if (std::getline(rIn, line)) {
            auto f = split(line, '|');
            if (f.size() >= 6 && f[0] == "R") {
                bank.initRates(std::stod(f[1]), std::stod(f[2]), std::stod(f[3]),
                               std::stod(f[4]), std::stod(f[5]));
            }
        }
    }
    loadUsers(bank);
    loadAccounts(bank);
    loadTransactions(bank);
}
