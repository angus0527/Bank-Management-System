#include "../include/Bank.h"

#include <ctime>
#include <iostream>
#include <random>

#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"

// 随机生成不重复的 16 位银行卡号
long long Bank::generateAccountId() const {
    static std::mt19937_64 rng(static_cast<unsigned long long>(std::time(nullptr)));
    std::uniform_int_distribution<long long> dist(0, 9999999999LL);
    while (true) {
        long long id = 6222020000000000LL + dist(rng);
        //判断当前生成id是否重复
        if (accounts.count(id) == 0) {
            return id;
        }
    }
}


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
    //找不到用户id或者密码验证失败则返回空指针
    if (it == users.end() || !it->second->verifyPassword(password)) {
        std::cout << "登录失败：用户 " << userId << " 不存在或密码错误。\n";
        return nullptr;
    }
    return it->second.get();
}


long long Bank::registerCustomer(const std::string &name, const std::string &password, const std::string &phone) {
    if (name.empty() || password.empty()) {
        std::cout << "注册失败：姓名和密码不能为空。\n";
        return -1;
    }
    long long id = nextUserId++;
    users[id] = std::make_unique<Customer>(id, name, User::encryptPassword(password), phone);
    return id;
}


// 修改姓名
bool Bank::updateUserName(long long userId, const std::string &newName) {
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
    return true;
}

// 修改电话
bool Bank::updateCustomerPhone(long long userId, const std::string &newPhone) {
    Customer *cust = dynamic_cast<Customer *>(findUser(userId));
    if (cust == nullptr) {
        std::cout << "操作失败：客户 " << userId << " 不存在。\n";
        return false;
    }
    if (newPhone.empty()) {
        std::cout << "操作失败：电话不能为空。\n";
        return false;
    }
    cust->setPhoneNumber(newPhone);
    return true;
}

// 修改密码
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


long long Bank::openCurrentAccount(long long ownerId, double initialDeposit, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    initialDeposit = round2(initialDeposit);
    if (initialDeposit < 0.0) {
        std::cout << "操作失败：开户初始金额不能为负。\n";
        return -1;
    }
    long long id = generateAccountId();
    auto account = std::make_unique<CurrentAccount>(id, ownerId, initialDeposit, openDate, currentRate);
    //如果0元开户就不记存款流水
    if (initialDeposit > 0.0) {
        recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "开户存入");
    }
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}


long long Bank::openFixedAccount(long long ownerId, double initialDeposit, int termMonths,
                                 bool autoRenew, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    initialDeposit = round2(initialDeposit);
    if (initialDeposit <= 0.0) {
        std::cout << "操作失败：定期开户金额必须大于 0。\n";
        return -1;
    }
    long long id = generateAccountId();
    auto account = std::make_unique<FixedAccount>(id, ownerId, initialDeposit, openDate,
                                                  termMonths, getFixedRate(termMonths),
                                                  autoRenew, currentRate);
    recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "定期开户");
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}


long long Bank::openCreditAccount(long long ownerId, double creditLimit, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    creditLimit = round2(creditLimit);
    if (creditLimit <= 0.0) {
        std::cout << "操作失败：信用额度必须大于 0。\n";
        return -1;
    }
    long long id = generateAccountId();
    auto account = std::make_unique<CreditAccount>(id, ownerId, 0.0, openDate, creditLimit, creditDailyRate);
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}

// 存款
bool Bank::deposit(long long accountId, double amount, const std::string &remark) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    if (!acc->deposit(amount)) return false;
    recordTx(TransactionType::DEP, 0, accountId, round2(amount), acc->getBalance(),
             remark.empty() ? "存款/还款" : remark);
    return true;
}

// 取款
bool Bank::withdraw(long long accountId, double amount, const std::string &remark) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    if (!acc->withdraw(amount)) return false;
    recordTx(TransactionType::WDR, accountId, 0, round2(amount), acc->getBalance(),
             remark.empty() ? "取款/消费" : remark);
    return true;
}

