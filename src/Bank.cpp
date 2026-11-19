#include "../include/Bank.h"

#include "CreditAccount.h"
#include "CurrentAccount.h"
#include "FixedAccount.h"

// 检查输入文本是否包含存储分隔符 '|' 或换行符，防止破坏纯文本序列化协议结构
static bool hasReservedChar(const std::string &s) {
    return s.find('|') != std::string::npos ||
           s.find('\n') != std::string::npos ||
           s.find('\r') != std::string::npos;
}

// 用户登录鉴权：账号不存在与密码不匹配统一抛出 AuthFail，避免泄露用户存在性
User *Bank::login(long long userId, const std::string &password) {
    auto it = users.find(userId);
    if (it == users.end() || !it->second->verifyPassword(password)) {
        throw AuthFail("用户 " + std::to_string(userId) + " 登录失败：用户名或密码错误");
    }
    return it->second.get();
}

// 修改个人基本资料：校验姓名非空且不含保留字符；若为客户角色同步更新联系电话
void Bank::updateUserInfo(long long userId, const std::string &newName, const std::string &newPhone) {
    User *user = findUser(userId);
    if (newName.empty() || hasReservedChar(newName)) {
        throw InvalidAmount("姓名不能为空且不能包含 | 或换行符");
    }
    if (hasReservedChar(newPhone)) {
        throw InvalidAmount("联系电话不能包含 | 或换行符");
    }
    user->setName(newName);
    if (auto *cust = dynamic_cast<Customer *>(user)) {
        if (!newPhone.empty()) {
            cust->setPhoneNumber(newPhone);
        }
    }
}

// 修改登录密码：先核验原密码，再校验新密码合法性后执行位混淆覆盖
void Bank::changeUserPassword(long long userId, const std::string &oldPassword, const std::string &newPassword) {
    User *user = findUser(userId);
    if (!user->verifyPassword(oldPassword)) {
        throw AuthFail("原密码校验失败，无法修改密码");
    }
    if (newPassword.empty() || hasReservedChar(newPassword)) {
        throw InvalidAmount("新密码不能为空且不能包含 | 或换行符");
    }
    user->changePassword(newPassword);
}

// 开立活期账户：仅客户身份允许开户；生成新账号后绑定到客户名下，若有首存则追加开户流水
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
                                                    currentRateX10000);
    if (initialDeposit > 0) {
        recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit, "开户存入");
    }
    customer->addAccount(id);
    accounts.emplace(id, std::move(account));
    return id;
}

// 开立定期账户（3/12/36 个月）：按当前挂牌对应档位定期利率与活期参照利率创建 FixedAccount
long long Bank::openFixedAccount(long long ownerId, long long initialDeposit, int termMonths,
                                 bool autoRenew, const Date &openDate) {
    User *owner = findUser(ownerId);
    auto *customer = dynamic_cast<Customer *>(owner);
    if (customer == nullptr) {
        throw InvalidAmount("只有客户身份可以开户，用户 " + std::to_string(ownerId) + " 是管理员");
    }
    if (initialDeposit <= 0) {
        throw InvalidAmount("定期存款开户金额必须大于 0");
    }
    long long rate = getFixedRateX10000(termMonths);
    long long id = nextAccountId++;
    auto account = std::make_unique<FixedAccount>(id, ownerId, initialDeposit, openDate,
                                                  termMonths, rate, autoRenew, currentRateX10000);
    recordTx(TransactionType::DEP, 0, id, initialDeposit, initialDeposit,
             "定期开户(" + std::to_string(termMonths) + "个月)");
    customer->addAccount(id);
    accounts.emplace(id, std::move(account));
    return id;
}

// 开立信用账户：校验授信额度大于 0，初始本金为 0，绑定当前日利率
long long Bank::openCreditAccount(long long ownerId, long long creditLimit, const Date &openDate) {
    User *owner = findUser(ownerId);
    auto *customer = dynamic_cast<Customer *>(owner);
    if (customer == nullptr) {
        throw InvalidAmount("只有客户身份可以开户，用户 " + std::to_string(ownerId) + " 是管理员");
    }
    if (creditLimit <= 0) {
        throw InvalidAmount("信用账户授信额度必须大于 0");
    }
    long long id = nextAccountId++;
    auto account = std::make_unique<CreditAccount>(id, ownerId, 0, openDate, creditLimit, creditDailyRateX10000);
    customer->addAccount(id);
    accounts.emplace(id, std::move(account));
    return id;
}

