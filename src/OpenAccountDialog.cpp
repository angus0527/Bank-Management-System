#include "OpenAccountDialog.h"

#include <cmath>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "Bank.h"
#include "Logger.h"
#include "User.h"
#include "exception.h"

OpenAccountDialog::OpenAccountDialog(Bank &bank, long long customerId, QWidget *parent)
    : QDialog(parent), bank(bank), customerId(customerId) {
    setWindowTitle("开立新账户");
    setFixedSize(380, 240);

    User *user = bank.findUser(customerId);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(25, 20, 25, 20);

    layout->addWidget(new QLabel(QString("开户人: %1 (ID: %2)")
                                     .arg(QString::fromStdString(user->getName()))
                                     .arg(customerId)));
    layout->addWidget(new QLabel("账户类型: 活期储蓄账户 (Current Account)"));
    layout->addWidget(new QLabel("执行利率: 年利率 0.35% (日息按日计复/单利)"));

    layout->addWidget(new QLabel("初始存入金额 (元):"));
    amountEdit = new QLineEdit(this);
    amountEdit->setText("0.00");
    amountEdit->setPlaceholderText("请输入初始存款金额，可为 0.00");
    layout->addWidget(amountEdit);

    auto *btnLayout = new QHBoxLayout();
    confirmButton = new QPushButton("立即开户", this);
    confirmButton->setStyleSheet("background-color: #0d6efd; color: white; padding: 6px 16px; font-weight: bold;");
    cancelButton = new QPushButton("取消", this);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelButton);
    btnLayout->addWidget(confirmButton);
    layout->addLayout(btnLayout);

    connect(confirmButton, &QPushButton::clicked, this, &OpenAccountDialog::handleSubmit);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(amountEdit, &QLineEdit::returnPressed, this, &OpenAccountDialog::handleSubmit);
}

void OpenAccountDialog::handleSubmit() {
    QString text = amountEdit->text().trimmed();
    if (text.isEmpty()) {
        text = "0.00";
    }

    QRegularExpression re(R"(^\d+(\.\d{1,2})?$)");
    if (!re.match(text).hasMatch()) {
        QMessageBox::warning(this, "提示", "金额格式不正确，请输入非负数且最多保留两位小数！");
        return;
    }

    double val = text.toDouble();
    if (val < 0.0) {
        QMessageBox::warning(this, "提示", "初始存款金额不能为负数！");
        return;
    }

    long long cents = std::llround(val * 100.0);

    try {
        createdAccountId = bank.openAccount(customerId, cents);
        Logger::log("客户 " + std::to_string(customerId) + " 成功开户 " + std::to_string(createdAccountId) +
                    "，初始存入 " + formatMoney(cents) + " 元");
        QMessageBox::information(this, "开户成功",
                                 QString("恭喜！新账户开立成功！\n账号: %1\n初始金额: %2 元")
                                     .arg(createdAccountId)
                                     .arg(QString::fromStdString(formatMoney(cents))));
        accept();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "开户失败", QString::fromUtf8(e.what()));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "开户失败", QString::fromUtf8(e.what()));
    }
}
