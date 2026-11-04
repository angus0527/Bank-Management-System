#include "MainWindow.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include "Bank.h"
#include "DepositWithdrawDialog.h"
#include "Logger.h"
#include "LoginDialog.h"
#include "OpenAccountDialog.h"
#include "Storage.h"
#include "TransferDialog.h"
#include "User.h"
#include "exception.h"

static QString txTypeToChinese(TransactionType type) {
    switch (type) {
        case TransactionType::DEP: return "存款";
        case TransactionType::WDR: return "取款";
        case TransactionType::TRF: return "转账";
        case TransactionType::INT: return "结息";
        default: return "其他";
    }
}

MainWindow::MainWindow(Bank &bank, User *currentUser, Storage &storage, QWidget *parent)
    : QMainWindow(parent), bank(bank), currentUser(currentUser), storage(storage) {
    initUI();
    createMenus();
    refreshCurrentView();
}

void MainWindow::initUI() {
    setWindowTitle("银行综合业务管理系统");
    resize(1020, 680);

    stackedWidget = new QStackedWidget(this);
    customerViewWidget = createCustomerView();
    adminViewWidget = createAdminView();

    stackedWidget->addWidget(customerViewWidget); // Index 0: 客户视图
    stackedWidget->addWidget(adminViewWidget);    // Index 1: 管理员视图

    setCentralWidget(stackedWidget);
    statusBar()->showMessage("系统就绪", 3000);
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");

    auto *saveAction = fileMenu->addAction("保存数据(&S)");
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::handleSaveData);

    auto *switchAction = fileMenu->addAction("切换用户(&L)");
    connect(switchAction, &QAction::triggered, this, &MainWindow::handleSwitchUser);

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction("退出系统(&X)");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    auto *aboutAction = helpMenu->addAction("关于本系统(&A)");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::handleAbout);
}

