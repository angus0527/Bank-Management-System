#include "../include/Bank.h"

#include <iostream>

#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"

Customer *Bank::findCustomer(long long ownerId) {
    User *u = findUser(ownerId);
    if (u == nullptr) {
        std::cout << "操作失败：用户 " << ownerId << " 不存在。\n";
        return nullptr;
    }
    Customer *customer = dynamic_cast<Customer *>(u);
    if (customer == nullptr) {
        std::cout << "操作失败：只有客户可以开户。\n";
        return nullptr;
    }
    return customer;
}

User *Bank::login(long long userId, const std::string &password) {
    auto it = users.find(userId);
    if (it == users.end() || !it->second->verifyPassword(password)) {
        std::cout << "登录失败：用户 " << userId << " 不存在或密码错误。\n";
        return nullptr;
    }
    return it->second.get();
}

bool Bank::updateUserInfo(long long userId, const std::string &newName, const std::string &newPhone) {
    User *user = findUser(userId);
    if (user == nullptr) {
        std::cout << "操作失败：用户 " << userId << " 不存在。\n";
        return false;
    }
    if (newName.empty()) {
        std::cout << "操作失败：姓名不能为空。\n";
        return false;
    }
    user->setName(newName);
    if (Customer *cust = dynamic_cast<Customer *>(user)) {
        if (!newPhone.empty()) {
            cust->setPhoneNumber(newPhone);
        }
    }
    return true;
}

bool Bank::changeUserPassword(long long userId, const std::string &oldPassword, const std::string &newPassword) {
    User *user = findUser(userId);
    if (user == nullptr) {
        std::cout << "操作失败：用户 " << userId << " 不存在。\n";
        return false;
    }
    if (!user->verifyPassword(oldPassword)) {
        std::cout << "操作失败：原密码错误。\n";
        return false;
    }
    if (newPassword.empty()) {
        std::cout << "操作失败：新密码不能为空。\n";
        return false;
    }
    user->changePassword(newPassword);
    return true;
}

long long Bank::openAccount(long long ownerId, long long initialDeposit, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    if (initialDeposit < 0) {
        std::cout << "操作失败：开户初始金额不能为负。\n";
        return -1;
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CurrentAccount>(id, ownerId, initialDeposit, openDate, currentRateX10000);
    if (initialDeposit > 0) {
        recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "开户存入");
    }
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}

long long Bank::openFixedAccount(long long ownerId, long long initialDeposit, int termMonths,
                                 bool autoRenew, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    if (initialDeposit <= 0) {
        std::cout << "操作失败：定期开户金额必须大于 0。\n";
        return -1;
    }
    if (termMonths != 3 && termMonths != 12 && termMonths != 36) {
        std::cout << "操作失败：定期存期只支持 3 / 12 / 36 个月。\n";
        return -1;
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<FixedAccount>(id, ownerId, initialDeposit, openDate,
                                                  termMonths, getFixedRateX10000(termMonths),
                                                  autoRenew, currentRateX10000);
    recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "定期开户");
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}

long long Bank::openCreditAccount(long long ownerId, long long creditLimit, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    if (creditLimit <= 0) {
        std::cout << "操作失败：信用额度必须大于 0。\n";
        return -1;
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CreditAccount>(id, ownerId, 0, openDate, creditLimit, creditDailyRateX10000);
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}

bool Bank::deposit(long long accountId, long long amount, const std::string &remark) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    if (!acc->deposit(amount)) return false;
    recordTx(TransactionType::DEP, 0, accountId, amount, acc->getBalance(), remark.empty() ? "存款/还款" : remark);
    return true;
}

bool Bank::withdraw(long long accountId, long long amount, const std::string &remark) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    if (!acc->withdraw(amount)) return false;
    recordTx(TransactionType::WDR, accountId, 0, amount, acc->getBalance(), remark.empty() ? "取款/消费" : remark);
    return true;
}