//因为定期账户取款必须一次取完，所以与上面的函数区分
double Bank::withdrawFixed(long long accountId, const Date &today) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return -1.0;
    }
    FixedAccount *fixed = dynamic_cast<FixedAccount *>(acc);
    if (fixed == nullptr) {
        std::cout << "操作失败：该账号不是定期账户。\n";
        return -1.0;
    }
    if (!fixed->isNormal()) {
        std::cout << "操作失败：账户已" << fixed->getStatusString() << "，禁止支取。\n";
        return -1.0;
    }
    if (fixed->getBalance() <= 0.005) {
        std::cout << "操作失败：定期账户余额为 0。\n";
        return -1.0;
    }

    if (fixed->isAutoRenew() && today >= fixed->getMaturityDate()) {
        settleInterest(accountId, today);
    }

    double interest = fixed->calcInterest(today);
    if (interest > 0.0) {
        fixed->setBalance(fixed->getBalance() + interest);
        recordTx(TransactionType::INT, 0, accountId, interest, fixed->getBalance(),
                 today < fixed->getMaturityDate() ? "定期提前支取活期结息" : "定期到期结息");
    }
    double payout = fixed->getBalance();
    if (!fixed->withdraw(payout)) return -1.0;
    recordTx(TransactionType::WDR, accountId, 0, payout, 0.0, "定期支取本息");
    return payout;
}


bool Bank::transfer(long long fromAccountId, long long toAccountId, double amount, const std::string &remark) {
    amount = round2(amount);
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
    if (amount <= 0.0) {
        std::cout << "操作失败：转账金额必须大于 0。\n";
        return false;
    }
    if (!from->isNormal() || !to->isNormal()) {
        std::cout << "操作失败：转账涉及的账户状态异常。\n";
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


double Bank::settleInterest(long long accountId, const Date &today) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return -1.0;
    }
    if (!acc->isNormal()) {
        std::cout << "操作失败：账户已" << acc->getStatusString() << "，无法结息。\n";
        return -1.0;
    }
    double interest = acc->settleInterest(today);
    if (interest > 0.0) {
        if (acc->getAccountType() == AccountType::CREDIT) {
            recordTx(TransactionType::INT, accountId, 0, interest, acc->getBalance(), "信用透支计息");
        } else {
            recordTx(TransactionType::INT, 0, accountId, interest, acc->getBalance(), "账户结息");
        }
    }
    return interest;
}


int Bank::processMaturityRollovers(const Date &today) {
    int count = 0;//记录自动转存账户数
    for (const auto &[aid, acc] : accounts) {
        FixedAccount *fix = dynamic_cast<FixedAccount *>(acc.get());
        //条件:必须是定期账户，账户状态正常，账户开启了自动转存，账户的余额大于0
        if (!fix || !fix->isNormal() || !fix->isAutoRenew() || fix->getBalance() <= 0.0) continue;
        if (today >= fix->getMaturityDate()) {
            settleInterest(aid, today);
            count++;
        }
    }
    return count;
}

// 冻结
bool Bank::freezeAccount(long long accountId) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    return acc->freeze();
}

// 解冻
bool Bank::unfreezeAccount(long long accountId) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    return acc->unfreeze();
}

// 销户
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