// ==================== 客户视图构建 ====================
QWidget *MainWindow::createCustomerView() {
    auto *widget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(widget);
    mainLayout->setContentsMargins(15, 12, 15, 12);
    mainLayout->setSpacing(10);

    // 顶部欢迎横幅
    auto *headerGroup = new QGroupBox("客户信息", widget);
    auto *headerLayout = new QHBoxLayout(headerGroup);
    custWelcomeLabel = new QLabel(headerGroup);
    custWelcomeLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    headerLayout->addWidget(custWelcomeLabel);
    headerLayout->addStretch();
    auto *btnQuickOpen = new QPushButton("＋ 申请开户", headerGroup);
    btnQuickOpen->setStyleSheet("background-color: #198754; color: white; padding: 4px 12px; font-weight: bold; border-radius: 4px;");
    connect(btnQuickOpen, &QPushButton::clicked, this, &MainWindow::handleCustomerOpenAccount);
    headerLayout->addWidget(btnQuickOpen);
    mainLayout->addWidget(headerGroup);

    // 中间：账户选择与详情卡片
    auto *accGroup = new QGroupBox("账户概览", widget);
    auto *accMainLayout = new QVBoxLayout(accGroup);

    auto *selectorLayout = new QHBoxLayout();
    selectorLayout->addWidget(new QLabel("选择我的账户:"));
    custAccountCombo = new QComboBox(accGroup);
    custAccountCombo->setMinimumWidth(220);
    connect(custAccountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCustomerAccountChanged);
    selectorLayout->addWidget(custAccountCombo);
    selectorLayout->addStretch();
    accMainLayout->addLayout(selectorLayout);

    // 卡片信息
    auto *cardLayout = new QHBoxLayout();
    custAccountIdLabel = new QLabel("账号: --");
    custBalanceLabel = new QLabel("余额: -- 元");
    custBalanceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #38bdf8;");
    custStatusLabel = new QLabel("状态: --");
    custOpenDateLabel = new QLabel("开户日期: --");
    custRateLabel = new QLabel("年利率: 0.35%");

    cardLayout->addWidget(custAccountIdLabel);
    cardLayout->addSpacing(20);
    cardLayout->addWidget(custBalanceLabel);
    cardLayout->addSpacing(20);
    cardLayout->addWidget(custStatusLabel);
    cardLayout->addSpacing(20);
    cardLayout->addWidget(custOpenDateLabel);
    cardLayout->addSpacing(20);
    cardLayout->addWidget(custRateLabel);
    cardLayout->addStretch();
    accMainLayout->addLayout(cardLayout);

    // 业务快捷按钮
    auto *btnLayout = new QHBoxLayout();
    btnCustDeposit = new QPushButton("存款", accGroup);
    btnCustWithdraw = new QPushButton("取款", accGroup);
    btnCustTransfer = new QPushButton("转账", accGroup);
    btnCustSettle = new QPushButton("活期结息", accGroup);
    btnCustClose = new QPushButton("申请销户", accGroup);

    btnCustDeposit->setStyleSheet("background-color: #0d6efd; color: white; padding: 5px 12px;");
    btnCustWithdraw->setStyleSheet("background-color: #fd7e14; color: white; padding: 5px 12px;");
    btnCustTransfer->setStyleSheet("background-color: #6f42c1; color: white; padding: 5px 12px;");
    btnCustSettle->setStyleSheet("background-color: #20c997; color: white; padding: 5px 12px;");
    btnCustClose->setStyleSheet("background-color: #dc3545; color: white; padding: 5px 12px;");

    connect(btnCustDeposit, &QPushButton::clicked, this, &MainWindow::handleCustomerDeposit);
    connect(btnCustWithdraw, &QPushButton::clicked, this, &MainWindow::handleCustomerWithdraw);
    connect(btnCustTransfer, &QPushButton::clicked, this, &MainWindow::handleCustomerTransfer);
    connect(btnCustSettle, &QPushButton::clicked, this, &MainWindow::handleCustomerSettleInterest);
    connect(btnCustClose, &QPushButton::clicked, this, &MainWindow::handleCustomerCloseAccount);

    btnLayout->addWidget(btnCustDeposit);
    btnLayout->addWidget(btnCustWithdraw);
    btnLayout->addWidget(btnCustTransfer);
    btnLayout->addWidget(btnCustSettle);
    btnLayout->addWidget(btnCustClose);
    btnLayout->addStretch();
    accMainLayout->addLayout(btnLayout);

    mainLayout->addWidget(accGroup);

    // 底部：历史流水表格
    auto *txGroup = new QGroupBox("账户历史交易流水", widget);
    auto *txLayout = new QVBoxLayout(txGroup);

    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("交易类型过滤:"));
    custTxFilterCombo = new QComboBox(txGroup);
    custTxFilterCombo->addItems({"全部", "存款", "取款", "转账", "结息"});
    connect(custTxFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterCustomerTransactions);
    filterLayout->addWidget(custTxFilterCombo);
    filterLayout->addStretch();

    auto *btnExport = new QPushButton("导出对账单 (CSV)", txGroup);
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::handleCustomerExportCsv);
    filterLayout->addWidget(btnExport);
    txLayout->addLayout(filterLayout);

    custTxTable = new QTableWidget(txGroup);
    custTxTable->setColumnCount(7);
    custTxTable->setHorizontalHeaderLabels({"流水号", "交易时间", "类型", "对方账号", "变动金额 (元)", "余额快照 (元)", "备注"});
    custTxTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    custTxTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    custTxTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    custTxTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    custTxTable->setAlternatingRowColors(true);
    txLayout->addWidget(custTxTable);

    mainLayout->addWidget(txGroup);
    return widget;
}

