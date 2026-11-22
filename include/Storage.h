#ifndef STORAGE_H
#define STORAGE_H

#include <string>

class Bank;

class Storage {
public:
    virtual ~Storage() = default;

    // 从txt读取信息
    virtual void load(Bank &bank) = 0;

    // 将 Bank 当前的信息全部存入data
    virtual void save(const Bank &bank) = 0;
};

class TextFileStorage : public Storage {
private:
    std::string usersPath;         // 用户表文件路径
    std::string accountsPath;      // 账户表文件路径
    std::string transactionsPath;  // 流水表文件路径
    std::string ratesPath;         // 利率配置文件路径

    void saveUsers(const Bank &bank) const;
    void saveAccounts(const Bank &bank) const;
    void saveTransactions(const Bank &bank) const;
    void saveRates(const Bank &bank) const;

    void loadUsers(Bank &bank);
    void loadAccounts(Bank &bank);
    void loadTransactions(Bank &bank);
    void loadRates(Bank &bank);

public:
    TextFileStorage(std::string usersFile = "data/users.txt",
                    std::string accountsFile = "data/accounts.txt",
                    std::string txFile = "data/transactions.txt",
                    std::string ratesFile = "data/rates.txt");

    void load(Bank &bank) override;
    void save(const Bank &bank) override;
};

#endif // STORAGE_H