bool Bank::setCurrentRate(double rate) {
    if (rate < 0.0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    currentRate = rate;
    for (const auto &[aid, acc] : accounts) {
        if (CurrentAccount *cur = dynamic_cast<CurrentAccount *>(acc.get())) cur->setRate(rate);
        else if (FixedAccount *fix = dynamic_cast<FixedAccount *>(acc.get())) fix->setCurrentRate(rate);
    }
    return true;
}


bool Bank::setFixedRate(int termMonths, double rate) {
    if (rate < 0.0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    switch (termMonths) {
        case 3:
            fixed3MRate = rate;
            break;
        case 12:
            fixed1YRate = rate;
            break;
        case 36:
            fixed3YRate = rate;
            break;
        default:
            std::cout << "操作失败：定期档位只支持 3 / 12 / 36 个月。\n";
            return false;
    }

    for (const auto &[aid, acc] : accounts) {
        if (FixedAccount *fix = dynamic_cast<FixedAccount *>(acc.get())) {
            if (fix->getTermMonths() == termMonths) fix->setRate(rate);
        }
    }
    return true;
}

// 调整全行信用透支日利率，并同步到现有信用账户
bool Bank::setCreditDailyRate(double rate) {
    if (rate < 0.0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    creditDailyRate = rate;
    for (const auto &[aid, acc] : accounts) {
        if (CreditAccount *cr = dynamic_cast<CreditAccount *>(acc.get())) cr->setDailyRate(rate);
    }
    return true;
}

// 按存期月数查询当前定期基准年利率
double Bank::getFixedRate(int termMonths) const {
    if (termMonths == 3) return fixed3MRate;
    if (termMonths == 12) return fixed1YRate;
    if (termMonths == 36) return fixed3YRate;
    return 0.0;
}

// 启动读档时一次性恢复 5 档基准利率配置
void Bank::initRates(double cur, double f3m, double f1y, double f3y, double cr) {
    currentRate = cur;
    fixed3MRate = f3m;
    fixed1YRate = f1y;
    fixed3YRate = f3y;
    creditDailyRate = cr;
}

// 按账号、类型、日期范围过滤流水（"YYYY-MM-DD" 字符串直接按字典序比较）
std::vector<const Transaction *> Bank::queryTransactions(long long accountId, int typeFilter,
                                                         const std::string &startDate,
                                                         const std::string &endDate) const {
    std::vector<const Transaction *> result;
    for (const Transaction &t : transactions) {
        if (accountId != 0 && t.getFromAccountId() != accountId && t.getToAccountId() != accountId) continue;
        //判断交易流水类型
        if (typeFilter != 0 && static_cast<int>(t.getType()) != typeFilter) continue;
        //判断日期是否在内
        std::string txDay = t.getDatetime().substr(0, 10);
        if (!startDate.empty() && txDay < startDate) continue;
        if (!endDate.empty() && txDay > endDate) continue;
        result.push_back(&t);
    }
    return result;
}

// 按账号查找账户指针，找不到返回 nullptr
Account *Bank::findAccount(long long accountId) {
    auto it = accounts.find(accountId);
    if (it == accounts.end()) return nullptr;
    return it->second.get();
}

// 按用户 ID 查找用户指针，找不到返回 nullptr
User *Bank::findUser(long long userId) {
    auto it = users.find(userId);
    if (it == users.end()) return nullptr;
    return it->second.get();
}

// 生成并追加一条新交易流水
void Bank::recordTx(TransactionType type, long long fromId, long long toId, double amount,
                    double balanceSnapshot, const std::string &remark) {
    transactions.push_back(Transaction(nextTransactionId++, Date::nowDateTime(), type, fromId, toId,
                                       amount, balanceSnapshot, remark));
}

// 装载用户对象并顺延下一个可用用户 ID（读档用）
bool Bank::addUser(std::unique_ptr<User> user) {
    long long id = user->getId();
    if (users.count(id) != 0) {
        std::cout << "操作失败：用户 " << id << " 重复装载。\n";
        return false;
    }
    if (id >= nextUserId) nextUserId = id + 1;
    users[id] = std::move(user);
    return true;
}

// 装载账户对象（读档用）
void Bank::addAccount(std::unique_ptr<Account> account) {
    long long id = account->getAccountId();
    accounts[id] = std::move(account);
}

// 装载流水记录并顺延下一个可用流水号（读档用）
void Bank::addTransaction(const Transaction &t) {
    if (t.getId() >= nextTransactionId) nextTransactionId = t.getId() + 1;
    transactions.push_back(t);
}
