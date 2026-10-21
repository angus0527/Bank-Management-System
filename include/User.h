//
// Created by huang on 2026/10/21.
//

#ifndef USER_H
#define USER_H
#include <string>
#include <vector>

class User {
protected:
    long long id;
    std::string name;
    std::string password;

public:
    virtual ~User() {
    };

    User(long long id, const std::string &name, const std::string &password, bool isAlreadyObfuscated = false);

    bool verifyPassword(const std::string &password) const;

    void changePassword(const std::string &newPassword);

    virtual std::string role() const = 0;

    std::string getName() const {
        return name;
    };

    long long getId() const {
        return id;
    };

    std::string getPassword() const { return password; }
};

class Admin : public User {
public:
    Admin(long long id, const std::string &name, const std::string &password, bool isAlreadyObfuscated = false);

    std::string role() const override {
        return "Admin";
    }
};

class Customer : public User {
    std::string phoneNumber;
    std::vector<long long> accountId;

public:
    Customer(long long id, const std::string &name, const std::string &password, const std::string &phoneNumber,
             bool isAlreadyObfuscated = false);

    std::string role() const override {
        return "Customer";
    }

    std::string getPhoneNumber() const {
        return phoneNumber;
    }

    void addAccount(long long id) {
        accountId.push_back(id);
    }

    bool removeAccount(long long id);

    const std::vector<long long>& getAccountIds() const {
        return accountId;
    }
};


#endif //USER_H
