#ifndef CONSOLEUI_H
#define CONSOLEUI_H

#include "Bank.h"
#include "User.h"

// 客户交互菜单
void customerMenu(Bank &bank, Customer *cust);

// 管理员交互菜单
void adminMenu(Bank &bank, User *admin);

#endif // CONSOLEUI_H