// 存款 / 信用还款：多态调用 Account::deposit，成功后记录余额快照与交易流水
void Bank::deposit(long long accountId, long long amount, const std::string &remark) {
    if (hasReservedChar(remark)) {
        throw InvalidAmount("交易备注不能包含 | 或换行符");
    }
    Account &acc = *findAccount(accountId);
    acc.deposit(amount);
    std::string finalRemark = remark.empty()
                                  ? (acc.getAccountType() == AccountType::CREDIT ? "信用还款" : "存款")
                                  : remark;
    recordTx(TransactionType::DEP, 0, accountId, amount, acc.getBalance(), finalRemark);
}

// 取款 / 信用透支消费：多态调用 Account::withdraw（CreditAccount 会在额度内透支，普通账户校验余额）
void Bank::withdraw(long long accountId, long long amount, const std::string &remark) {
    if (hasReservedChar(remark)) {
        throw InvalidAmount("交易备注不能包含 | 或换行符");
    }
    Account &acc = *findAccount(accountId);
    acc.withdraw(amount);
    std::string finalRemark = remark.empty()
                                  ? (acc.getAccountType() == AccountType::CREDIT ? "信用透支" : "取款")
                                  : remark;
    recordTx(TransactionType::WDR, accountId, 0, amount, acc.getBalance(), finalRemark);
}

// 定期全额支取流程：
// 1. 校验该账号确为定期且状态正常；
// 2. 先调用 settleOnWithdraw(today) 结算利息（未到期按活期利率×天数，已到期按定期利率）；
// 3. 若有利息产生则先记录 INT 利息流水，随后一次性全部取出并记录 WDR 流水，返回实付本息总额
long long Bank::withdrawFixed(long long accountId, const Date &today) {
    Account &acc = *findAccount(accountId);
    auto *fixed = dynamic_cast<FixedAccount *>(&acc);
    if (fixed == nullptr) {
        throw InvalidAmount("账号 " + std::to_string(accountId) + " 不是定期存款账户");
    }
    if (!fixed->isNormal()) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + fixed->getStatusString() + "，禁止支取");
    }
    if (fixed->getBalance() <= 0) {
        throw InvalidAmount("该定期账户当前余额为 0，无需支取");
    }

    bool isEarly = (today < fixed->getMaturityDate());
    long long interest = fixed->settleOnWithdraw(today);
    if (interest > 0) {
        recordTx(TransactionType::INT, 0, accountId, interest, fixed->getBalance(),
                 isEarly ? "定期提前支取活期结息" : "定期到期结息");
    }

    long long payout = fixed->getBalance();
    fixed->withdraw(payout);
    recordTx(TransactionType::WDR, accountId, 0, payout, 0,
             isEarly ? "定期提前支取本息" : "定期到期全额支取");
    return payout;
}