// ==================== 管理员视图构建 ====================
QWidget *MainWindow::createAdminView() {
    auto *widget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(widget);
    mainLayout->setContentsMargins(15, 12, 15, 12);
    mainLayout->setSpacing(10);

    // 顶部横幅
    adminWelcomeLabel = new QLabel("管理员控制台", widget);
    adminWelcomeLabel->setStyleSheet("font-size: 15px; font-weight: bold; padding: 4px;");
    mainLayout->addWidget(adminWelcomeLabel);

    auto *tabWidget = new QTabWidget(widget);

    // --- Tab 1: 全行账户管理 ---
    auto *tab1 = new QWidget();
    auto *tab1Layout = new QVBoxLayout(tab1);

    auto *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("搜索过滤 (账号/客户ID):"));
    adminAccSearchEdit = new QLineEdit(tab1);
    adminAccSearchEdit->setPlaceholderText("输入关键字即时过滤...");
    connect(adminAccSearchEdit, &QLineEdit::textChanged, this, &MainWindow::filterAdminAccounts);
    searchLayout->addWidget(adminAccSearchEdit);
    searchLayout->addStretch();
    tab1Layout->addLayout(searchLayout);

    adminAccTable = new QTableWidget(tab1);
    adminAccTable->setColumnCount(6);
    adminAccTable->setHorizontalHeaderLabels({"账号", "所属客户ID", "类型", "余额 (元)", "状态", "开户日期"});
    adminAccTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    adminAccTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    adminAccTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    adminAccTable->setAlternatingRowColors(true);
    tab1Layout->addWidget(adminAccTable);

    auto *tab1BtnLayout = new QHBoxLayout();
    auto *btnFreeze = new QPushButton("冻结选中账户", tab1);
    auto *btnUnfreeze = new QPushButton("解冻选中账户", tab1);
    auto *btnCloseAcc = new QPushButton("强制销户", tab1);
    auto *btnSettleAll = new QPushButton("一键全行日终结息", tab1);

    btnFreeze->setStyleSheet("background-color: #ffc107; color: black; font-weight: bold;");
    btnUnfreeze->setStyleSheet("background-color: #198754; color: white; font-weight: bold;");
    btnCloseAcc->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold;");
    btnSettleAll->setStyleSheet("background-color: #0d6efd; color: white; font-weight: bold;");

    connect(btnFreeze, &QPushButton::clicked, this, &MainWindow::handleAdminFreeze);
    connect(btnUnfreeze, &QPushButton::clicked, this, &MainWindow::handleAdminUnfreeze);
    connect(btnCloseAcc, &QPushButton::clicked, this, &MainWindow::handleAdminCloseAccount);
    connect(btnSettleAll, &QPushButton::clicked, this, &MainWindow::handleAdminSettleAllInterest);

    tab1BtnLayout->addWidget(btnFreeze);
    tab1BtnLayout->addWidget(btnUnfreeze);
    tab1BtnLayout->addWidget(btnCloseAcc);
    tab1BtnLayout->addStretch();
    tab1BtnLayout->addWidget(btnSettleAll);
    tab1Layout->addLayout(tab1BtnLayout);

    tabWidget->addTab(tab1, "全行账户管理");

    // --- Tab 2: 客户名册 ---
    auto *tab2 = new QWidget();
    auto *tab2Layout = new QVBoxLayout(tab2);
    adminUserTable = new QTableWidget(tab2);
    adminUserTable->setColumnCount(6);
    adminUserTable->setHorizontalHeaderLabels({"用户 ID", "姓名", "角色", "联系电话", "名下账户数", "总资产 (元)"});
    adminUserTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    adminUserTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    adminUserTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    adminUserTable->setAlternatingRowColors(true);
    tab2Layout->addWidget(adminUserTable);
    tabWidget->addTab(tab2, "客户名册");

    // --- Tab 3: 全行流水监控 ---
    auto *tab3 = new QWidget();
    auto *tab3Layout = new QVBoxLayout(tab3);

    auto *txFilterLayout = new QHBoxLayout();
    txFilterLayout->addWidget(new QLabel("相关账号:"));
    adminTxSearchEdit = new QLineEdit(tab3);
    adminTxSearchEdit->setPlaceholderText("留空查询全部流水，或输入账号");
    connect(adminTxSearchEdit, &QLineEdit::textChanged, this, &MainWindow::filterAdminTransactions);
    txFilterLayout->addWidget(adminTxSearchEdit);

    txFilterLayout->addWidget(new QLabel("交易类型:"));
    adminTxTypeFilterCombo = new QComboBox(tab3);
    adminTxTypeFilterCombo->addItems({"全部", "存款", "取款", "转账", "结息"});
    connect(adminTxTypeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterAdminTransactions);
    txFilterLayout->addWidget(adminTxTypeFilterCombo);
    txFilterLayout->addStretch();
    tab3Layout->addLayout(txFilterLayout);

    adminTxTable = new QTableWidget(tab3);
    adminTxTable->setColumnCount(8);
    adminTxTable->setHorizontalHeaderLabels({"流水号", "交易时间", "类型", "付款账号", "收款账号", "金额 (元)", "余额快照 (元)", "备注"});
    adminTxTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    adminTxTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    adminTxTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    adminTxTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    adminTxTable->setAlternatingRowColors(true);
    tab3Layout->addWidget(adminTxTable);
    tabWidget->addTab(tab3, "全行流水监控");

    // --- Tab 4: 审计日志 ---
    auto *tab4 = new QWidget();
    auto *tab4Layout = new QVBoxLayout(tab4);
    adminLogViewer = new QPlainTextEdit(tab4);
    adminLogViewer->setReadOnly(true);
    adminLogViewer->setStyleSheet("font-family: Consolas, monospace; font-size: 12px;");
    tab4Layout->addWidget(adminLogViewer);

    auto *logBtnLayout = new QHBoxLayout();
    auto *btnRefreshLogs = new QPushButton("刷新日志", tab4);
    auto *btnClearLogs = new QPushButton("清空日志", tab4);
    connect(btnRefreshLogs, &QPushButton::clicked, this, &MainWindow::handleAdminRefreshLogs);
    connect(btnClearLogs, &QPushButton::clicked, this, &MainWindow::handleAdminClearLogs);
    logBtnLayout->addWidget(btnRefreshLogs);
    logBtnLayout->addWidget(btnClearLogs);
    logBtnLayout->addStretch();
    tab4Layout->addLayout(logBtnLayout);
    tabWidget->addTab(tab4, "系统审计日志");

    // --- Tab 5: 业务统计看板 ---
    auto *tab5 = new QWidget();
    auto *tab5Layout = new QVBoxLayout(tab5);

    auto *statGroupBox = new QGroupBox("全行业务汇总", tab5);
    auto *grid = new QGridLayout(statGroupBox);
    grid->setHorizontalSpacing(30);
    grid->setVerticalSpacing(20);

    statTotalDepositLabel = new QLabel("全行储蓄总额: --", statGroupBox);
    statTotalDepositLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #38bdf8;");
    statTotalAccountsLabel = new QLabel("总账户数: --", statGroupBox);
    statNormalAccountsLabel = new QLabel("正常账户: --", statGroupBox);
    statNormalAccountsLabel->setStyleSheet("color: #20c997; font-weight: bold;");
    statFrozenAccountsLabel = new QLabel("冻结账户: --", statGroupBox);
    statFrozenAccountsLabel->setStyleSheet("color: #ff6b6b; font-weight: bold;");
    statTotalCustomersLabel = new QLabel("注册客户数: --", statGroupBox);
    statTotalTransactionsLabel = new QLabel("累计交易笔数: --", statGroupBox);

    grid->addWidget(statTotalDepositLabel, 0, 0);
    grid->addWidget(statTotalAccountsLabel, 0, 1);
    grid->addWidget(statNormalAccountsLabel, 1, 0);
    grid->addWidget(statFrozenAccountsLabel, 1, 1);
    grid->addWidget(statTotalCustomersLabel, 2, 0);
    grid->addWidget(statTotalTransactionsLabel, 2, 1);

    tab5Layout->addWidget(statGroupBox);
    tab5Layout->addStretch();
    tabWidget->addTab(tab5, "业务统计报表");

    mainLayout->addWidget(tabWidget);
    return widget;
}

