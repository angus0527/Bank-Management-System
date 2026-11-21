#include <iostream>
#include <string>

#include "Bank.h"
#include "ConsoleUI.h"
#include "Date.h"
#include "Logger.h"
#include "Storage.h"

#ifdef _WIN32
#include <windows.h>
#endif

// 程序主入口：初始化读档、自动处理到期定期转存、进入登录/注册主菜单
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Bank bank;
    TextFileStorage storage;
    storage.load(bank);
    bank.processMaturityRollovers(Date::today());

    bool running = true;
    while (running) {
        std::cout << "\n========= 银行账户管理系统 =========\n";
        std::cout << "1. 登录系统\n2. 注册新客户\n0. 保存并退出\n";
        std::cout << "请选择: ";
        int op = -1;
        if (!(std::cin >> op)) break;

        switch (op) {
            case 0: {
                storage.save(bank);
                Logger::log("系统退出并保存");
                std::cout << "数据已保存，再见！\n";
                running = false;
                break;
            }
            case 1: {
                long long uid;
                std::string pwd;
                std::cout << "用户ID: ";
                std::cin >> uid;
                std::cout << "密码: ";
                std::cin >> pwd;
                User *u = bank.login(uid, pwd);
                if (u != nullptr) {
                    Logger::log("用户 " + std::to_string(uid) + " 登录");
                    if (u->role() == "Admin") adminMenu(bank, u);
                    else if (Customer *c = dynamic_cast<Customer *>(u)) customerMenu(bank, c);
                }
                break;
            }
            case 2: {
                std::string name, pwd, phone;
                std::cout << "姓名: ";
                std::cin >> name;
                std::cout << "密码: ";
                std::cin >> pwd;
                std::cout << "电话: ";
                std::cin >> phone;
                long long nid = bank.registerCustomer(name, pwd, phone);
                if (nid > 0) {
                    storage.save(bank);
                    Logger::log("新客户注册 ID:" + std::to_string(nid) + " 姓名:" + name);
                    std::cout << "注册成功！您的用户ID为: " << nid << "（请牢记，用于登录）\n";
                }
                break;
            }
            default:
                std::cout << "无效选项，请重新输入。\n";
                break;
        }
    }
    return 0;
}
