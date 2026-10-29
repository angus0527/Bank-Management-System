//
// Created by angus on 2026/10/29.
//

#include "../include/Bank.h"

#include "CurrentAccount.h"

// 活期默认年利率 0.35%（×10000 = 35），管理员利率管理（Y02）落地后可调
static constexpr long long kDefaultCurrentRateX10000 = 35;

User *Bank::login(long long userId, const std::string &password) {
    auto it = users.find(userId);
    if (it == users.end() || !it->second->verifyPassword(password)) {
        throw AuthFail("用户 " + std::to_string(userId) + " 登录失败：用户名或密码错误");
    }
    return it->second.get();
}

long long Bank::openAccount(long long ownerId, long long initialDeposit, const Date &openDate) {
    User *owner = findUser(ownerId);
    auto *customer = dynamic_cast<Customer *>(owner);
    if (customer == nullptr) {
        throw InvalidAmount("只有客户身份可以开户，用户 " + std::to_string(ownerId) + " 是管理员");
    }
    if (initialDeposit < 0) {
        throw InvalidAmount("开户初始金额不能为负");
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CurrentAccount>(id, ownerId, initialDeposit, openDate,
                                                    kDefaultCurrentRateX10000);
    if (initialDeposit > 0) {
        recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "开户存入");
    }
    customer->addAccount(id);
    accounts.emplace(id, std::move(account));
    return id;
}

void Bank::deposit(long long accountId, long long amount, const std::string &remark) {
    Account &acc = *findAccount(accountId);
    acc.deposit(amount); // 金额/状态校验与余额变更（失败抛异常，余额不动）
    recordTx(TransactionType::DEP, 0, accountId, amount, acc.getBalance(), remark);
}

void Bank::withdraw(long long accountId, long long amount, const std::string &remark) {
    Account &acc = *findAccount(accountId);
    acc.withdraw(amount);
    recordTx(TransactionType::WDR, accountId, 0, amount, acc.getBalance(), remark);
}

void Bank::transfer(long long fromAccountId, long long toAccountId, long long amount,
                    const std::string &remark) {
    if (fromAccountId == toAccountId) {
        throw InvalidAmount("转账双方不能是同一个账户");
    }
    Account &from = *findAccount(fromAccountId);
    Account &to = *findAccount(toAccountId);
    // 原子性关键：所有校验通过后才变更余额。下面的 withdraw/deposit 内部会重复
    // 校验，但走到这里必然全部通过，不会出现"扣了钱却没到账"的中间态。
    if (amount <= 0) {
        throw InvalidAmount("转账金额必须大于 0");
    }
    if (!from.isNormal()) {
        throw AccountFrozen("付款账户已" + from.getStatusString() + "，禁止转出");
    }
    if (!to.isNormal()) {
        throw AccountFrozen("收款账户已" + to.getStatusString() + "，禁止转入");
    }
    if (amount > from.getBalance()) {
        throw InsufficientBalance("余额不足：当前余额 " + formatMoney(from.getBalance()) +
                                  " 元，尝试转出 " + formatMoney(amount) + " 元");
    }
    from.withdraw(amount);
    to.deposit(amount);
    recordTx(TransactionType::TRF, fromAccountId, toAccountId, amount, from.getBalance(), remark);
}

long long Bank::settleInterest(long long accountId) {
    Account &acc = *findAccount(accountId);
    long long interest = acc.settleInterest(Date::today());
    if (interest > 0) {
        recordTx(TransactionType::INT, 0, accountId, interest, acc.getBalance(), "活期结息");
    }
    return interest;
}

void Bank::freezeAccount(long long accountId) {
    findAccount(accountId)->freeze();
}

void Bank::unfreezeAccount(long long accountId) {
    findAccount(accountId)->unfreeze();
}

void Bank::closeAccount(long long accountId) {
    Account &acc = *findAccount(accountId);
    acc.closeAccount(); // 余额不为 0 抛 InvalidAmount
    if (auto *customer = dynamic_cast<Customer *>(findUser(acc.getOwnerId()))) {
        customer->removeAccount(accountId);
    }
}

std::vector<const Transaction *> Bank::queryTransactions(long long accountId) const {
    std::vector<const Transaction *> result;
    for (const Transaction &t: transactions) {
        if (accountId == 0 || t.getFromAccountId() == accountId || t.getToAccountId() == accountId) {
            result.push_back(&t);
        }
    }
    return result;
}

Account *Bank::findAccount(long long accountId) {
    auto it = accounts.find(accountId);
    if (it == accounts.end()) {
        throw AccountNotFound("账户 " + std::to_string(accountId) + " 不存在");
    }
    return it->second.get();
}

User *Bank::findUser(long long userId) {
    auto it = users.find(userId);
    if (it == users.end()) {
        throw AccountNotFound("用户 " + std::to_string(userId) + " 不存在");
    }
    return it->second.get();
}

void Bank::recordTx(TransactionType type, long long fromId, long long toId, long long amount,
                    long long balanceSnapshot, const std::string &remark) {
    transactions.emplace_back(nextTransactionId++, Date::nowDateTime(), type, fromId, toId,
                              amount, balanceSnapshot, remark);
}

void Bank::addUser(std::unique_ptr<User> user) {
    long long id = user->getId();
    if (users.count(id) != 0) {
        throw InvalidAmount("用户 " + std::to_string(id) + " 重复装载");
    }
    users.emplace(id, std::move(user));
}

void Bank::addAccount(std::unique_ptr<Account> account) {
    long long id = account->getAccountId();
    if (id >= nextAccountId) {
        nextAccountId = id + 1; // 保证后续开户发号不冲突
    }
    accounts.emplace(id, std::move(account));
}

void Bank::addTransaction(const Transaction &t) {
    if (t.getId() >= nextTransactionId) {
        nextTransactionId = t.getId() + 1;
    }
    transactions.push_back(t);
}

void Bank::setNextAccountId(long long id) {
    nextAccountId = id;
}

void Bank::setNextTransactionId(long long id) {
    nextTransactionId = id;
}