// 定期全额支取：先按当前日期结算利息并入本金，再一次性取出全部本息
long long Bank::withdrawFixed(long long accountId, const Date &today) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return -1;
    }
    FixedAccount *fixed = dynamic_cast<FixedAccount *>(acc);
    if (fixed == nullptr) {
        std::cout << "操作失败：该账号不是定期账户。\n";
        return -1;
    }
    if (!fixed->isNormal()) {
        std::cout << "操作失败：账户已" << fixed->getStatusString() << "，禁止支取。\n";
        return -1;
    }
    if (fixed->getBalance() <= 0) {
        std::cout << "操作失败：定期账户余额为 0。\n";
        return -1;
    }

    long long interest = fixed->calcInterest(today);
    if (interest > 0) {
        fixed->setBalance(fixed->getBalance() + interest);
        recordTx(TransactionType::INT, 0, accountId, interest, fixed->getBalance(),
                 today < fixed->getMaturityDate() ? "定期提前支取活期结息" : "定期到期结息");
    }
    long long payout = fixed->getBalance();
    if (!fixed->withdraw(payout)) return -1;
    recordTx(TransactionType::WDR, accountId, 0, payout, 0, "定期支取本息");
    return payout;
}

// 转账：先校验双方状态与账户类型，再通过多态 withdraw/deposit 扣款和入账
bool Bank::transfer(long long fromAccountId, long long toAccountId, long long amount, const std::string &remark) {
    if (fromAccountId == toAccountId) {
        std::cout << "操作失败：转账双方不能是同一个账户。\n";
        return false;
    }
    Account *from = findAccount(fromAccountId);
    if (from == nullptr) {
        std::cout << "操作失败：付款账户 " << fromAccountId << " 不存在。\n";
        return false;
    }
    Account *to = findAccount(toAccountId);
    if (to == nullptr) {
        std::cout << "操作失败：收款账户 " << toAccountId << " 不存在。\n";
        return false;
    }
    if (amount <= 0) {
        std::cout << "操作失败：转账金额必须大于 0。\n";
        return false;
    }
    if (!from->isNormal() || !to->isNormal()) {
        std::cout << "操作失败：转账涉及的账户状态异常（已冻结或销户）。\n";
        return false;
    }
    if (from->getAccountType() == AccountType::FIXED || to->getAccountType() == AccountType::FIXED) {
        std::cout << "操作失败：定期账户不支持直接转账。\n";
        return false;
    }

    if (!from->withdraw(amount)) return false;
    to->deposit(amount);
    recordTx(TransactionType::TRF, fromAccountId, toAccountId, amount, from->getBalance(), remark);
    return true;
}

long long Bank::settleInterest(long long accountId, const Date &today) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return -1;
    }
    if (!acc->isNormal()) {
        std::cout << "操作失败：账户已" << acc->getStatusString() << "，无法结息。\n";
        return -1;
    }
    long long interest = acc->settleInterest(today);
    if (interest > 0) {
        if (acc->getAccountType() == AccountType::CREDIT) {
            recordTx(TransactionType::INT, accountId, 0, interest, acc->getBalance(), "信用透支计息");
        } else {
            recordTx(TransactionType::INT, 0, accountId, interest, acc->getBalance(), "账户结息");
        }
    }
    return interest;
}

int Bank::processMaturityRollovers(const Date &today) {
    int count = 0;
    for (const auto &pair : accounts) {
        long long aid = pair.first;
        FixedAccount *fix = dynamic_cast<FixedAccount *>(pair.second.get());
        if (!fix || !fix->isNormal() || !fix->isAutoRenew() || fix->getBalance() <= 0) continue;
        while (today >= fix->getMaturityDate()) {
            long long interest = fix->settleInterest(today);
            if (interest > 0) {
                recordTx(TransactionType::INT, 0, aid, interest, fix->getBalance(), "定期到期自动转存结息");
            }
            count++;
        }
    }
    return count;
}

bool Bank::freezeAccount(long long accountId) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    acc->freeze();
    return true;
}

bool Bank::unfreezeAccount(long long accountId) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    acc->unfreeze();
    return true;
}

bool Bank::closeAccount(long long accountId) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    if (!acc->closeAccount()) return false;
    if (Customer *customer = dynamic_cast<Customer *>(findUser(acc->getOwnerId()))) {
        customer->removeAccount(accountId);
    }
    return true;
}