void MainWindow::refreshCurrentView() {
    if (!currentUser) return;

    if (currentUser->role() == "Admin") {
        stackedWidget->setCurrentIndex(1);
        adminWelcomeLabel->setText(QString("管理员控制台 | 当前登录：%1 (ID: %2)")
                                       .arg(QString::fromStdString(currentUser->getName()))
                                       .arg(currentUser->getId()));
        refreshAdminData();
    } else {
        stackedWidget->setCurrentIndex(0);
        auto *cust = dynamic_cast<Customer *>(currentUser);
        QString phone = cust ? QString::fromStdString(cust->getPhoneNumber()) : "无";
        custWelcomeLabel->setText(QString("欢迎您，%1 客户 (用户ID: %2 | 手机: %3)")
                                      .arg(QString::fromStdString(currentUser->getName()))
                                      .arg(currentUser->getId())
                                      .arg(phone));
        refreshCustomerData();
    }
}

// ==================== 客户数据刷新 ====================
void MainWindow::refreshCustomerData() {
    auto *cust = dynamic_cast<Customer *>(currentUser);
    if (!cust) return;

    long long prevSelected = getSelectedCustomerAccountId();

    custAccountCombo->blockSignals(true);
    custAccountCombo->clear();

    const auto &accIds = cust->getAccountIds();
    int selectIndex = -1;
    for (int i = 0; i < static_cast<int>(accIds.size()); ++i) {
        long long aid = accIds[i];
        custAccountCombo->addItem(QString("账号 %1").arg(aid), QVariant::fromValue(aid));
        if (aid == prevSelected) {
            selectIndex = i;
        }
    }
    custAccountCombo->blockSignals(false);

    if (accIds.empty()) {
        custAccountIdLabel->setText("账号: 暂无账户");
        custBalanceLabel->setText("余额: 0.00 元");
        custStatusLabel->setText("状态: --");
        custOpenDateLabel->setText("开户日期: --");
        btnCustDeposit->setEnabled(false);
        btnCustWithdraw->setEnabled(false);
        btnCustTransfer->setEnabled(false);
        btnCustSettle->setEnabled(false);
        btnCustClose->setEnabled(false);
        custTxTable->setRowCount(0);
        return;
    }

    btnCustDeposit->setEnabled(true);
    btnCustWithdraw->setEnabled(true);
    btnCustTransfer->setEnabled(true);
    btnCustSettle->setEnabled(true);
    btnCustClose->setEnabled(true);

    if (selectIndex >= 0) {
        custAccountCombo->setCurrentIndex(selectIndex);
    } else {
        custAccountCombo->setCurrentIndex(0);
    }
    onCustomerAccountChanged(custAccountCombo->currentIndex());
}

