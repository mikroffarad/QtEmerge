#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , emergeProcess(new QProcess(this))
    , isCalculating(false)
{
    ui->setupUi(this);

    connect(ui->pte_search, &QPlainTextEdit::textChanged,
            this, [this]() { filterPackages(ui->pte_search->toPlainText()); });

    connect(emergeProcess, &QProcess::readyReadStandardOutput,
            this, &MainWindow::processOutput);
    connect(emergeProcess, &QProcess::errorOccurred,
            this, &MainWindow::handleProcessError);

    loadPackageList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadPackageList()
{
    emergeProcess->start("emerge", QStringList() << "world" << "-ep");
    ui->statusbar->showMessage("Loading package list...");
    setUIEnabled(false);
}

void MainWindow::processOutput()
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
            ui->statusbar->showMessage("Package list loaded");
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

void MainWindow::setUIEnabled(bool enabled)
{
    ui->pte_search->setEnabled(enabled);
    ui->lw_packages->setEnabled(enabled);
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

void MainWindow::handleProcessError(QProcess::ProcessError error)
{
    qDebug() << "Process error:" << error;
    ui->statusbar->showMessage("Error loading package list");
    setUIEnabled(true);
}
