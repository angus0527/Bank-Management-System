#include "../include/Account.h"

#include <cmath>
#include <iomanip>
#include <sstream>
//四舍入伍
// 将金额四舍五入保留两位小数（精确到分）
double round2(double value) {
    return std::round(value * 100.0) / 100.0;
}

// 把金额（元）格式化成固定两位小数字符串（如 1000.30）
std::string formatMoney(double yuan) {
    if (std::abs(yuan) < 0.005) {
        return "0.00";
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << round2(yuan);
    return oss.str();
}

// 构造账户基类：初始化账号、户主、余额和开户日，默认正常状态
Account::Account(long long id, long long ownerId_, double initialBalance, const Date &date)
    : accountId(id), ownerId(ownerId_), balance(round2(initialBalance)),
      status(AccountStatus::NORMAL), openDate(date) {
    if (balance < 0.0) {
        std::cout << "提示：初始金额不能为负，已按 0.00 元处理。\n";
        balance = 0.0;
    }
}

// 返回账户状态的中文名称
std::string Account::getStatusString() const {
    switch (status) {
        case AccountStatus::NORMAL:
            return "正常";
        case AccountStatus::FROZEN:
            return "冻结";
        case AccountStatus::CLOSED:
            return "销户";
        default:
            return "未知状态";
    }
}

// 存款：校验金额与状态正常后增加余额
bool Account::deposit(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：存款金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止存款。\n";
        return false;
    }
    balance = round2(balance + amount);
    return true;
}

// 取款：校验金额、状态和余额是否充足后扣减余额
bool Account::withdraw(double amount) {
    amount = round2(amount);
    if (amount <= 0.0) {
        std::cout << "操作失败：取款金额必须大于 0。\n";
        return false;
    }
    if (!isNormal()) {
        std::cout << "操作失败：账户" << accountId << "已" << getStatusString() << "，禁止取款。\n";
        return false;
    }
    if (amount > balance + 0.005) {
        std::cout << "操作失败：余额不足，当前余额 " << formatMoney(balance)
                  << " 元，尝试取款 " << formatMoney(amount) << " 元。\n";
        return false;
    }
    balance = round2(balance - amount);
    return true;
}

// 打印账户公共基本信息
void Account::displayInfo() const {
    std::cout << "----------------------------------------\n";
    std::cout << "【账号】: " << accountId << "\n";
    std::cout << "【户主id】: " << ownerId << "\n";
    std::cout << "【状态】: " << getStatusString() << "\n";
    std::cout << "【开户日期】: " << openDate.toString() << "\n";
    std::cout << "【当前余额】: " << formatMoney(balance) << " 元\n";
}

// 冻结账户（禁止存取转）
void Account::freeze() {
    status = AccountStatus::FROZEN;
}

// 解冻账户（恢复为正常状态）
void Account::unfreeze() {
    if (status == AccountStatus::FROZEN) {
        status = AccountStatus::NORMAL;
    }
}

// 销户：余额清零时才允许标记为已销户
bool Account::closeAccount() {
    if (std::abs(balance) > 0.005) {
        std::cout << "操作失败：账户尚有余额 " << formatMoney(balance) << " 元，禁止销户。\n";
        return false;
    }
    balance = 0.0;
    status = AccountStatus::CLOSED;
    return true;
}
