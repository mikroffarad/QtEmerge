#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , emergeManager(new EmergeManager(this))
{
    ui->setupUi(this);

    connect(ui->b_searchUninstall, &QPushButton::clicked, this, &MainWindow::switchToSearchUninstallPage);
    connect(ui->b_install, &QPushButton::clicked, this, &MainWindow::switchToInstallPage);

    connect(ui->pte_search, &QPlainTextEdit::textChanged, this, [this]() { filterPackages(ui->pte_search->toPlainText()); });
    connect(ui->lw_packages, &QListWidget::currentRowChanged, this, &MainWindow::handlePackageSelected);
    connect(ui->b_remove, &QPushButton::clicked, this, &MainWindow::handlePackageRemoval);
    connect(ui->b_refresh, &QPushButton::clicked, this, &MainWindow::refreshPackageList);
    connect(ui->b_back, &QPushButton::clicked, this, &MainWindow::switchToMainMenu);

    connect(ui->b_back1, &QPushButton::clicked, this, &MainWindow::switchToMainMenu);
    connect(ui->b_installPackages, &QPushButton::clicked, this, &MainWindow::handlePackageInstallation);

    connect(emergeManager, &EmergeManager::packageListUpdated, this, &MainWindow::handlePackageListUpdated);
    connect(emergeManager, &EmergeManager::operationCompleted, this, &MainWindow::handleOperationCompleted);
    connect(emergeManager, &EmergeManager::operationProgress,
            ui->statusbar, [this](const QString& message) {
                ui->statusbar->showMessage(message);
            });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::switchToSearchUninstallPage()
{
    ui->statusbar->show();
    ui->stackedWidget->setCurrentIndex(2);

    if (ui->lw_packages->count() == 0) {
        refreshPackageList();
    }
}

void MainWindow::switchToInstallPage()
{
    ui->statusbar->show();
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::switchToMainMenu()
{
    ui->statusbar->hide();
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::handlePackageSelected(int row)
{
    if (row >= 0 && row < ui->lw_packages->count()) {
        QString displayedText = ui->lw_packages->item(row)->text();
        for (const Package& pkg : allPackages) {
            if (pkg.getFullName() == displayedText) {
                selectedPackage = pkg;
                ui->b_remove->setEnabled(true);
                return;
            }
        }
    }
    selectedPackage = Package();
    ui->b_remove->setEnabled(false);
}

void MainWindow::handlePackageRemoval()
{
    if (selectedPackage.name.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Confirm Package Removal",
                                                              "Are you sure you want to remove " + selectedPackage.getFullName() + "?",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        setUIEnabled(false);
        emergeManager->removePackage(selectedPackage.getFullName());
    }
}

void MainWindow::handlePackageListUpdated(const QList<Package>& packages)
{
    allPackages = packages;
    ui->lw_packages->clear();

    for (const Package& pkg : packages) {
        ui->lw_packages->addItem(pkg.getFullName());
    }

    setUIEnabled(true);
}

void MainWindow::handlePackageInstallation()
{
    QString packages = ui->pte_packages->toPlainText().trimmed();

    if (packages.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter package names");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Confirm Package Installation",
                                                              "Are you sure you want to install these packages:\n" + packages,
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        setUIEnabled(false);
        emergeManager->installPackages(packages);
    }
}

void MainWindow::handleOperationCompleted(bool success, const QString& message)
{
    setUIEnabled(true);

    if (!success) {
        QMessageBox::critical(this, "Error", message);
    } else {
        if (message.contains("removed")) {
            refreshPackageList();
            ui->statusbar->showMessage("Package removed successfully", 3000);
        }
    }
}

void MainWindow::filterPackages(const QString &text)
{
    ui->lw_packages->clear();
    if (text.isEmpty()) {
        for (const Package& pkg : allPackages) {
            ui->lw_packages->addItem(pkg.getFullName());
        }
        return;
    }

    for (const Package& pkg : allPackages) {
        if (pkg.getFullName().contains(text, Qt::CaseInsensitive)) {
            ui->lw_packages->addItem(pkg.getFullName());
        }
    }
}

void MainWindow::refreshPackageList()
{
    ui->pte_search->clear();
    setUIEnabled(false);
    emergeManager->listInstalledPackages();
}

void MainWindow::setUIEnabled(bool enabled)
{
    ui->pte_search->setEnabled(enabled);
    ui->lw_packages->setEnabled(enabled);
    ui->b_remove->setEnabled(enabled && ui->lw_packages->currentRow() >= 0);
    ui->b_refresh->setEnabled(enabled);
}
