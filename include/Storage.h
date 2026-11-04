#ifndef STORAGE_H
#define STORAGE_H

#include <string>

class Bank;

class Storage {
public:
    virtual ~Storage() = default;
    virtual void load(Bank &bank) = 0;
    virtual void save(const Bank &bank) = 0;
};

class TextFileStorage : public Storage {
private:
    std::string usersPath;
    std::string accountsPath;
    std::string transactionsPath;

    void saveUsers(const Bank &bank) const;
    void saveAccounts(const Bank &bank) const;
    void saveTransactions(const Bank &bank) const;

    void loadUsers(Bank &bank);
    void loadAccounts(Bank &bank);
    void loadTransactions(Bank &bank);

public:
    TextFileStorage(std::string usersFile = "data/users.txt",
                    std::string accountsFile = "data/accounts.txt",
                    std::string txFile = "data/transactions.txt");

    void load(Bank &bank) override;
    void save(const Bank &bank) override;
};

#endif //STORAGE_H
