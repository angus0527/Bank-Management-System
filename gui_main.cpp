#include <QApplication>
#include <QMessageBox>

#include "Bank.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include "Storage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    Bank bank;
    TextFileStorage storage;
    storage.load(bank);

    // 如果数据文件为空（如首次部署或运行路径不同），自动初始化预置数据
    if (bank.getUsers().empty()) {
        bank.addUser(std::make_unique<Admin>(10001, "admin", "admin123"));
        bank.addUser(std::make_unique<Customer>(20001, "张三", "zs123456", "13800000001"));
        bank.addUser(std::make_unique<Customer>(20002, "李四", "ls123456", "13800000002"));
        bank.openAccount(20001, 100029, Date(2026, 10, 2));
        bank.openAccount(20002, 30004, Date(2026, 10, 15));
        storage.save(bank);
    }

    LoginDialog loginDlg(bank);
    if (loginDlg.exec() != QDialog::Accepted) {
        return 0;
    }

    User *loggedInUser = loginDlg.getLoggedInUser();
    if (!loggedInUser) {
        return 0;
    }

    MainWindow mainWin(bank, loggedInUser, storage);
    mainWin.show();

    return app.exec();
}
