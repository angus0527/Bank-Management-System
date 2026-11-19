#include "../include/FixedAccount.h"

// 新开定期账户构造：校验存期档位（仅允许 3/12/36 个月）及利率合法性，自动推算到期日
FixedAccount::FixedAccount(long long id, long long ownerId, long long initialBalance,
                           const Date &openDate, int termMonths_, long long rate,
                           bool autoRenew_, long long currentRate)
    : Account(id, ownerId, initialBalance, openDate),
      rateX10000(rate),
      currentRateX10000(currentRate),
      termMonths(termMonths_),
      maturityDate(openDate.addMonths(termMonths_)),
      autoRenew(autoRenew_) {
    if (termMonths != 3 && termMonths != 12 && termMonths != 36) {
        throw InvalidAmount("定期存期仅支持 3 个月、12 个月（1 年）或 36 个月（3 年）");
    }
    if (rateX10000 < 0 || currentRateX10000 < 0) {
        throw InvalidAmount("利率不能为负");
    }
}

// 读档专用构造函数：直接恢复文本中保存的到期日（可能与自动转存后的开户日对应）
FixedAccount::FixedAccount(long long id, long long ownerId, long long initialBalance,
                           const Date &openDate, int termMonths_, const Date &maturityDate_,
                           long long rate, bool autoRenew_, long long currentRate)
    : Account(id, ownerId, initialBalance, openDate),
      rateX10000(rate),
      currentRateX10000(currentRate),
      termMonths(termMonths_),
      maturityDate(maturityDate_),
      autoRenew(autoRenew_) {
}

// 定期存款遵循“整存整取”原则，不支持向现有定期账户中途零散追加存入
void FixedAccount::deposit(long long /*amount*/) {
    throw InvalidAmount("定期存款账户（整存整取）不支持中途追加存款");
}

// 定期支取校验：必须全额一次性支取，不允许部分支取
void FixedAccount::withdraw(long long amount) {
    if (amount <= 0) {
        throw InvalidAmount("支取金额必须大于 0");
    }
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止支取");
    }
    if (amount > balance) {
        throw InsufficientBalance("余额不足：当前余额 " + formatMoney(balance) + " 元，尝试支取 " +
                                  formatMoney(amount) + " 元");
    }
    if (amount != balance) {
        throw InvalidAmount("定期存款为整存整取，请一次性全额支取（当前本息合计 " +
                            formatMoney(balance) + " 元）");
    }
    balance = 0;
}

// 多态计息实现：
// 1. 若已到期（today >= maturityDate）：按定期年利率 × 存期月数/12 计算整期利息；
// 2. 若未到期提前支取：按活期年利率 × 实际存期天数/360 计息；全程先乘后除防止整数截断
long long FixedAccount::calcInterest(const Date &today) const {
    if (balance <= 0 || status == AccountStatus::CLOSED) {
        return 0;
    }
    if (today >= maturityDate) {
        return balance * rateX10000 * termMonths / (10000LL * 12LL);
    }
    long long days = openDate.daysBetween(today);
    if (days <= 0) {
        return 0;
    }
    return balance * currentRateX10000 * days / (10000LL * 360LL);
}

// 定期正常到期结息：仅在达到到期日后执行，把定期利息滚入本金并把起息日与到期日顺延至下一周期
long long FixedAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED || balance <= 0 || today < maturityDate) {
        return 0;
    }
    long long interest = balance * rateX10000 * termMonths / (10000LL * 12LL);
    balance += interest;
    openDate = maturityDate;
    maturityDate = openDate.addMonths(termMonths);
    return interest;
}

// 定期全额支取前结息：无论是到期支取还是提前支取，均按 calcInterest 规则核算出利息并入余额
long long FixedAccount::settleOnWithdraw(const Date &today) {
    if (status == AccountStatus::FROZEN || status == AccountStatus::CLOSED) {
        throw AccountFrozen("账户" + std::to_string(accountId) + "已" + getStatusString() + "，禁止支取");
    }
    if (balance <= 0) {
        return 0;
    }
    long long interest = calcInterest(today);
    balance += interest;
    openDate = today;
    if (today >= maturityDate) {
        maturityDate = today.addMonths(termMonths);
    }
    return interest;
}

AccountType FixedAccount::getAccountType() const {
    return AccountType::FIXED;
}

// 格式化为单行文本存入 accounts.txt（末尾附加提前支取活期利率与账户状态字段）
std::string FixedAccount::serialize() const {
    return "A|" + std::to_string(accountId) + "|F|" + std::to_string(ownerId) + "|" +
           std::to_string(balance) + "|" + openDate.toString() + "|" +
           std::to_string(rateX10000) + "|" + std::to_string(termMonths) + "|" +
           maturityDate.toString() + "|" + (autoRenew ? "1" : "0") + "|0|0|" +
           std::to_string(currentRateX10000) + "|" + std::to_string(static_cast<int>(status));
}

void FixedAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【账户类型】: 定期（整存整取 " << termMonths << " 个月）\n";
    std::cout << "【年利率】: " << formatMoney(rateX10000) << "%\n";
    std::cout << "【到期日期】: " << maturityDate.toString() << "\n";
    std::cout << "【到期自动转存】: " << (autoRenew ? "是" : "否") << "\n";
}
