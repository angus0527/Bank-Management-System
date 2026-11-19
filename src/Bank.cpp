#include "../include/Bank.h"

#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"

User *Bank::login(long long userId, const std::string &password) {
    auto it = users.find(userId);
    if (it == users.end() || !it->second->verifyPassword(password)) {
        throw AuthFail("用户 " + std::to_string(userId) + " 登录失败：用户名或密码错误");
    }
    return it->second.get();
}

void Bank::updateUserInfo(long long userId, const std::string &newName, const std::string &newPhone) {
    User *user = findUser(userId);
    if (newName.empty()) {
        throw InvalidAmount("姓名不能为空");
    }
    user->setName(newName);
    if (auto *cust = dynamic_cast<Customer *>(user)) {
        if (!newPhone.empty()) {
            cust->setPhoneNumber(newPhone);
        }
    }
}

void Bank::changeUserPassword(long long userId, const std::string &oldPassword, const std::string &newPassword) {
    User *user = findUser(userId);
    if (!user->verifyPassword(oldPassword)) {
        throw AuthFail("原密码错误");
    }
    if (newPassword.empty()) {
        throw InvalidAmount("新密码不能为空");
    }
    user->changePassword(newPassword);
}

long long Bank::openAccount(long long ownerId, long long initialDeposit, const Date &openDate) {
    auto *customer = dynamic_cast<Customer *>(findUser(ownerId));
    if (customer == nullptr) {
        throw InvalidAmount("只有客户可以开户");
    }
    if (initialDeposit < 0) {
        throw InvalidAmount("开户初始金额不能为负");
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CurrentAccount>(id, ownerId, initialDeposit, openDate, currentRateX10000);
    if (initialDeposit > 0) {
        recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "开户存入");
    }
    customer->addAccount(id);
    accounts.emplace(id, std::move(account));
    return id;
}

long long Bank::openFixedAccount(long long ownerId, long long initialDeposit, int termMonths,
                                 bool autoRenew, const Date &openDate) {
    auto *customer = dynamic_cast<Customer *>(findUser(ownerId));
    if (customer == nullptr) {
        throw InvalidAmount("只有客户可以开户");
    }
    if (initialDeposit <= 0) {
        throw InvalidAmount("定期开户金额必须大于 0");
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<FixedAccount>(id, ownerId, initialDeposit, openDate,
                                                  termMonths, getFixedRateX10000(termMonths),
                                                  autoRenew, currentRateX10000);
    recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "定期开户");
    customer->addAccount(id);
    accounts.emplace(id, std::move(account));
    return id;
}