long long MainWindow::getSelectedCustomerAccountId() const {
    if (custAccountCombo->currentIndex() < 0) return 0;
    return custAccountCombo->currentData().toLongLong();
}

void MainWindow::onCustomerAccountChanged(int index) {
    if (index < 0) return;
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) return;

    try {
        Account *acc = bank.findAccount(aid);
        custAccountIdLabel->setText(QString("账号: %1").arg(aid));
        custBalanceLabel->setText(QString("余额: %1 元").arg(QString::fromStdString(formatMoney(acc->getBalance()))));

        QString statusHtml;
        if (acc->getStatus() == AccountStatus::NORMAL) {
            statusHtml = "状态: <span style='color: #20c997; font-weight: bold;'>正常</span>";
            btnCustDeposit->setEnabled(true);
            btnCustWithdraw->setEnabled(true);
            btnCustTransfer->setEnabled(true);
        } else if (acc->getStatus() == AccountStatus::FROZEN) {
            statusHtml = "状态: <span style='color: #ff6b6b; font-weight: bold;'>已冻结</span>";
            btnCustDeposit->setEnabled(false);
            btnCustWithdraw->setEnabled(false);
            btnCustTransfer->setEnabled(false);
        } else {
            statusHtml = "状态: <span style='color: #adb5bd; font-weight: bold;'>已销户</span>";
            btnCustDeposit->setEnabled(false);
            btnCustWithdraw->setEnabled(false);
            btnCustTransfer->setEnabled(false);
        }
        custStatusLabel->setText(statusHtml);
        custOpenDateLabel->setText(QString("开户日期: %1").arg(QString::fromStdString(acc->getOpenDate().toString())));

        filterCustomerTransactions();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "错误", QString::fromUtf8(e.what()));
    }
}

void MainWindow::filterCustomerTransactions() {
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) {
        custTxTable->setRowCount(0);
        return;
    }

    auto txs = bank.queryTransactions(aid);
    QString filterType = custTxFilterCombo->currentText();

    custTxTable->setRowCount(0);
    for (const auto *t : txs) {
        QString typeStr = txTypeToChinese(t->getType());
        if (filterType != "全部" && typeStr != filterType) {
            continue;
        }

        int row = custTxTable->rowCount();
        custTxTable->insertRow(row);

        custTxTable->setItem(row, 0, new QTableWidgetItem(QString::number(t->getId())));
        custTxTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(t->getDatetime())));
        custTxTable->setItem(row, 2, new QTableWidgetItem(typeStr));

        long long otherAcc = (t->getFromAccountId() == aid) ? t->getToAccountId() : t->getFromAccountId();
        custTxTable->setItem(row, 3, new QTableWidgetItem(otherAcc == 0 ? "-" : QString::number(otherAcc)));

        // 金额显示
        QString amtText = QString::fromStdString(formatMoney(t->getAmount()));
        auto *amtItem = new QTableWidgetItem();
        if (t->getType() == TransactionType::DEP || t->getType() == TransactionType::INT ||
            (t->getType() == TransactionType::TRF && t->getToAccountId() == aid)) {
            amtItem->setText("+" + amtText);
            amtItem->setForeground(QBrush(QColor("#20c997"))); // 绿色进账
        } else {
            amtItem->setText("-" + amtText);
            amtItem->setForeground(QBrush(QColor("#ff6b6b"))); // 红色出账
        }
        custTxTable->setItem(row, 4, amtItem);

        custTxTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(formatMoney(t->getBalanceSnapshot()))));
        custTxTable->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(t->getRemark())));
    }
}

void MainWindow::handleCustomerDeposit() {
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) return;
    DepositWithdrawDialog dlg(bank, aid, DepositWithdrawDialog::Mode::Deposit, this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshCustomerData();
    }
}

void MainWindow::handleCustomerWithdraw() {
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) return;
    DepositWithdrawDialog dlg(bank, aid, DepositWithdrawDialog::Mode::Withdraw, this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshCustomerData();
    }
}

void MainWindow::handleCustomerTransfer() {
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) return;
    TransferDialog dlg(bank, aid, this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshCustomerData();
    }
}

