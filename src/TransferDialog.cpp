#include "TransferDialog.h"

#include <cmath>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "Bank.h"
#include "Logger.h"
#include "exception.h"

TransferDialog::TransferDialog(Bank &bank, long long fromAccountId, QWidget *parent)
    : QDialog(parent), bank(bank), fromAccountId(fromAccountId) {
    setWindowTitle("转账汇款");
    setFixedSize(380, 270);

    Account *acc = bank.findAccount(fromAccountId);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(25, 20, 25, 20);

    auto *infoLayout = new QHBoxLayout();
    infoLayout->addWidget(new QLabel(QString("付款账号: %1").arg(fromAccountId)));
    balanceLabel = new QLabel(QString("可用余额: %1 元").arg(QString::fromStdString(formatMoney(acc->getBalance()))));
    balanceLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");
    infoLayout->addStretch();
    infoLayout->addWidget(balanceLabel);
    layout->addLayout(infoLayout);

    layout->addWidget(new QLabel("收款账号:"));
    toAccountEdit = new QLineEdit(this);
    toAccountEdit->setPlaceholderText("请输入对方账号，如 62002");
    layout->addWidget(toAccountEdit);

    layout->addWidget(new QLabel("转账金额 (元):"));
    amountEdit = new QLineEdit(this);
    amountEdit->setPlaceholderText("请输入转账金额，最多两位小数");
    layout->addWidget(amountEdit);

    layout->addWidget(new QLabel("交易备注:"));
    remarkEdit = new QLineEdit(this);
    remarkEdit->setText("同行转账");
    layout->addWidget(remarkEdit);

    auto *btnLayout = new QHBoxLayout();
    confirmButton = new QPushButton("确认转账", this);
    confirmButton->setStyleSheet("background-color: #0d6efd; color: white; padding: 6px 16px; font-weight: bold;");
    cancelButton = new QPushButton("取消", this);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelButton);
    btnLayout->addWidget(confirmButton);
    layout->addLayout(btnLayout);

    connect(confirmButton, &QPushButton::clicked, this, &TransferDialog::handleSubmit);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(amountEdit, &QLineEdit::returnPressed, this, &TransferDialog::handleSubmit);
}

void TransferDialog::handleSubmit() {
    QString toAccText = toAccountEdit->text().trimmed();
    if (toAccText.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入收款账号！");
        return;
    }

    bool ok = false;
    long long toAccountId = toAccText.toLongLong(&ok);
    if (!ok || toAccountId <= 0) {
        QMessageBox::warning(this, "提示", "收款账号必须为正整数！");
        return;
    }

    if (toAccountId == fromAccountId) {
        QMessageBox::warning(this, "提示", "付款账号与收款账号不能相同！");
        return;
    }

    QString text = amountEdit->text().trimmed();
    if (text.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入转账金额！");
        return;
    }

    QRegularExpression re(R"(^\d+(\.\d{1,2})?$)");
    if (!re.match(text).hasMatch()) {
        QMessageBox::warning(this, "提示", "金额格式不正确，请输入正数且最多保留两位小数！");
        return;
    }

    double val = text.toDouble();
    if (val <= 0.0) {
        QMessageBox::warning(this, "提示", "转账金额必须大于 0！");
        return;
    }

    long long cents = std::llround(val * 100.0);
    std::string remark = remarkEdit->text().trimmed().toStdString();
    if (remark.empty()) {
        remark = "同行转账";
    }

    try {
        bank.transfer(fromAccountId, toAccountId, cents, remark);
        Logger::log("账户 " + std::to_string(fromAccountId) + " 向账户 " + std::to_string(toAccountId) +
                    " 转账 " + formatMoney(cents) + " 元");
        QMessageBox::information(this, "转账成功",
                                 QString("已成功向账号 %1 转账 %2 元！")
                                     .arg(toAccountId)
                                     .arg(QString::fromStdString(formatMoney(cents))));
        accept();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "转账失败", QString::fromUtf8(e.what()));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "转账失败", QString::fromUtf8(e.what()));
    }
}
