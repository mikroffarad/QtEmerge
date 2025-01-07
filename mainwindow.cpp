#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    process = new QProcess(this);  // Додайте цей рядок

    // UI Controls
    connect(ui->b_goToSearchUninstallPage, &QPushButton::clicked, this, &MainWindow::goToSearchUninstallPage);
    connect(ui->b_goToInstallPage, &QPushButton::clicked, this, &MainWindow::goToInstallPage);
    connect(ui->b_goToUpdatePage, &QPushButton::clicked, this, &MainWindow::goToUpdatePage);
    connect(ui->b_back, &QPushButton::clicked, this, &MainWindow::goToMainMenu);
    connect(ui->b_back1, &QPushButton::clicked, this, &MainWindow::goToMainMenu);
    connect(ui->b_back2, &QPushButton::clicked, this, &MainWindow::goToMainMenu);

    // Search/Uninstall Page
    connect(ui->b_refresh, &QPushButton::clicked, this, &MainWindow::refreshInstalledPackages);
    connect(ui->b_remove, &QPushButton::clicked, this, &MainWindow::removePackage);
    connect(ui->pte_search, &QPlainTextEdit::textChanged, this, &MainWindow::refreshInstalledPackages);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::goToSearchUninstallPage()
{
    ui->stackedWidget->setCurrentIndex(1);
    refreshInstalledPackages();
}

void MainWindow::goToInstallPage()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::goToUpdatePage()
{
    ui->stackedWidget->setCurrentIndex(3);
}

void MainWindow::goToMainMenu()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::refreshInstalledPackages()
{
    QProcess process;
    process.start("qlist", QStringList() << "-I");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    allPackages = output.split("\n", Qt::SkipEmptyParts);

    filterPackages();

    ui->statusbar->showMessage(QString("Found %1 installed packages").arg(ui->lw_packages->count()));
}

void MainWindow::removePackage()
{
    QListWidgetItem *item = ui->lw_packages->currentItem();
    if (!item) return;

    QString package = item->text();
    if (QMessageBox::question(this, "Confirm Removal",
                              QString("Are you sure you want to remove %1?").arg(package))
        == QMessageBox::Yes) {
        connect(process, &QProcess::finished, this, &MainWindow::refreshInstalledPackages);
        executeCommand(QString("emerge -C %1").arg(package));
    }
}

void MainWindow::filterPackages()
{
    QString searchText = ui->pte_search->toPlainText().toLower().trimmed();

    ui->lw_packages->clear();

    if (searchText.isEmpty()) {
        ui->lw_packages->addItems(allPackages);
    } else {
        for (const QString &package : allPackages) {
            if (package.toLower().contains(searchText)) {
                ui->lw_packages->addItem(package);
            }
        }
    }
}

void MainWindow::executeCommand(const QString &cmd)
{
    qDebug() << "Executing command:" << cmd;
    QString pkexecPath = QStandardPaths::findExecutable("pkexec");

    process->start(pkexecPath, QStringList() << "sh" << "-c" << cmd);

    connect(process, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = process->readAllStandardOutput();
        QStringList lines = output.split("\n", Qt::SkipEmptyParts);

        for (const QString& line : lines) {
            if (line.startsWith(">>> ")) {
                QString statusMessage = line.mid(4).trimmed();
                if (statusMessage.startsWith("Unmerging in")) {
                    statusMessage = "Unmerging in: 5 4 3 2 1";
                }
                ui->statusbar->showMessage(statusMessage);
            }
        }

        qDebug().noquote() << output;
    });

    connect(process, &QProcess::readyReadStandardError, this, [this]() {
        QString error = process->readAllStandardError();
        qDebug().noquote() << error;
    });
}