void MainWindow::handleCustomerOpenAccount() {
    OpenAccountDialog dlg(bank, currentUser->getId(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refreshCustomerData();
        // 自动切换至新开立的账户
        long long newId = dlg.getCreatedAccountId();
        int idx = custAccountCombo->findData(QVariant::fromValue(newId));
        if (idx >= 0) {
            custAccountCombo->setCurrentIndex(idx);
        }
    }
}

void MainWindow::handleCustomerSettleInterest() {
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) return;

    try {
        long long interest = bank.settleInterest(aid);
        if (interest > 0) {
            Logger::log("账户 " + std::to_string(aid) + " 结息 " + formatMoney(interest) + " 元");
            QMessageBox::information(this, "结息成功",
                                     QString("结息成功！本次入账利息：%1 元").arg(QString::fromStdString(formatMoney(interest))));
        } else {
            QMessageBox::information(this, "提示", "当前无需计结息（同日不重复结息或未达到起息周期）。");
        }
        refreshCustomerData();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "结息失败", QString::fromUtf8(e.what()));
    }
}

void MainWindow::handleCustomerCloseAccount() {
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) return;

    auto reply = QMessageBox::question(this, "确认销户",
                                       QString("您确定要注销账户 %1 吗？\n注销前请确保账户余额已全部取出为 0.00 元。").arg(aid),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    try {
        bank.closeAccount(aid);
        Logger::log("客户 " + std::to_string(currentUser->getId()) + " 销户 " + std::to_string(aid));
        QMessageBox::information(this, "销户成功", "账户已成功销户！");
        refreshCustomerData();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "销户失败", QString::fromUtf8(e.what()));
    }
}

void MainWindow::handleCustomerExportCsv() {
    long long aid = getSelectedCustomerAccountId();
    if (aid == 0) return;

    QString defaultName = QString("对账单_账号%1_%2.csv").arg(aid).arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    QString path = QFileDialog::getSaveFileName(this, "导出对账单", defaultName, "CSV 文件 (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法创建目标文件！");
        return;
    }

    QTextStream out(&file);
    // 写入 UTF-8 BOM 方便 Excel 正确识别中文
    out.setGenerateByteOrderMark(true);
    out << "流水号,交易时间,交易类型,对方账号,变动金额(元),余额快照(元),备注\n";

    auto txs = bank.queryTransactions(aid);
    for (const auto *t : txs) {
        long long otherAcc = (t->getFromAccountId() == aid) ? t->getToAccountId() : t->getFromAccountId();
        QString otherStr = otherAcc == 0 ? "-" : QString::number(otherAcc);
        out << t->getId() << ","
            << QString::fromStdString(t->getDatetime()) << ","
            << txTypeToChinese(t->getType()) << ","
            << otherStr << ","
            << QString::fromStdString(formatMoney(t->getAmount())) << ","
            << QString::fromStdString(formatMoney(t->getBalanceSnapshot())) << ","
            << QString::fromStdString(t->getRemark()) << "\n";
    }

    file.close();
    Logger::log("账户 " + std::to_string(aid) + " 导出对账单到 " + path.toStdString());
    QMessageBox::information(this, "导出成功", "交易对账单已成功导出！");
}

// ==================== 管理员业务逻辑 ====================
void MainWindow::refreshAdminData() {
    filterAdminAccounts();

    // 刷新客户名册 (Tab 2)
    adminUserTable->setRowCount(0);
    for (const auto &[uid, user] : bank.getUsers()) {
        int row = adminUserTable->rowCount();
        adminUserTable->insertRow(row);
        adminUserTable->setItem(row, 0, new QTableWidgetItem(QString::number(uid)));
        adminUserTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(user->getName())));
        adminUserTable->setItem(row, 2, new QTableWidgetItem(user->role() == "Admin" ? "管理员" : "客户"));

        auto *cust = dynamic_cast<Customer *>(user.get());
        if (cust) {
            adminUserTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(cust->getPhoneNumber())));
            adminUserTable->setItem(row, 4, new QTableWidgetItem(QString::number(cust->getAccountIds().size())));

            long long totalAssets = 0;
            for (long long aid : cust->getAccountIds()) {
                try {
                    totalAssets += bank.findAccount(aid)->getBalance();
                } catch (...) {}
            }
            adminUserTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(formatMoney(totalAssets))));
        } else {
            adminUserTable->setItem(row, 3, new QTableWidgetItem("-"));
            adminUserTable->setItem(row, 4, new QTableWidgetItem("-"));
            adminUserTable->setItem(row, 5, new QTableWidgetItem("-"));
        }
    }

    filterAdminTransactions();
    handleAdminRefreshLogs();

    // 统计看板 (Tab 5)
    long long totalDep = 0;
    int normalCount = 0;
    int frozenCount = 0;
    for (const auto &[aid, acc] : bank.getAccounts()) {
        totalDep += acc->getBalance();
        if (acc->getStatus() == AccountStatus::NORMAL) normalCount++;
        else if (acc->getStatus() == AccountStatus::FROZEN) frozenCount++;
    }

    statTotalDepositLabel->setText(QString("全行储蓄总额: %1 元").arg(QString::fromStdString(formatMoney(totalDep))));
    statTotalAccountsLabel->setText(QString("总账户数: %1 个").arg(bank.getAccounts().size()));
    statNormalAccountsLabel->setText(QString("正常运行账户: %1 个").arg(normalCount));
    statFrozenAccountsLabel->setText(QString("冻结监管账户: %1 个").arg(frozenCount));

    int custCount = 0;
    for (const auto &[uid, user] : bank.getUsers()) {
        if (user->role() == "Customer") custCount++;
    }
    statTotalCustomersLabel->setText(QString("注册客户数: %1 位").arg(custCount));
    statTotalTransactionsLabel->setText(QString("累计交易流水: %1 笔").arg(bank.getTransactions().size()));
}