// 同行转账：严格执行“前置完备校验 → 原子资金变更”两阶段
// 保证任意一方账户不存在、冻结、金额非法或类型不允许时双方余额均保持不变
void Bank::transfer(long long fromAccountId, long long toAccountId, long long amount,
                    const std::string &remark) {
    if (hasReservedChar(remark)) {
        throw InvalidAmount("交易备注不能包含 | 或换行符");
    }
    if (fromAccountId == toAccountId) {
        throw InvalidAmount("转账双方不能是同一个账户");
    }
    Account &from = *findAccount(fromAccountId);
    Account &to = *findAccount(toAccountId);
    if (amount <= 0) {
        throw InvalidAmount("转账金额必须大于 0");
    }
    if (!from.isNormal()) {
        throw AccountFrozen("付款账户已" + from.getStatusString() + "，禁止转出");
    }
    if (!to.isNormal()) {
        throw AccountFrozen("收款账户已" + to.getStatusString() + "，禁止转入");
    }
    if (from.getAccountType() == AccountType::FIXED) {
        throw InvalidAmount("定期账户整存整取不支持直接转出，请先办理定期支取");
    }
    if (to.getAccountType() == AccountType::FIXED) {
        throw InvalidAmount("定期账户整存整取不支持作为转账收款方");
    }
    if (auto *creditFrom = dynamic_cast<CreditAccount *>(&from)) {
        if (amount > creditFrom->getAvailableCredit()) {
            throw InsufficientBalance("超出可用信用额度：当前可用额度 " +
                                      formatMoney(creditFrom->getAvailableCredit()) +
                                      " 元，尝试转出 " + formatMoney(amount) + " 元");
        }
    } else if (amount > from.getBalance()) {
        throw InsufficientBalance("余额不足：当前余额 " + formatMoney(from.getBalance()) +
                                  " 元，尝试转出 " + formatMoney(amount) + " 元");
    }

    from.withdraw(amount);
    to.deposit(amount);
    recordTx(TransactionType::TRF, fromAccountId, toAccountId, amount, from.getBalance(), remark);
}

// 账户结息：多态调用 Account::settleInterest(today)，区分存款结息（收款）与信用透支计息（扣款）记录流水
long long Bank::settleInterest(long long accountId, const Date &today) {
    Account &acc = *findAccount(accountId);
    if (!acc.isNormal()) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + acc.getStatusString() + "，无法结息");
    }
    long long interest = acc.settleInterest(today);
    if (interest > 0) {
        if (acc.getAccountType() == AccountType::CREDIT) {
            recordTx(TransactionType::INT, accountId, 0, interest, acc.getBalance(), "信用透支计息");
        } else if (acc.getAccountType() == AccountType::FIXED) {
            recordTx(TransactionType::INT, 0, accountId, interest, acc.getBalance(), "定期结息");
        } else {
            recordTx(TransactionType::INT, 0, accountId, interest, acc.getBalance(), "活期结息");
        }
    }
    return interest;
}

// 到期自动转存批处理：扫描全部开启 autoRenew 的正常定期账户，若已超过一个或多个到期周期，
// 使用 while 循环逐期复利结息并推进到期日，直到当前日期追平最新到期周期
int Bank::processMaturityRollovers(const Date &today) {
    int rolledCount = 0;
    for (auto &[aid, accPtr] : accounts) {
        auto *fixed = dynamic_cast<FixedAccount *>(accPtr.get());
        if (!fixed || !fixed->isNormal() || !fixed->isAutoRenew() || fixed->getBalance() <= 0) {
            continue;
        }
        while (today >= fixed->getMaturityDate()) {
            long long interest = fixed->settleInterest(today);
            if (interest > 0) {
                recordTx(TransactionType::INT, 0, aid, interest, fixed->getBalance(), "定期到期自动转存结息");
            }
            rolledCount++;
        }
    }
    return rolledCount;
}

void Bank::freezeAccount(long long accountId) {
    findAccount(accountId)->freeze();
}

void Bank::unfreezeAccount(long long accountId) {
    findAccount(accountId)->unfreeze();
}

// 销户操作：若账户尚有余额（或信用账户有未还欠款）会抛出 InvalidAmount，成功后从户主名下解绑卡号
void Bank::closeAccount(long long accountId) {
    Account &acc = *findAccount(accountId);
    acc.closeAccount();
    if (auto *customer = dynamic_cast<Customer *>(findUser(acc.getOwnerId()))) {
        customer->removeAccount(accountId);
    }
}

// 调整活期基准利率，并实时同步更新所有未销户活期账户及定期提前支取参照利率
void Bank::setCurrentRate(long long rateX10000) {
    if (rateX10000 < 0) {
        throw InvalidAmount("利率不能为负数");
    }
    currentRateX10000 = rateX10000;
    for (auto &[id, acc] : accounts) {
        if (acc->getStatus() == AccountStatus::CLOSED) continue;
        if (auto *cur = dynamic_cast<CurrentAccount *>(acc.get())) {
            cur->setRateX10000(rateX10000);
        } else if (auto *fix = dynamic_cast<FixedAccount *>(acc.get())) {
            fix->setCurrentRateX10000(rateX10000);
        }
    }
}

