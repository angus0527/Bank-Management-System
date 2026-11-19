#include "../include/Bank.h"

#include <iostream>

#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"

// 校验并获取客户指针（开户专用，管理员不能开户）
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

// 用户登录验证，成功返回 User 指针，失败返回 nullptr
User *Bank::login(long long userId, const std::string &password) {
    auto it = users.find(userId);
    if (it == users.end() || !it->second->verifyPassword(password)) {
        std::cout << "登录失败：用户 " << userId << " 不存在或密码错误。\n";
        return nullptr;
    }
    return it->second.get();
}

// 注册新客户，自动分配用户 ID 并加密保存密码
long long Bank::registerCustomer(const std::string &name, const std::string &password, const std::string &phone) {
    if (name.empty() || password.empty()) {
        std::cout << "注册失败：姓名和密码不能为空。\n";
        return -1;
    }
    long long id = nextUserId++;
    users[id] = std::make_unique<Customer>(id, name, User::encryptPassword(password), phone);
    return id;
}

// 修改用户姓名与联系电话
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

// 校验原密码后修改登录密码
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

// 开立活期账户并绑定到客户名下，成功返回新账号
long long Bank::openAccount(long long ownerId, double initialDeposit, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    initialDeposit = round2(initialDeposit);
    if (initialDeposit < 0.0) {
        std::cout << "操作失败：开户初始金额不能为负。\n";
        return -1;
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CurrentAccount>(id, ownerId, initialDeposit, openDate, currentRate);
    if (initialDeposit > 0.0) {
        recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "开户存入");
    }
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}

// 开立定期账户（支持 3/12/36 个月），成功返回新账号
long long Bank::openFixedAccount(long long ownerId, double initialDeposit, int termMonths,
                                 bool autoRenew, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    initialDeposit = round2(initialDeposit);
    if (initialDeposit <= 0.0) {
        std::cout << "操作失败：定期开户金额必须大于 0。\n";
        return -1;
    }
    if (termMonths != 3 && termMonths != 12 && termMonths != 36) {
        std::cout << "操作失败：定期存期只支持 3 / 12 / 36 个月。\n";
        return -1;
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<FixedAccount>(id, ownerId, initialDeposit, openDate,
                                                  termMonths, getFixedRate(termMonths),
                                                  autoRenew, currentRate);
    recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "定期开户");
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}

// 开立信用账户，设置信用额度并绑定客户，成功返回新账号
long long Bank::openCreditAccount(long long ownerId, double creditLimit, const Date &openDate) {
    Customer *customer = findCustomer(ownerId);
    if (customer == nullptr) return -1;
    creditLimit = round2(creditLimit);
    if (creditLimit <= 0.0) {
        std::cout << "操作失败：信用额度必须大于 0。\n";
        return -1;
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CreditAccount>(id, ownerId, 0.0, openDate, creditLimit, creditDailyRate);
    customer->addAccount(id);
    accounts[id] = std::move(account);
    return id;
}

// 存款（信用账户为还款），入账并记流水
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

// 取款（信用账户为透支消费），扣款并记流水
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

// 定期全额支取：先按当前日期结算利息并入本金，再一次性取出全部本息
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

// 转账：先校验双方状态与账户类型，再通过多态 withdraw/deposit 扣款和入账
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

// 对指定账户结算截至 today 的利息并记录结息流水
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

//把所有的定期账户续订
// 批量处理全行已到期且开启自动转存的定期账户，返回转存笔数
int Bank::processMaturityRollovers(const Date &today) {
    int count = 0;
    for (const auto &pair : accounts) {
        long long aid = pair.first;
        FixedAccount *fix = dynamic_cast<FixedAccount *>(pair.second.get());
        if (!fix || !fix->isNormal() || !fix->isAutoRenew() || fix->getBalance() <= 0.0) continue;
        while (today >= fix->getMaturityDate()) {
            double interest = fix->settleInterest(today);
            if (interest > 0.0) {
                recordTx(TransactionType::INT, 0, aid, interest, fix->getBalance(), "定期到期自动转存结息");
            }
            count++;
        }
    }
    return count;
}

// 冻结指定账户
bool Bank::freezeAccount(long long accountId) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    acc->freeze();
    return true;
}

// 解冻指定账户
bool Bank::unfreezeAccount(long long accountId) {
    Account *acc = findAccount(accountId);
    if (acc == nullptr) {
        std::cout << "操作失败：账户 " << accountId << " 不存在。\n";
        return false;
    }
    acc->unfreeze();
    return true;
}

// 销户：清零校验通过后从客户持卡列表中移除
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

// 调整全行活期年利率，并同步到现有活期及定期提前支取参照利率
bool Bank::setCurrentRate(double rate) {
    if (rate < 0.0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    currentRate = rate;
    for (const auto &pair : accounts) {
        Account *acc = pair.second.get();
        if (CurrentAccount *cur = dynamic_cast<CurrentAccount *>(acc)) cur->setRate(rate);
        else if (FixedAccount *fix = dynamic_cast<FixedAccount *>(acc)) fix->setCurrentRate(rate);
    }
    return true;
}

// 调整指定存期（3/12/36个月）的定期年利率，并同步到对应定期账户
bool Bank::setFixedRate(int termMonths, double rate) {
    if (rate < 0.0) {
        std::cout << "操作失败：利率不能为负。\n";
        return false;
    }
    if (termMonths == 3) fixed3MRate = rate;
    else if (termMonths == 12) fixed1YRate = rate;
    else if (termMonths == 36) fixed3YRate = rate;
    else {
        std::cout << "操作失败：定期档位只支持 3 / 12 / 36 个月。\n";
        return false;
    }

    for (const auto &pair : accounts) {
        if (FixedAccount *fix = dynamic_cast<FixedAccount *>(pair.second.get())) {
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
    for (const auto &pair : accounts) {
        if (CreditAccount *cr = dynamic_cast<CreditAccount *>(pair.second.get())) cr->setDailyRate(rate);
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
        if (typeFilter != 0 && static_cast<int>(t.getType()) != typeFilter) continue;
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

// 装载账户对象并顺延下一个可用账号（读档用）
void Bank::addAccount(std::unique_ptr<Account> account) {
    long long id = account->getAccountId();
    if (id >= nextAccountId) nextAccountId = id + 1;
    accounts[id] = std::move(account);
}

// 装载流水记录并顺延下一个可用流水号（读档用）
void Bank::addTransaction(const Transaction &t) {
    if (t.getId() >= nextTransactionId) nextTransactionId = t.getId() + 1;
    transactions.push_back(t);
}

// 手动设置发号器起始账号
void Bank::setNextAccountId(long long id) { nextAccountId = id; }
// 手动设置发号器起始流水号
void Bank::setNextTransactionId(long long id) { nextTransactionId = id; }
