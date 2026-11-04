#ifndef TRANSFERDIALOG_H
#define TRANSFERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class Bank;

class TransferDialog : public QDialog {
    Q_OBJECT

public:
    explicit TransferDialog(Bank &bank, long long fromAccountId, QWidget *parent = nullptr);

private slots:
    void handleSubmit();

private:
    Bank &bank;
    long long fromAccountId;

    QLabel *balanceLabel;
    QLineEdit *toAccountEdit;
    QLineEdit *amountEdit;
    QLineEdit *remarkEdit;
    QPushButton *confirmButton;
    QPushButton *cancelButton;
};

#endif // TRANSFERDIALOG_H
