#include "../include/FixedAccount.h"

#include <cmath>
#include <sstream>

//(账户id，用户id，初始存款，开户日期，存多久，定期年利率，是否自动转纯，活期年利率）
FixedAccount::FixedAccount(long long id, long long ownerId, double initialBalance,
                           const Date &openDate, int termMonths, double rate,
                           bool autoRenew, double currentRate)
    : Account(id, ownerId, initialBalance, openDate),
      termMonths((termMonths == 3 || termMonths == 12 || termMonths == 36) ? termMonths : 12),
      rate(rate),
      currentRate(currentRate),
      maturityDate(openDate.addMonths(this->termMonths)),
      autoRenew(autoRenew) {
    if (termMonths != 3 && termMonths != 12 && termMonths != 36) {
        std::cout << "提示：定期存期仅支持 3、12 或 36 个月，已默认按 12 个月处理。\n";
    }
}

// 定期账户禁止中途追加存款
bool FixedAccount::deposit(double /*amount*/) {
    std::cout << "操作失败：定期账户不支持中途追加存款。\n";
    return false;
}

// 定期取款，必须一次性全额取出
bool FixedAccount::withdraw(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：支取金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户已" << getStatusString() << "，禁止支取。\n";
        return false;
    }
    if (std::abs(amount - balance) > 0.005) {
        std::cout << "操作失败：定期存款必须一次性全额支取，当前余额为 " << formatMoney(balance) << " 元。\n";
        return false;
    }
    balance = 0.0;
    return true;
}


double FixedAccount::calcInterest(const Date &today) const {
    if (balance <= 0.0 || status == AccountStatus::CLOSED) return 0.0;
    //如果今天在过了结息日期就按定期年利率结息
    if (today >= maturityDate) {
        return round2(balance * rate * static_cast<double>(termMonths) / 12.0);
    }
    long long days = openDate.daysBetween(today);
    if (days <= 0) return 0.0;
    return round2(balance * currentRate * static_cast<double>(days) / 360.0);
}

// 结息：未开启自动转存只结一次；开启自动转存时循环滚存全部已满期周期
double FixedAccount::settleInterest(const Date &today) {
    if (status == AccountStatus::CLOSED || balance <= 0.0 || today < maturityDate) {
        return 0.0;
    }
    if (!autoRenew) {
        double interest = calcInterest(today);
        balance = round2(balance + interest);
        openDate = maturityDate;
        maturityDate = openDate.addMonths(termMonths);
        return interest;
    }
    double totalInterest = 0.0;
    while (balance > 0.0 && today >= maturityDate) {
        double interest = calcInterest(today);
        balance = round2(balance + interest);
        totalInterest = round2(totalInterest + interest);
        openDate = maturityDate;
        maturityDate = openDate.addMonths(termMonths);
    }
    return totalInterest;
}

// A(账户)|账号|F(定期)|户主id|余额|开户日|定期年利率|存期月数|到期日|是否自动转存(1是/0否)|0|0|活期年利率|状态(1正常/2冻结/3销户)
std::string FixedAccount::serialize() const {
    std::ostringstream oss;
    oss << "A|" << accountId << "|F|" << ownerId << "|"
        << formatMoney(balance) << "|" << openDate.toString() << "|"
        << rate << "|" << termMonths << "|"
        << maturityDate.toString() << "|" << (autoRenew ? "1" : "0") << "|0|0|"
        << currentRate << "|" << static_cast<int>(status);
    return oss.str();
}

void FixedAccount::displayInfo() const {
    Account::displayInfo();
    std::cout << "【类型】: 定期（" << termMonths << "个月，到期日 " << maturityDate.toString()
              << (autoRenew ? "，自动转存）\n" : "，不自动转存）\n");
}
