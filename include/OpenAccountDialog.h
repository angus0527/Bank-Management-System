#ifndef OPENACCOUNTDIALOG_H
#define OPENACCOUNTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class Bank;

class OpenAccountDialog : public QDialog {
    Q_OBJECT

public:
    explicit OpenAccountDialog(Bank &bank, long long customerId, QWidget *parent = nullptr);

    long long getCreatedAccountId() const { return createdAccountId; }

private slots:
    void handleSubmit();

private:
    Bank &bank;
    long long customerId;
    long long createdAccountId = 0;

    QLineEdit *amountEdit;
    QPushButton *confirmButton;
    QPushButton *cancelButton;
};

#endif // OPENACCOUNTDIALOG_H
