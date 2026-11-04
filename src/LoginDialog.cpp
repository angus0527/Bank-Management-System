#include "LoginDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "Bank.h"
#include "Logger.h"
#include "exception.h"

LoginDialog::LoginDialog(Bank &bank, QWidget *parent)
    : QDialog(parent), bank(bank) {
    setWindowTitle("银行管理系统 - 登录");
    setFixedSize(360, 200);

    idEdit = new QLineEdit(this);
    idEdit->setPlaceholderText("请输入用户 ID（如 20001 / 10001）");

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("请输入登录密码");
    passwordEdit->setEchoMode(QLineEdit::Password);

    loginButton = new QPushButton("登录", this);
    exitButton = new QPushButton("退出", this);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(30, 25, 30, 25);

    layout->addWidget(new QLabel("用户账号:"));
    layout->addWidget(idEdit);
    layout->addWidget(new QLabel("登录密码:"));
    layout->addWidget(passwordEdit);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(loginButton);
    btnLayout->addWidget(exitButton);
    layout->addLayout(btnLayout);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::handleLogin);
    connect(exitButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::handleLogin);
}

void LoginDialog::handleLogin() {
    QString idText = idEdit->text().trimmed();
    QString pwdText = passwordEdit->text();

    if (idText.isEmpty() || pwdText.isEmpty()) {
        QMessageBox::warning(this, "提示", "账号或密码不能为空！");
        return;
    }

    bool ok = false;
    long long userId = idText.toLongLong(&ok);
    if (!ok) {
        QMessageBox::warning(this, "提示", "用户 ID 必须为纯数字！");
        return;
    }

    try {
        currentUser = bank.login(userId, pwdText.toStdString());
        Logger::log("用户 " + std::to_string(userId) + " 登录成功");
        accept();
    } catch (const AuthFail &e) {
        QMessageBox::warning(this, "登录失败", e.what());
    }
}