// 调整定期指定档位（3/12/36个月）年利率，并实时同步至对应存期的现有账户
void Bank::setFixedRate(int termMonths, long long rateX10000) {
    if (rateX10000 < 0) {
        throw InvalidAmount("利率不能为负数");
    }
    if (termMonths == 3) {
        fixed3MRateX10000 = rateX10000;
    } else if (termMonths == 12) {
        fixed1YRateX10000 = rateX10000;
    } else if (termMonths == 36) {
        fixed3YRateX10000 = rateX10000;
    } else {
        throw InvalidAmount("定期档位仅支持 3 / 12 / 36 个月");
    }
    for (auto &[id, acc] : accounts) {
        if (acc->getStatus() == AccountStatus::CLOSED) continue;
        if (auto *fix = dynamic_cast<FixedAccount *>(acc.get())) {
            if (fix->getTermMonths() == termMonths) {
                fix->setRateX10000(rateX10000);
            }
        }
    }
}

// 调整信用账户透支日利率，并实时更新所有未销户信用账户
void Bank::setCreditDailyRate(long long rateX10000) {
    if (rateX10000 < 0) {
        throw InvalidAmount("利率不能为负数");
    }
    creditDailyRateX10000 = rateX10000;
    for (auto &[id, acc] : accounts) {
        if (acc->getStatus() == AccountStatus::CLOSED) continue;
        if (auto *cr = dynamic_cast<CreditAccount *>(acc.get())) {
            cr->setDailyRateX10000(rateX10000);
        }
    }
}

long long Bank::getFixedRateX10000(int termMonths) const {
    if (termMonths == 3) return fixed3MRateX10000;
    if (termMonths == 12) return fixed1YRateX10000;
    if (termMonths == 36) return fixed3YRateX10000;
    throw InvalidAmount("定期档位仅支持 3 / 12 / 36 个月");
}

void Bank::initRates(long long cur, long long f3m, long long f1y, long long f3y, long long cr) {
    currentRateX10000 = cur;
    fixed3MRateX10000 = f3m;
    fixed1YRateX10000 = f1y;
    fixed3YRateX10000 = f3y;
    creditDailyRateX10000 = cr;
}

std::vector<const Transaction *> Bank::queryTransactions(long long accountId) const {
    return queryTransactionsFiltered(accountId, 0, "", "");
}

// 多条件组合流水检索：
// 参数 accountId 为 0 时不限制账号，typeFilter 为 0 时不限制类型，startDate/endDate 留空时不限制起止日期
std::vector<const Transaction *> Bank::queryTransactionsFiltered(long long accountId,
                                                                 int typeFilter,
                                                                 const std::string &startDate,
                                                                 const std::string &endDate) const {
    bool hasStart = !startDate.empty();
    bool hasEnd = !endDate.empty();
    Date sDate(1970, 1, 1);
    Date eDate(1970, 1, 1);
    if (hasStart) {
        sDate = Date::fromString(startDate);
    }
    if (hasEnd) {
        eDate = Date::fromString(endDate);
    }

    std::vector<const Transaction *> result;
    for (const Transaction &t : transactions) {
        if (accountId != 0 && t.getFromAccountId() != accountId && t.getToAccountId() != accountId) {
            continue;
        }
        if (typeFilter != 0 && static_cast<int>(t.getType()) != typeFilter) {
            continue;
        }
        if ((hasStart || hasEnd) && t.getDatetime().size() >= 10) {
            try {
                // 提取时间戳前 10 位日期部分（"YYYY-MM-DD"）参与区间比对
                Date txDate = Date::fromString(t.getDatetime().substr(0, 10));
                if (hasStart && txDate < sDate) continue;
                if (hasEnd && txDate > eDate) continue;
            } catch (...) {
            }
        }
        result.push_back(&t);
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
