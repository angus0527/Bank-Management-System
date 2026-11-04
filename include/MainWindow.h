#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>

class Bank;
class User;
class Storage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Bank &bank, User *currentUser, Storage &storage, QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void handleSaveData();
    void handleSwitchUser();
    void handleAbout();

    // 客户视图槽函数
    void onCustomerAccountChanged(int index);
    void handleCustomerDeposit();
    void handleCustomerWithdraw();
    void handleCustomerTransfer();
    void handleCustomerOpenAccount();
    void handleCustomerSettleInterest();
    void handleCustomerCloseAccount();
    void handleCustomerExportCsv();
    void filterCustomerTransactions();

    // 管理员视图槽函数
    void handleAdminFreeze();
    void handleAdminUnfreeze();
    void handleAdminCloseAccount();
    void handleAdminSettleAllInterest();
    void handleAdminRefreshLogs();
    void handleAdminClearLogs();
    void filterAdminAccounts();
    void filterAdminTransactions();

private:
    Bank &bank;
    User *currentUser;
    Storage &storage;

    QStackedWidget *stackedWidget;

    // --- 客户视图控件 ---
    QWidget *customerViewWidget;
    QLabel *custWelcomeLabel;
    QComboBox *custAccountCombo;
    QLabel *custAccountIdLabel;
    QLabel *custBalanceLabel;
    QLabel *custStatusLabel;
    QLabel *custOpenDateLabel;
    QLabel *custRateLabel;
    QTableWidget *custTxTable;
    QComboBox *custTxFilterCombo;
    QPushButton *btnCustDeposit;
    QPushButton *btnCustWithdraw;
    QPushButton *btnCustTransfer;
    QPushButton *btnCustSettle;
    QPushButton *btnCustClose;

    // --- 管理员视图控件 ---
    QWidget *adminViewWidget;
    QLabel *adminWelcomeLabel;
    // Tab 1: 账户管理
    QLineEdit *adminAccSearchEdit;
    QTableWidget *adminAccTable;
    // Tab 2: 客户名册
    QTableWidget *adminUserTable;
    // Tab 3: 全行流水
    QLineEdit *adminTxSearchEdit;
    QComboBox *adminTxTypeFilterCombo;
    QTableWidget *adminTxTable;
    // Tab 4: 审计日志
    QPlainTextEdit *adminLogViewer;
    // Tab 5: 统计看板
    QLabel *statTotalDepositLabel;
    QLabel *statTotalAccountsLabel;
    QLabel *statNormalAccountsLabel;
    QLabel *statFrozenAccountsLabel;
    QLabel *statTotalCustomersLabel;
    QLabel *statTotalTransactionsLabel;

    void initUI();
    void createMenus();
    QWidget *createCustomerView();
    QWidget *createAdminView();

    void refreshCurrentView();
    void refreshCustomerData();
    void refreshAdminData();
    long long getSelectedCustomerAccountId() const;
    long long getSelectedAdminAccountId() const;
};

#endif // MAINWINDOW_H
