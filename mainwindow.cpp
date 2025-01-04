#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , emergeProcess(new QProcess(this))
    , removeProcess(new QProcess(this))
    , isCalculating(false)
{
    ui->setupUi(this);

    connect(ui->b_searchUninstall, &QPushButton::clicked, this, &MainWindow::switchToSearchUninstallPage);
    connect(emergeProcess, &QProcess::finished, this, &MainWindow::processInstalledPackages);
    connect(ui->pte_search, &QPlainTextEdit::textChanged,
            this, [this]() { filterPackages(ui->pte_search->toPlainText()); });
    connect(ui->lw_packages, &QListWidget::currentRowChanged, this, &MainWindow::onPackageSelected);
    connect(ui->b_remove, &QPushButton::clicked, this, &MainWindow::removeSelectedPackage);
    connect(removeProcess, &QProcess::finished, this, &MainWindow::handleRemoveProcessFinished);
    connect(ui->b_refresh, &QPushButton::clicked, this, &MainWindow::loadPackageList);
    connect(ui->b_back, &QPushButton::clicked, this, &MainWindow::switchToMainMenu);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadPackageList()
{
    ui->pte_search->clear();
    emergeProcess->start("emerge", QStringList() << "world" << "-ep");
    ui->statusbar->showMessage("Loading package list...");
    setUIEnabled(false);
}

void MainWindow::processInstalledPackages()
{
    QString output = QString::fromUtf8(emergeProcess->readAllStandardOutput());
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        if (line.contains("Calculating dependencies")) {
            isCalculating = true;
            ui->statusbar->showMessage("Calculating dependencies...");
            setUIEnabled(false);
            continue;
        }

        if (line.contains("Dependency resolution took")) {
            isCalculating = false;
            ui->statusbar->showMessage(line);
            setUIEnabled(true);
            continue;
        }

        if (line.startsWith("[")) {
            QRegularExpression rx("\\[(binary|ebuild)\\s+[R\\s]+\\]\\s+([^\\s]+)");
            QRegularExpressionMatch match = rx.match(line);

            if (match.hasMatch()) {
                QString packageName = match.captured(2);
                if (!packageName.isEmpty() && !allPackages.contains(packageName)) {
                    allPackages.append(packageName);
                    if (!isCalculating) {
                        ui->lw_packages->addItem(packageName);
                    }
                }
            }
        }
    }
}

void MainWindow::onPackageSelected(int row)
{
    if (row >= 0) {
        selectedPackage = ui->lw_packages->item(row)->text();
        ui->b_remove->setEnabled(true);
    } else {
        selectedPackage.clear();
        ui->b_remove->setEnabled(false);
    }
}

void MainWindow::removeSelectedPackage()
{
    QString basePackageName = getBasePackageName(selectedPackage);

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Package Removal",
                                  "Are you sure you want to remove " + basePackageName + "?",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {

        QString pkexecPath = getPolkitPath();
        if (pkexecPath.isEmpty()) {
            QMessageBox::critical(this, "Error", "pkexec not found. Please install polkit.");
            return;
        }

        ui->statusbar->showMessage("Removing package: " + basePackageName);
        setUIEnabled(false);

        QStringList arguments;
        arguments << "emerge" << "-C" << basePackageName;
        removeProcess->start(pkexecPath, arguments);
    }
}

void MainWindow::handleRemoveProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        ui->statusbar->showMessage("Package removed successfully. Refreshing package list...");

        allPackages.clear();
        ui->lw_packages->clear();
        selectedPackage.clear();
        ui->b_remove->setEnabled(false);

        loadPackageList();
    } else {
        setUIEnabled(true);
        QString errorOutput = QString::fromUtf8(removeProcess->readAllStandardError());
        ui->statusbar->showMessage("Error removing package");
        QMessageBox::critical(this, "Error",
                              "Failed to remove package: " + selectedPackage +
                                  "\nExit code: " + QString::number(exitCode) +
                                  "\nError: " + errorOutput);
    }
}

void MainWindow::switchToSearchUninstallPage()
{
    ui->statusbar->show();
    ui->stackedWidget->setCurrentIndex(1);

    if (ui->lw_packages->count() == 0) {
        ui->b_remove->setEnabled(false);
        loadPackageList();
    }
}

void MainWindow::switchToMainMenu()
{
    if (emergeProcess->state() != QProcess::NotRunning) {
        emergeProcess->kill();
        emergeProcess->waitForFinished();
    }
    ui->statusbar->hide();

    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::setUIEnabled(bool enabled)
{
    ui->pte_search->setEnabled(enabled);
    ui->lw_packages->setEnabled(enabled);
}

QString MainWindow::getBasePackageName(const QString &fullPackageName)
{
    QRegularExpression versionRegex("-\\d");
    QRegularExpressionMatch match = versionRegex.match(fullPackageName);
    if (match.hasMatch()) {
        return fullPackageName.left(match.capturedStart());
    }
    return fullPackageName;
}

QString MainWindow::getPolkitPath()
{
    QString pkexecPath = QStandardPaths::findExecutable("pkexec");
    if (pkexecPath.isEmpty()) {
        qDebug() << "pkexec not found in PATH";
        return QString();
    }
    return pkexecPath;
}

void MainWindow::filterPackages(const QString &text)
{
    ui->lw_packages->clear();
    if (text.isEmpty()) {
        ui->lw_packages->addItems(allPackages);
        return;
    }

    QStringList filtered = allPackages.filter(text, Qt::CaseInsensitive);
    ui->lw_packages->addItems(filtered);
}