bool Bank::setCurrentRate(long long rateX10000) {
    if (rateX10000 < 0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    currentRateX10000 = rateX10000;
    for (const auto &pair : accounts) {
        Account *acc = pair.second.get();
        if (CurrentAccount *cur = dynamic_cast<CurrentAccount *>(acc)) cur->setRateX10000(rateX10000);
        else if (FixedAccount *fix = dynamic_cast<FixedAccount *>(acc)) fix->setCurrentRateX10000(rateX10000);
    }
    return true;
}

bool Bank::setFixedRate(int termMonths, long long rateX10000) {
    if (rateX10000 < 0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    if (termMonths == 3) fixed3MRateX10000 = rateX10000;
    else if (termMonths == 12) fixed1YRateX10000 = rateX10000;
    else if (termMonths == 36) fixed3YRateX10000 = rateX10000;
    else {
        std::cout << "操作失败：定期档位只支持 3 / 12 / 36 个月。\n";
        return false;
    }

    for (const auto &pair : accounts) {
        if (FixedAccount *fix = dynamic_cast<FixedAccount *>(pair.second.get())) {
            if (fix->getTermMonths() == termMonths) fix->setRateX10000(rateX10000);
        }
    }
    return true;
}

bool Bank::setCreditDailyRate(long long rateX10000) {
    if (rateX10000 < 0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    creditDailyRateX10000 = rateX10000;
    for (const auto &pair : accounts) {
        if (CreditAccount *cr = dynamic_cast<CreditAccount *>(pair.second.get())) cr->setDailyRateX10000(rateX10000);
    }
    return true;
}

long long Bank::getFixedRateX10000(int termMonths) const {
    if (termMonths == 3) return fixed3MRateX10000;
    if (termMonths == 12) return fixed1YRateX10000;
    if (termMonths == 36) return fixed3YRateX10000;
    return 0;
}

void Bank::initRates(long long cur, long long f3m, long long f1y, long long f3y, long long cr) {
    currentRateX10000 = cur;
    fixed3MRateX10000 = f3m;
    fixed1YRateX10000 = f1y;
    fixed3YRateX10000 = f3y;
    creditDailyRateX10000 = cr;
}

// 按账号、类型、日期范围过滤流水（"YYYY-MM-DD" 字符串直接按字典序比较）
std::vector<const Transaction *> Bank::queryTransactions(long long accountId, int typeFilter,
                                                         const std::string &startDate,
                                                         const std::string &endDate) const {
    std::vector<const Transaction *> result;
    for (const Transaction &t : transactions) {
        if (accountId != 0 && t.getFromAccountId() != accountId && t.getToAccountId() != accountId) continue;
        if (typeFilter != 0 && static_cast<int>(t.getType()) != typeFilter) continue;
        std::string txDay = t.getDatetime().substr(0, 10);
        if (!startDate.empty() && txDay < startDate) continue;
        if (!endDate.empty() && txDay > endDate) continue;
        result.push_back(&t);
    }
    return result;
}

Account *Bank::findAccount(long long accountId) {
    auto it = accounts.find(accountId);
    if (it == accounts.end()) return nullptr;
    return it->second.get();
}

User *Bank::findUser(long long userId) {
    auto it = users.find(userId);
    if (it == users.end()) return nullptr;
    return it->second.get();
}

void Bank::recordTx(TransactionType type, long long fromId, long long toId, long long amount,
                    long long balanceSnapshot, const std::string &remark) {
    transactions.push_back(Transaction(nextTransactionId++, Date::nowDateTime(), type, fromId, toId,
                                       amount, balanceSnapshot, remark));
}

bool Bank::addUser(std::unique_ptr<User> user) {
    long long id = user->getId();
    if (users.count(id) != 0) {
        std::cout << "操作失败：用户 " << id << " 重复装载。\n";
        return false;
    }
    users[id] = std::move(user);
    return true;
}

void Bank::addAccount(std::unique_ptr<Account> account) {
    long long id = account->getAccountId();
    if (id >= nextAccountId) nextAccountId = id + 1;
    accounts[id] = std::move(account);
}

void Bank::addTransaction(const Transaction &t) {
    if (t.getId() >= nextTransactionId) nextTransactionId = t.getId() + 1;
    transactions.push_back(t);
}

void Bank::setNextAccountId(long long id) { nextAccountId = id; }
void Bank::setNextTransactionId(long long id) { nextTransactionId = id; }
