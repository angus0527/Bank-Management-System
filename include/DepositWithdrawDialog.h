#ifndef DEPOSITWITHDRAWDIALOG_H
#define DEPOSITWITHDRAWDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class Bank;

class DepositWithdrawDialog : public QDialog {
    Q_OBJECT

public:
    enum class Mode {
        Deposit,
        Withdraw
    };

    explicit DepositWithdrawDialog(Bank &bank, long long accountId, Mode mode, QWidget *parent = nullptr);

private slots:
    void handleSubmit();

private:
    Bank &bank;
    long long accountId;
    Mode mode;

    QLabel *balanceLabel;
    QLineEdit *amountEdit;
    QLineEdit *remarkEdit;
    QPushButton *confirmButton;
    QPushButton *cancelButton;
};

#endif // DEPOSITWITHDRAWDIALOG_H
