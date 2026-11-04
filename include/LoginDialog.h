#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class Bank;
class User;

class LoginDialog : public QDialog {
    Q_OBJECT // 核心魔法宏：只要这个类要用到"点击事件/信号槽"，必须在第一行写 Q_OBJECT

private:
    Bank &bank;              // 引用我们写好的银行大脑
    User *currentUser = nullptr; // 登录成功后，记录当前是谁登进来了

    // 窗口上的 4 个零件（积木）
    QLineEdit *idEdit;          // 账号输入框
    QLineEdit *passwordEdit;    // 密码输入框
    QPushButton *loginButton;   // 登录按钮
    QPushButton *exitButton;    // 退出按钮

private slots:
    // 槽函数：专门用来响应"点击登录按钮"这个事件的函数
    void handleLogin();

public:
    // 构造函数：负责把上面的零件画在窗口上
    explicit LoginDialog(Bank &bank, QWidget *parent = nullptr);

    // 登录成功后，供外部主程序拿走当前用户
    User *getLoggedInUser() const { return currentUser; }
};

#endif // LOGINDIALOG_H