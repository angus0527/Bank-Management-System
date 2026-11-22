# 银行综合业务管理系统（课程设计报告书 · 同学 B）

> **姓名**：【请填写本人姓名】 | **学号**：【请填写本人学号】 | **班级**：2026 级人工智能（拔尖班） | **指导教师**：沃焱

## 一、选题背景

针对传统单文件银行程序耦合度高、难维护的问题，本小组基于 C++17 实现《银行综合业务管理系统》。系统用 `double`（元）存金额与小数利率并配合 `round2()` 保留两位小数，以 `bool`/`-1`/`nullptr` 拦截非法操作，随机生成 `622202` 开头 16 位卡号，支持客户与管理员双角色运行。分工如下：

| 层级 | 核心类/文件 | 主要职责 | 负责人 |
|---|---|---|---|
| 核心与门面 | `Account` 三子类、`Date`、`Transaction`、`Bank` | 多态计息、日期、流水与存取转门面 | 同学 A |
| 用户与权限 | `User`、`Customer`、`Admin` | 多态继承、密码混淆与序列化 | 同学 B |
| 存储与日志 | `Storage`、`TextFileStorage`、`Logger` | 四表持久化、路径探测与审计日志 | 同学 B |
| 控制台界面 | `ConsoleUI`、`console_main.cpp` | `switch` 菜单、流水筛选与 CSV 导出 | 同学 B |

## 二、系统设计

### 1、分层架构与类体系
系统分为三层：**界面层**（`console_main.cpp`、`ConsoleUI`）负责交互并调用 `Bank`；**业务层**（`Bank`、`User`、`Account`、`Date`、`Transaction`）用 `map` 与 `unique_ptr` 管理对象；**存储层**（`Storage`、`TextFileStorage`、`Logger`）读写 `data/` 文件。包含三大继承体系：`User → Customer/Admin`、`Account → Current/Fixed/CreditAccount`、`Storage → TextFileStorage`。

### 2、本人负责核心类
- **`User / Customer / Admin`**：基类含 `id`、`name`、`password`（密文），提供 `encryptPassword()`、`verifyPassword()`、`changePassword()` 及纯虚函数 `role()`、`serialize()`；`Customer` 维护 `phoneNumber` 与持卡列表 `accountId`；`Admin` 拥有风控与统计权限。
- **`Storage / TextFileStorage`**：`Storage` 定义 `load(Bank&)`、`save(const Bank&)`；`TextFileStorage` 提供 `resolvePath()` 与四表读写。
- **`Logger`**：静态类，提供 `log()`（自动建目录写时间戳日志）与 `getAllLogs()`。
- **`ConsoleUI / console_main`**：提供 `customerMenu()`、`adminMenu()`、`showCustomerTransactions()`、`showAdminTransactions()` 与 `readTxFilter()`。

### 3、同行转账流程
界面先用 `hasAccount()` 校验付款卡归属，再调用 `Bank::transfer()`：检查卡号不同、账户存在、金额为正、状态正常且非定期；通过后先 `from->withdraw(amount)`，成功再 `to->deposit(amount)` 并记 `TRF` 流水。

## 三、系统实现

1. **加密与序列化**：`User::encryptPassword()` 用 `^ 0x5A` 混淆口令，构造 `User` 统一传密文防二次异或；`Customer`/`Admin` 重写 `serialize()`，由 `saveUsers()` 多态存盘。
2. **四表持久化**：`TextFileStorage` 以 `U`/`A`/`T`/`R` 标记读写四表（`accounts.txt` 固定 14 列）。`loadAccounts()` 按 `C`/`F`/`X` 建子类并调用 `Customer::addAccount()` 重建绑卡；`resolvePath()` 兼容多目录启动。
3. **卡号与计息**：`Bank::generateAccountId()` 用 `mt19937_64` 生成 `622202` 开头 16 位卡号。活期按日计息；定期到期按定率、提前按活期，开启 `autoRenew` 由 `Bank::settleInterest()` 滚存；信用支持透支与按日扣息。
4. **交互与隔离**：`ConsoleUI` 拆分客户与管理员流水查询并支持 CSV 导出；`switch` 菜单用 `cin.clear()` 与 `ignore()` 拦截非法输入。

## 四、结果分析

在 Windows 11 + MinGW-w64（GCC 15.2.0）以 `-std=c++17 -Wall -Wextra` 零警告编译通过，完成 8 组核心测试与 7 组压测：

| 编号 | 功能 | 测试要点与结果 | 结论 |
|---|---|---|---|
| T01 | 注册登录 | 分配 `20001+` ID；错密返回 `nullptr`，正密进菜单 | 通过 |
| T02 | 三类开户 | 生成 16 位 `622202` 卡号，非法存期（6 月）拦截 | 通过 |
| T03 | 活期转账 | `1000` 元存 31 天结息 `0.30`；同卡/超额拦截 | 通过 |
| T04 | 定期转存 | 提前支取按活期得 `10002.92`；两年转存 `10353.06` | 通过 |
| T05 | 信用销户 | 透支 `100` 还 `60` 计 10 天息 `0.20`；欠款禁销户 | 通过 |
| T06 | 风控校验 | 拦截重复冻结与非法解冻；冻结期禁存、取、转、息 | 通过 |
| T07 | 管理统计 | 登录 `10001` 查 125 账户总览、全行结息及日志 | 通过 |
| T08 | 压测重载 | 62 用户、125 账户、229+ 流水及百户千条压测一致 | 通过 |

全部用例与 7 组压测均通过，验证了 `double` + `round2()` 精度与持久化稳定性。

## 五、课程设计总结

十二周实践让我经历了需求分析、类图设计、并行编码与压测验收全过程。通过 `User`、`Account` 和 `Storage` 接口体系，体会到面向对象解耦对团队协作的价值；联调中解决了目录缺失写日志失败、读档漏绑户主、信用结息账号为 `0` 及重复冻结等隐患。后续可在 `Bank` 增加 `lastError` 与 `getLastError()` 替代核心层 `cout` 提示，便于扩展图形界面。

## 参考文献

[1] Bjarne Stroustrup. The C++ Programming Language (4th Ed.)[M]. Addison-Wesley, 2013.
[2] Stanley B. Lippman, et al. C++ Primer (5th Ed.)[M]. Addison-Wesley, 2012.
[3] cppreference.com. Filesystem & Random library (C++17)[EB/OL]. https://en.cppreference.com/, 2026.
