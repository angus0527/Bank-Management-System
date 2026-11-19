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

// 将文本中的状态编码还原为 AccountStatus 枚举（缺省回落为 NORMAL，保证向后兼容）
static AccountStatus parseStatusField(const std::string &s) {
    if (s == "2") return AccountStatus::FROZEN;
    if (s == "3") return AccountStatus::CLOSED;
    return AccountStatus::NORMAL;
}

// 根据 usersPath 所在目录推导同级 rates.txt 路径，避免污染正式测试目录
static std::string deriveRatesPath(const std::string &usersPath) {
    std::filesystem::path p(usersPath);
    if (p.has_parent_path()) {
        return (p.parent_path() / "rates.txt").string();
    }
    return "rates.txt";
}

TextFileStorage::TextFileStorage(std::string usersFile, std::string accountsFile, std::string txFile)
    : usersPath(resolvePath(usersFile)),
      accountsPath(resolvePath(accountsFile)),
      transactionsPath(resolvePath(txFile)) {}

// 序列化保存全行用户：格式为 U|用户ID|角色(C/A)|姓名|混淆密码|电话
void TextFileStorage::saveUsers(const Bank &bank) const {
    std::ofstream out(usersPath);
    if (!out.is_open()) return;

    for (const auto &[id, user] : bank.getUsers()) {
        std::string role = (user->role() == "Admin") ? "A" : "C";
        std::string phone = "0000";
        if (role == "C") {
            const auto *cust = dynamic_cast<const Customer *>(user.get());
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

    for (const auto &[id, account] : bank.getAccounts()) {
        out << account->serialize() << "\n";
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

// 解析 users.txt 并按角色标识还原 Admin / Customer 对象
// 注意构造函数末尾传 true 表示密文已混淆，避免重启读档发生二次混淆
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
            bank.addUser(std::make_unique<Admin>(id, f[3], f[4], true));
        } else {
            bank.addUser(std::make_unique<Customer>(id, f[3], f[4], f[5], true));
        }
    }
}

// 解析 accounts.txt，根据类型标识（C活期/F定期/X信用）动态实例化子类并补全与户主的双向关联
void TextFileStorage::loadAccounts(Bank &bank) {
    std::ifstream in(accountsPath);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = split(line, '|');
        if (f.size() < 7 || f[0] != "A") continue;

        long long id = std::stoll(f[1]);
        long long ownerId = std::stoll(f[3]);
        long long balance = std::stoll(f[4]);
        Date openDate = Date::fromString(f[5]);
        long long rate = std::stoll(f[6]);

        std::unique_ptr<Account> acc;
        if (f[2] == "C") {
            // 还原活期账户及其上次结息日、状态
            auto cur = std::make_unique<CurrentAccount>(id, ownerId, balance, openDate, rate);
            if (f.size() >= 13 && !f[12].empty()) {
                cur->setLastSettleDate(Date::fromString(f[12]));
            }
            if (f.size() >= 14 && !f[13].empty()) {
                cur->setStatus(parseStatusField(f[13]));
            }
            acc = std::move(cur);
        } else if (f[2] == "F" && f.size() >= 10) {
            // 还原定期账户：存期月数、到期日、自动转存标志与提前支取参照活期利率
            int termMonths = std::stoi(f[7]);
            Date maturityDate = Date::fromString(f[8]);
            bool autoRenew = (f[9] == "1");
            long long curRate = (f.size() >= 13 && !f[12].empty()) ? std::stoll(f[12]) : bank.getCurrentRateX10000();
            auto fix = std::make_unique<FixedAccount>(id, ownerId, balance, openDate, termMonths,
                                                      rate, autoRenew, curRate);
            fix->setMaturityDate(maturityDate);
            if (f.size() >= 14 && !f[13].empty()) {
                fix->setStatus(parseStatusField(f[13]));
            }
            acc = std::move(fix);
        } else if (f[2] == "X" && f.size() >= 11) {
            // 还原信用账户：支持恢复负余额（透支欠款）及信用额度
            long long creditLimit = std::stoll(f[10]);
            auto cr = std::make_unique<CreditAccount>(id, ownerId, balance, openDate, creditLimit, rate);
            if (f.size() >= 13 && !f[12].empty()) {
                cr->setLastSettleDate(Date::fromString(f[12]));
            }
            if (f.size() >= 14 && !f[13].empty()) {
                cr->setStatus(parseStatusField(f[13]));
            }
            acc = std::move(cr);
        }

        if (acc) {
            AccountStatus st = acc->getStatus();
            bank.addAccount(std::move(acc));
            // 仅将未销户的账号回填进所属 Customer 的持卡列表，保证读档后名下卡号完整
            if (st != AccountStatus::CLOSED) {
                try {
                    User *u = bank.findUser(ownerId);
                    if (auto *cust = dynamic_cast<Customer *>(u)) {
                        cust->addAccount(id);
                    }
                } catch (...) {
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
        long long amount = std::stoll(f[6]);
        long long snapshot = std::stoll(f[7]);
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

    std::ofstream rOut(deriveRatesPath(usersPath));
    if (rOut.is_open()) {
        rOut << "R|" << bank.getCurrentRateX10000() << "|"
             << bank.getFixedRateX10000(3) << "|"
             << bank.getFixedRateX10000(12) << "|"
             << bank.getFixedRateX10000(36) << "|"
             << bank.getCreditDailyRateX10000() << "\n";
    }
}

// 全量装载：先恢复基准利率配置，再依次恢复用户、账户与流水（顺序不可颠倒）
void TextFileStorage::load(Bank &bank) {
    std::ifstream rIn(deriveRatesPath(usersPath));
    if (rIn.is_open()) {
        std::string line;
        if (std::getline(rIn, line)) {
            auto f = split(line, '|');
            if (f.size() >= 6 && f[0] == "R") {
                bank.initRates(std::stoll(f[1]), std::stoll(f[2]), std::stoll(f[3]),
                               std::stoll(f[4]), std::stoll(f[5]));
            }
        }
    }
    loadUsers(bank);
    loadAccounts(bank);
    loadTransactions(bank);
}
