#ifndef STORAGE_H
#define STORAGE_H

#include <string>

class Bank;

// 数据持久化抽象接口（依赖倒置）：上层只依赖此纯虚契约，方便日后由文本存储无缝切换为数据库存储
class Storage {
public:
    virtual ~Storage() = default;

    // 从持久化介质读取全部用户、账户、交易流水及利率配置，装载到 Bank 中
    virtual void load(Bank &bank) = 0;

    // 将 Bank 当前内存中的全部状态持久化写入存储介质
    virtual void save(const Bank &bank) = 0;
};

// 纯文本文件存储实现（继承自 Storage）：采用竖线 '|' 分隔的单行文本协议读写数据文件
class TextFileStorage : public Storage {
private:
    std::string usersPath;         // 用户表文件路径
    std::string accountsPath;      // 账户表文件路径
    std::string transactionsPath;  // 流水表文件路径
    std::string ratesPath;         // 利率配置文件路径

    // 按实体类别拆分保存逻辑
    void saveUsers(const Bank &bank) const;
    void saveAccounts(const Bank &bank) const;
    void saveTransactions(const Bank &bank) const;

    // 按实体类别拆分读档与多态反序列化逻辑
    void loadUsers(Bank &bank);
    void loadAccounts(Bank &bank);
    void loadTransactions(Bank &bank);

public:
    // 构造函数：支持传入自定义路径（测试用），构造时会做跨目录路径自适应探测
    TextFileStorage(std::string usersFile = "data/users.txt",
                    std::string accountsFile = "data/accounts.txt",
                    std::string txFile = "data/transactions.txt",
                    std::string ratesFile = "data/rates.txt");

    void load(Bank &bank) override;
    void save(const Bank &bank) override;
};

#endif // STORAGE_H