long long MainWindow::getSelectedAdminAccountId() const {
    int row = adminAccTable->currentRow();
    if (row < 0) return 0;
    QTableWidgetItem *item = adminAccTable->item(row, 0);
    return item ? item->text().toLongLong() : 0;
}

void MainWindow::filterAdminAccounts() {
    QString kw = adminAccSearchEdit->text().trimmed();
    adminAccTable->setRowCount(0);

    for (const auto &[aid, acc] : bank.getAccounts()) {
        QString aidStr = QString::number(aid);
        QString ownerStr = QString::number(acc->getOwnerId());
        if (!kw.isEmpty() && !aidStr.contains(kw) && !ownerStr.contains(kw)) {
            continue;
        }

        int row = adminAccTable->rowCount();
        adminAccTable->insertRow(row);
        adminAccTable->setItem(row, 0, new QTableWidgetItem(aidStr));
        adminAccTable->setItem(row, 1, new QTableWidgetItem(ownerStr));
        adminAccTable->setItem(row, 2, new QTableWidgetItem("活期"));
        adminAccTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(formatMoney(acc->getBalance()))));

        auto *statusItem = new QTableWidgetItem(QString::fromStdString(acc->getStatusString()));
        if (acc->getStatus() == AccountStatus::NORMAL) {
            statusItem->setForeground(QBrush(QColor("#20c997")));
        } else if (acc->getStatus() == AccountStatus::FROZEN) {
            statusItem->setForeground(QBrush(QColor("#ff6b6b")));
        }
        adminAccTable->setItem(row, 4, statusItem);

        adminAccTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(acc->getOpenDate().toString())));
    }
}

void MainWindow::handleAdminFreeze() {
    long long aid = getSelectedAdminAccountId();
    if (aid == 0) {
        QMessageBox::information(this, "提示", "请先在表格中选中一个账户！");
        return;
    }

    try {
        bank.freezeAccount(aid);
        Logger::log("管理员 " + std::to_string(currentUser->getId()) + " 冻结账户 " + std::to_string(aid));
        QMessageBox::information(this, "成功", QString("账户 %1 已被成功冻结！").arg(aid));
        refreshAdminData();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "冻结失败", QString::fromUtf8(e.what()));
    }
}

void MainWindow::handleAdminUnfreeze() {
    long long aid = getSelectedAdminAccountId();
    if (aid == 0) {
        QMessageBox::information(this, "提示", "请先在表格中选中一个账户！");
        return;
    }

    try {
        bank.unfreezeAccount(aid);
        Logger::log("管理员 " + std::to_string(currentUser->getId()) + " 解冻账户 " + std::to_string(aid));
        QMessageBox::information(this, "成功", QString("账户 %1 已恢复正常状态！").arg(aid));
        refreshAdminData();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "解冻失败", QString::fromUtf8(e.what()));
    }
}

