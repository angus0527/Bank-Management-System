#include "DepositWithdrawDialog.h"

#include <cmath>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "Bank.h"
#include "Logger.h"
#include "exception.h"

DepositWithdrawDialog::DepositWithdrawDialog(Bank &bank, long long accountId, Mode mode, QWidget *parent)
    : QDialog(parent), bank(bank), accountId(accountId), mode(mode) {
    bool isDeposit = (mode == Mode::Deposit);
    setWindowTitle(isDeposit ? "账户存款" : "账户取款");
    setFixedSize(380, 240);

    Account *acc = bank.findAccount(accountId);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(25, 20, 25, 20);

    auto *infoLayout = new QHBoxLayout();
    infoLayout->addWidget(new QLabel(QString("当前账号: %1").arg(accountId)));
    balanceLabel = new QLabel(QString("可用余额: %1 元").arg(QString::fromStdString(formatMoney(acc->getBalance()))));
    balanceLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");
    infoLayout->addStretch();
    infoLayout->addWidget(balanceLabel);
    layout->addLayout(infoLayout);

    layout->addWidget(new QLabel(isDeposit ? "存款金额 (元):" : "取款金额 (元):"));
    amountEdit = new QLineEdit(this);
    amountEdit->setPlaceholderText("请输入金额，最多两位小数，如 100.00");
    layout->addWidget(amountEdit);

    layout->addWidget(new QLabel("交易备注:"));
    remarkEdit = new QLineEdit(this);
    remarkEdit->setText(isDeposit ? "柜面存入" : "ATM取款");
    layout->addWidget(remarkEdit);

    auto *btnLayout = new QHBoxLayout();
    confirmButton = new QPushButton(isDeposit ? "确认存款" : "确认取款", this);
    confirmButton->setStyleSheet("background-color: #0d6efd; color: white; padding: 6px 16px; font-weight: bold;");
    cancelButton = new QPushButton("取消", this);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelButton);
    btnLayout->addWidget(confirmButton);
    layout->addLayout(btnLayout);

    connect(confirmButton, &QPushButton::clicked, this, &DepositWithdrawDialog::handleSubmit);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(amountEdit, &QLineEdit::returnPressed, this, &DepositWithdrawDialog::handleSubmit);
}

void DepositWithdrawDialog::handleSubmit() {
    QString text = amountEdit->text().trimmed();
    if (text.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入操作金额！");
        return;
    }

    QRegularExpression re(R"(^\d+(\.\d{1,2})?$)");
    if (!re.match(text).hasMatch()) {
        QMessageBox::warning(this, "提示", "金额格式不正确，请输入正数且最多保留两位小数！");
        return;
    }

    double val = text.toDouble();
    if (val <= 0.0) {
        QMessageBox::warning(this, "提示", "金额必须大于 0！");
        return;
    }

    long long cents = std::llround(val * 100.0);
    std::string remark = remarkEdit->text().trimmed().toStdString();
    if (remark.empty()) {
        remark = (mode == Mode::Deposit) ? "存款" : "取款";
    }

    try {
        if (mode == Mode::Deposit) {
            bank.deposit(accountId, cents, remark);
            Logger::log("账户 " + std::to_string(accountId) + " 存款 " + formatMoney(cents) + " 元");
            QMessageBox::information(this, "操作成功",
                                     QString("存款成功！\n存入金额: %1 元").arg(QString::fromStdString(formatMoney(cents))));
        } else {
            bank.withdraw(accountId, cents, remark);
            Logger::log("账户 " + std::to_string(accountId) + " 取款 " + formatMoney(cents) + " 元");
            QMessageBox::information(this, "操作成功",
                                     QString("取款成功！\n取出金额: %1 元").arg(QString::fromStdString(formatMoney(cents))));
        }
        accept();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "操作失败", QString::fromUtf8(e.what()));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "操作失败", QString::fromUtf8(e.what()));
    }
}
