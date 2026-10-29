//
// Created by angus on 2026/10/29.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>
#include <string>

// 统一异常体系（README §2 设计约定 4：业务错误一律抛自定义异常，
// UI/控制台层 catch 后提示，不允许用返回值 -1 之类约定）
class BankException : public std::runtime_error {
public:
    explicit BankException(const std::string &message) : std::runtime_error(message) {}
};

// 金额非法（≤0、初始金额为负、销户时余额未清零等）
class InvalidAmount : public BankException {
public:
    explicit InvalidAmount(const std::string &message = "金额非法") : BankException(message) {}
};

// 余额不足
class InsufficientBalance : public BankException {
public:
    explicit InsufficientBalance(const std::string &message = "余额不足") : BankException(message) {}
};

// 账户（或账户相关联的用户）不存在
class AccountNotFound : public BankException {
public:
    explicit AccountNotFound(const std::string &message = "账户不存在") : BankException(message) {}
};

// 账户被冻结或已销户，禁止交易
class AccountFrozen : public BankException {
public:
    explicit AccountFrozen(const std::string &message = "账户已冻结") : BankException(message) {}
};

// 登录鉴权失败
class AuthFail : public BankException {
public:
    explicit AuthFail(const std::string &message = "用户名或密码错误") : BankException(message) {}
};

#endif //EXCEPTION_H