void MainWindow::handleAdminCloseAccount() {
    long long aid = getSelectedAdminAccountId();
    if (aid == 0) {
        QMessageBox::information(this, "提示", "请先在表格中选中一个账户！");
        return;
    }

    auto reply = QMessageBox::question(this, "确认销户",
                                       QString("管理员确定要销毁账户 %1 吗？\n销户要求该账户当前余额必须为 0.00 元。").arg(aid),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    try {
        bank.closeAccount(aid);
        Logger::log("管理员 " + std::to_string(currentUser->getId()) + " 销户 " + std::to_string(aid));
        QMessageBox::information(this, "成功", QString("账户 %1 已成功销户！").arg(aid));
        refreshAdminData();
    } catch (const BankException &e) {
        QMessageBox::warning(this, "销户失败", QString::fromUtf8(e.what()));
    }
}

void MainWindow::handleAdminSettleAllInterest() {
    long long totalInterest = 0;
    int count = 0;

    for (const auto &[aid, acc] : bank.getAccounts()) {
        try {
            long long curInterest = bank.settleInterest(aid);
            if (curInterest > 0) {
                totalInterest += curInterest;
                count++;
            }
        } catch (...) {}
    }

    Logger::log("管理员执行一键全行日终结息：共结息 " + std::to_string(count) + " 个账户，入账利息 " +
                formatMoney(totalInterest) + " 元");
    QMessageBox::information(this, "结息完成",
                             QString("全行日终结息处理完成！\n本次产生利息账户数: %1\n累计结息总金额: %2 元")
                                 .arg(count)
                                 .arg(QString::fromStdString(formatMoney(totalInterest))));
    refreshAdminData();
}

void MainWindow::filterAdminTransactions() {
    QString aidStr = adminTxSearchEdit->text().trimmed();
    long long targetAid = aidStr.isEmpty() ? 0 : aidStr.toLongLong();
    QString typeFilter = adminTxTypeFilterCombo->currentText();

    auto txs = bank.queryTransactions(targetAid);
    adminTxTable->setRowCount(0);

    for (const auto *t : txs) {
        QString typeStr = txTypeToChinese(t->getType());
        if (typeFilter != "全部" && typeStr != typeFilter) {
            continue;
        }

        int row = adminTxTable->rowCount();
        adminTxTable->insertRow(row);
        adminTxTable->setItem(row, 0, new QTableWidgetItem(QString::number(t->getId())));
        adminTxTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(t->getDatetime())));
        adminTxTable->setItem(row, 2, new QTableWidgetItem(typeStr));
        adminTxTable->setItem(row, 3, new QTableWidgetItem(t->getFromAccountId() == 0 ? "-" : QString::number(t->getFromAccountId())));
        adminTxTable->setItem(row, 4, new QTableWidgetItem(t->getToAccountId() == 0 ? "-" : QString::number(t->getToAccountId())));
        adminTxTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(formatMoney(t->getAmount()))));
        adminTxTable->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(formatMoney(t->getBalanceSnapshot()))));
        adminTxTable->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(t->getRemark())));
    }
}

void MainWindow::handleAdminRefreshLogs() {
    auto logs = Logger::getAllLogs();
    adminLogViewer->clear();
    for (const auto &log : logs) {
        adminLogViewer->appendPlainText(QString::fromStdString(log));
    }
}

void MainWindow::handleAdminClearLogs() {
    auto reply = QMessageBox::question(this, "确认清空", "确定要清空全部系统审计日志吗？此操作不可逆！",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        Logger::clearAllLogs();
        handleAdminRefreshLogs();
        statusBar()->showMessage("审计日志已清空", 3000);
    }
}

// ==================== 菜单通用操作 ====================
void MainWindow::handleSaveData() {
    storage.save(bank);
    Logger::log("操作员手动保存全行数据至本地存储");
    statusBar()->showMessage("数据已成功保存至本地文件", 3000);
    QMessageBox::information(this, "保存成功", "银行全部数据已成功保存至本地文件！");
}

void MainWindow::handleSwitchUser() {
    LoginDialog loginDlg(bank, this);
    if (loginDlg.exec() == QDialog::Accepted) {
        currentUser = loginDlg.getLoggedInUser();
        refreshCurrentView();
        statusBar()->showMessage("已切换为用户：" + QString::fromStdString(currentUser->getName()), 3000);
    }
}

void MainWindow::handleAbout() {
    QMessageBox::about(this, "关于银行管理系统",
                       "<h3>银行综合业务管理系统</h3>"
                       "<p>基于 <b>C++17 + Qt 6</b> 开发的分层架构银行管理系统。</p>"
                       "<p><b>架构特色：</b></p>"
                       "<ul>"
                       "<li>领域核心层（Core）与界面层（GUI）严格解耦；</li>"
                       "<li>全系统金额采用 64 位整数（分）存储，彻底避免浮点数精度误差；</li>"
                       "<li>统一异常处理机制与规范化操作审计日志。</li>"
                       "</ul>");
}

void MainWindow::closeEvent(QCloseEvent *event) {
    storage.save(bank);
    Logger::log("系统正常关闭并持久化保存数据");
    event->accept();
}
