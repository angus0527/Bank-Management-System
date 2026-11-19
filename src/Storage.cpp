#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "Bank.h"
#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"

static std::vector<std::string> split(const std::string &line, char sep) {
    std::vector<std::string> res;
    std::istringstream iss(line);
    std::string item;
    while (std::getline(iss, item, sep)) {
        res.push_back(item);
    }
    return res;
}

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

static AccountStatus parseStatusField(const std::string &s) {
    if (s == "2") return AccountStatus::FROZEN;
    if (s == "3") return AccountStatus::CLOSED;
    return AccountStatus::NORMAL;
}

static std::string deriveRatesPath(const std::string &usersPath) {
    std::filesystem::path p(usersPath);
    if (p.has_parent_path()) {
        return (p.parent_path() / "rates.txt").string();
    }
    return "rates.txt";
}

TextFileStorage::TextFileStorage(std::string usersFile, std::string accountsFile, std::string txFile)
    : usersPath(resolvePath(usersFile)), accountsPath(resolvePath(accountsFile)), transactionsPath(resolvePath(txFile)) {}

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

void TextFileStorage::saveAccounts(const Bank &bank) const {
    std::ofstream out(accountsPath);
    if (!out.is_open()) return;

    for (const auto &[id, account] : bank.getAccounts()) {
        out << account->serialize() << "\n";
    }
}

void TextFileStorage::saveTransactions(const Bank &bank) const {
    std::ofstream out(transactionsPath);
    if (!out.is_open()) return;

    for (const auto &tx : bank.getTransactions()) {
        out << tx.serialize() << "\n";
    }
}

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
            auto cur = std::make_unique<CurrentAccount>(id, ownerId, balance, openDate, rate);
            if (f.size() >= 13 && !f[12].empty()) {
                cur->setLastSettleDate(Date::fromString(f[12]));
            }
            if (f.size() >= 14 && !f[13].empty()) {
                cur->setStatus(parseStatusField(f[13]));
            }
            acc = std::move(cur);
        } else if (f[2] == "F" && f.size() >= 10) {
            int termMonths = std::stoi(f[7]);
            Date maturityDate = Date::fromString(f[8]);
            bool autoRenew = (f[9] == "1");
            long long curRate = (f.size() >= 13 && !f[12].empty()) ? std::stoll(f[12]) : bank.getCurrentRateX10000();
            auto fix = std::make_unique<FixedAccount>(id, ownerId, balance, openDate, termMonths,
                                                      maturityDate, rate, autoRenew, curRate);
            if (f.size() >= 14 && !f[13].empty()) {
                fix->setStatus(parseStatusField(f[13]));
            }
            acc = std::move(fix);
        } else if (f[2] == "X" && f.size() >= 11) {
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