long long Bank::openCreditAccount(long long ownerId, long long creditLimit, const Date &openDate) {
    auto *customer = dynamic_cast<Customer *>(findUser(ownerId));
    if (customer == nullptr) {
        throw InvalidAmount("只有客户可以开户");
    }
    if (creditLimit <= 0) {
        throw InvalidAmount("信用额度必须大于 0");
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CreditAccount>(id, ownerId, 0, openDate, creditLimit, creditDailyRateX10000);
    customer->addAccount(id);
    accounts.emplace(id, std::move(account));
    return id;
}

void Bank::deposit(long long accountId, long long amount, const std::string &remark) {
    Account &acc = *findAccount(accountId);
    acc.deposit(amount);
    recordTx(TransactionType::DEP, 0, accountId, amount, acc.getBalance(), remark.empty() ? "存款/还款" : remark);
}

void Bank::withdraw(long long accountId, long long amount, const std::string &remark) {
    Account &acc = *findAccount(accountId);
    acc.withdraw(amount);
    recordTx(TransactionType::WDR, accountId, 0, amount, acc.getBalance(), remark.empty() ? "取款/消费" : remark);
}

// 定期全额支取：先按当前日期结算利息并入本金，再一次性取出全部本息
long long Bank::withdrawFixed(long long accountId, const Date &today) {
    auto *fixed = dynamic_cast<FixedAccount *>(findAccount(accountId));
    if (fixed == nullptr) {
        throw InvalidAmount("该账号不是定期账户");
    }
    if (!fixed->isNormal()) {
        throw AccountFrozen("账户已" + fixed->getStatusString() + "，禁止支取");
    }
    if (fixed->getBalance() <= 0) {
        throw InvalidAmount("定期账户余额为 0");
    }

    long long interest = fixed->calcInterest(today);
    if (interest > 0) {
        fixed->setBalance(fixed->getBalance() + interest);
        recordTx(TransactionType::INT, 0, accountId, interest, fixed->getBalance(),
                 today < fixed->getMaturityDate() ? "定期提前支取活期结息" : "定期到期结息");
    }
    long long payout = fixed->getBalance();
    fixed->withdraw(payout);
    recordTx(TransactionType::WDR, accountId, 0, payout, 0, "定期支取本息");
    return payout;
}

// 转账：全部前置校验通过后再扣款与入账，保证原子性
void Bank::transfer(long long fromAccountId, long long toAccountId, long long amount, const std::string &remark) {
    if (fromAccountId == toAccountId) {
        throw InvalidAmount("转账双方不能是同一个账户");
    }
    Account &from = *findAccount(fromAccountId);
    Account &to = *findAccount(toAccountId);
    if (amount <= 0) {
        throw InvalidAmount("转账金额必须大于 0");
    }
    if (!from.isNormal() || !to.isNormal()) {
        throw AccountFrozen("转账涉及的账户状态异常（已冻结或销户）");
    }
    if (from.getAccountType() == AccountType::FIXED || to.getAccountType() == AccountType::FIXED) {
        throw InvalidAmount("定期账户不支持直接转账");
    }
    if (auto *cr = dynamic_cast<CreditAccount *>(&from)) {
        if (amount > cr->getAvailableCredit()) {
            throw InsufficientBalance("超出可用信用额度");
        }
    } else if (amount > from.getBalance()) {
        throw InsufficientBalance("余额不足：当前余额 " + formatMoney(from.getBalance()) + " 元");
    }

    from.withdraw(amount);
    to.deposit(amount);
    recordTx(TransactionType::TRF, fromAccountId, toAccountId, amount, from.getBalance(), remark);
}

long long Bank::settleInterest(long long accountId, const Date &today) {
    Account &acc = *findAccount(accountId);
    if (!acc.isNormal()) {
        throw AccountFrozen("账户已" + acc.getStatusString() + "，无法结息");
    }
    long long interest = acc.settleInterest(today);
    if (interest > 0) {
        if (acc.getAccountType() == AccountType::CREDIT) {
            recordTx(TransactionType::INT, accountId, 0, interest, acc.getBalance(), "信用透支计息");
        } else {
            recordTx(TransactionType::INT, 0, accountId, interest, acc.getBalance(), "账户结息");
        }
    }
    return interest;
}

int Bank::processMaturityRollovers(const Date &today) {
    int count = 0;
    for (auto &[aid, accPtr] : accounts) {
        auto *fix = dynamic_cast<FixedAccount *>(accPtr.get());
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

void Bank::freezeAccount(long long accountId) { findAccount(accountId)->freeze(); }
void Bank::unfreezeAccount(long long accountId) { findAccount(accountId)->unfreeze(); }

void Bank::closeAccount(long long accountId) {
    Account &acc = *findAccount(accountId);
    acc.closeAccount();
    if (auto *customer = dynamic_cast<Customer *>(findUser(acc.getOwnerId()))) {
        customer->removeAccount(accountId);
    }
}

void Bank::setCurrentRate(long long rateX10000) {
    if (rateX10000 < 0) throw InvalidAmount("利率不能为负");
    currentRateX10000 = rateX10000;
    for (auto &[id, acc] : accounts) {
        if (auto *cur = dynamic_cast<CurrentAccount *>(acc.get())) cur->setRateX10000(rateX10000);
        else if (auto *fix = dynamic_cast<FixedAccount *>(acc.get())) fix->setCurrentRateX10000(rateX10000);
    }
}

void Bank::setFixedRate(int termMonths, long long rateX10000) {
    if (rateX10000 < 0) throw InvalidAmount("利率不能为负");
    if (termMonths == 3) fixed3MRateX10000 = rateX10000;
    else if (termMonths == 12) fixed1YRateX10000 = rateX10000;
    else if (termMonths == 36) fixed3YRateX10000 = rateX10000;
    else throw InvalidAmount("定期档位只支持 3 / 12 / 36 个月");

    for (auto &[id, acc] : accounts) {
        if (auto *fix = dynamic_cast<FixedAccount *>(acc.get())) {
            if (fix->getTermMonths() == termMonths) fix->setRateX10000(rateX10000);
        }
    }
}

void Bank::setCreditDailyRate(long long rateX10000) {
    if (rateX10000 < 0) throw InvalidAmount("利率不能为负");
    creditDailyRateX10000 = rateX10000;
    for (auto &[id, acc] : accounts) {
        if (auto *cr = dynamic_cast<CreditAccount *>(acc.get())) cr->setDailyRateX10000(rateX10000);
    }
}

long long Bank::getFixedRateX10000(int termMonths) const {
    if (termMonths == 3) return fixed3MRateX10000;
    if (termMonths == 12) return fixed1YRateX10000;
    if (termMonths == 36) return fixed3YRateX10000;
    throw InvalidAmount("定期档位只支持 3 / 12 / 36 个月");
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
    if (it == accounts.end()) throw AccountNotFound("账户 " + std::to_string(accountId) + " 不存在");
    return it->second.get();
}

User *Bank::findUser(long long userId) {
    auto it = users.find(userId);
    if (it == users.end()) throw AccountNotFound("用户 " + std::to_string(userId) + " 不存在");
    return it->second.get();
}

void Bank::recordTx(TransactionType type, long long fromId, long long toId, long long amount,
                    long long balanceSnapshot, const std::string &remark) {
    transactions.emplace_back(nextTransactionId++, Date::nowDateTime(), type, fromId, toId,
                              amount, balanceSnapshot, remark);
}

void Bank::addUser(std::unique_ptr<User> user) {
    long long id = user->getId();
    if (users.count(id) != 0) throw InvalidAmount("用户重复装载");
    users.emplace(id, std::move(user));
}

void Bank::addAccount(std::unique_ptr<Account> account) {
    long long id = account->getAccountId();
    if (id >= nextAccountId) nextAccountId = id + 1;
    accounts.emplace(id, std::move(account));
}

void Bank::addTransaction(const Transaction &t) {
    if (t.getId() >= nextTransactionId) nextTransactionId = t.getId() + 1;
    transactions.push_back(t);
}

void Bank::setNextAccountId(long long id) { nextAccountId = id; }
void Bank::setNextTransactionId(long long id) { nextTransactionId = id; }
