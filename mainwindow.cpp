#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    process = new QProcess(this);
    updateProcess = new QProcess(this);
    orphanProcess = new QProcess(this);

    // UI Controls
    connect(ui->b_goToSearchUninstallPage, &QPushButton::clicked, this, &MainWindow::goToSearchUninstallPage);
    connect(ui->b_goToInstallPage, &QPushButton::clicked, this, &MainWindow::goToInstallPage);
    connect(ui->b_goToUpdatePage, &QPushButton::clicked, this, &MainWindow::goToUpdatePage);
    connect(ui->b_goToEditConfigPage, &QPushButton::clicked, this, &MainWindow::goToEditConfigPage);
    connect(ui->b_back, &QPushButton::clicked, this, &MainWindow::goToMainMenu);
    connect(ui->b_back1, &QPushButton::clicked, this, &MainWindow::goToMainMenu);
    connect(ui->b_back2, &QPushButton::clicked, this, &MainWindow::goToMainMenu);
    connect(ui->b_back3, &QPushButton::clicked, this, &MainWindow::goToMainMenu);

    // Search/Uninstall page
    connect(ui->b_refresh, &QPushButton::clicked, this, &MainWindow::refreshInstalledPackages);
    connect(ui->b_removeSelected, &QPushButton::clicked, this, &MainWindow::removePackage);
    connect(ui->pte_search, &QPlainTextEdit::textChanged, this, &MainWindow::refreshInstalledPackages);
    connect(ui->b_removeOrphaned, &QPushButton::clicked, this, &MainWindow::removeOrphanedPackages);

    // Install page
    connect(ui->b_installPackages, &QPushButton::clicked, this, &MainWindow::installPackages);
    connect(ui->b_savePreset, &QPushButton::clicked, this, &MainWindow::savePreset);
    connect(ui->b_loadPreset, &QPushButton::clicked, this, &MainWindow::loadPreset);
    connect(ui->b_deletePreset, &QPushButton::clicked, this, &MainWindow::removePreset);
    loadPresetsFromFile();

    // Update page
    connect(ui->b_updateGentooRepo, &QPushButton::clicked, this, &MainWindow::updateGentooRepo);
    connect(ui->b_checkForUpdates, &QPushButton::clicked, this, &MainWindow::checkForUpdates);
    connect(ui->b_updateAll, &QPushButton::clicked, this, &MainWindow::updateAll);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::goToSearchUninstallPage()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->statusbar->show();
    refreshInstalledPackages();
}

void MainWindow::goToInstallPage()
{
    ui->statusbar->show();
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::goToUpdatePage()
{
    ui->statusbar->show();
    ui->stackedWidget->setCurrentIndex(3);
}

void MainWindow::goToEditConfigPage()
{
    QMessageBox msgBox;
    msgBox.setText("Which config file would you like to edit?");
    pb_makeConf = msgBox.addButton("make.conf", QMessageBox::ActionRole);
    pb_packageUse = msgBox.addButton("package.use", QMessageBox::ActionRole);
    pb_packageLicense = msgBox.addButton("package.license", QMessageBox::ActionRole);
    pb_packageAcceptKeywords = msgBox.addButton("package.accept_keywords", QMessageBox::ActionRole);
    msgBox.exec();

    if (msgBox.clickedButton() == pb_makeConf) {
        showFile("/etc/portage/make.conf");
    }
    if (msgBox.clickedButton() == pb_packageUse) {
        showFile("/etc/portage/package.use");
    }
    if (msgBox.clickedButton() == pb_packageLicense) {
        showFile("/etc/portage/package.license");
    }
    if (msgBox.clickedButton() == pb_packageAcceptKeywords) {
        showFile("/etc/portage/package.accept_license");
    }

    ui->stackedWidget->setCurrentIndex(4);
    ui->statusbar->show();
}

void MainWindow::goToMainMenu()
{
    ui->statusbar->hide();
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
        executeCommand(QString("emerge -C %1").arg(package), true);
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

void MainWindow::removeOrphanedPackages()
{
    ui->statusbar->showMessage("Searching orphaned packages...");

    disconnect(orphanProcess, &QProcess::finished, nullptr, nullptr);

    connect(orphanProcess, &QProcess::finished, this, [this]() {
        QString output = orphanProcess->readAllStandardOutput();
        QStringList lines = output.split("\n");

        orphanedPackages.clear();
        for (const QString& line : lines) {
            if (line.contains("All selected packages:")) {
                orphanedPackages = line;
                break;
            }
        }

        if (orphanedPackages.isEmpty()) {
            QMessageBox::information(this, "No Orphaned Packages",
                                     "No orphaned packages found in your system.");
            refreshInstalledPackages();
            return;
        }

        orphanedPackages.replace("All selected packages:",
                                 "These are the packages that would be unmerged:");

        QMessageBox confirmBox;
        confirmBox.setText("Remove Orphaned Packages");
        confirmBox.setInformativeText(orphanedPackages);
        confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        confirmBox.setDefaultButton(QMessageBox::No);

        if (confirmBox.exec() == QMessageBox::Yes) {
            executeCommand("emerge --depclean", true);
        } else {
            refreshInstalledPackages();
        }

        ui->statusbar->clearMessage();
    });

    orphanProcess->start("emerge", QStringList() << "-p" << "--depclean");
}

void MainWindow::installPackages()
{
    QString packages = ui->pte_packages->toPlainText().trimmed();
    if (packages.isEmpty()) return;

    if (QMessageBox::question(this, "Confirm Installation",
                              QString("Are you sure you want to install these packages?\n%1").arg(packages))
        == QMessageBox::Yes) {
        executeCommand(QString("emerge %1").arg(packages), true);
    }
}

void MainWindow::savePreset()
{
    QString name = ui->le_presetName->text().trimmed();
    QString packages = ui->pte_packages->toPlainText().trimmed();

    if (name.isEmpty() || packages.isEmpty()) return;

    presetsData[name] = packages;

    savePresetsToFile();

    if (!ui->lw_presets->findItems(name, Qt::MatchExactly).isEmpty()) {
        ui->lw_presets->findItems(name, Qt::MatchExactly).first()->setText(name);
    } else {
        ui->lw_presets->addItem(name);
    }
}

void MainWindow::loadPreset()
{
    QListWidgetItem *item = ui->lw_presets->currentItem();
    if (!item) return;

    QString name = item->text();
    if (presetsData.contains(name)) {
        ui->pte_packages->setPlainText(presetsData[name].toString());
    }
}

void MainWindow::removePreset()
{
    QListWidgetItem *item = ui->lw_presets->currentItem();
    if (!item) return;

    QString name = item->text();
    presetsData.remove(name);
    savePresetsToFile();

    delete ui->lw_presets->takeItem(ui->lw_presets->row(item));
}

QString MainWindow::getPresetsFilePath() {
    QString configPath = QDir::homePath() + "/.config/QtEmerge";
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return configPath + "/presets.json";
}

void MainWindow::loadPresetsFromFile() {
    QFile file(getPresetsFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open presets file for reading";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qDebug() << "Invalid presets file format";
        return;
    }

    presetsData = doc.object();

    ui->lw_presets->clear();
    for (const QString& key : presetsData.keys()) {
        ui->lw_presets->addItem(key);
    }
}

void MainWindow::savePresetsToFile() {
    QFile file(getPresetsFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open presets file for writing";
        return;
    }

    QJsonDocument doc(presetsData);
    file.write(doc.toJson());
    file.close();
}

void MainWindow::updateGentooRepo()
{
    executeCommand("emaint --auto sync", true);
    connect(process, &QProcess::readyReadStandardOutput, ui->statusbar, [this]() {
        ui->statusbar->showMessage("Updating Gentoo repository...");
    });
    connect(process, &QProcess::finished, this, [this]() {
        ui->statusbar->showMessage("Gentoo repository updated successfully");
    });
}

void MainWindow::checkForUpdates()
{
    ui->lw_packagesToUpdate->clear();
    ui->statusbar->showMessage("Checking for updates...");

    connect(updateProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = updateProcess->readAllStandardOutput();
        QStringList lines = output.split("\n", Qt::SkipEmptyParts);

        for (const QString &line : lines) {
            if (line.contains("[") && line.contains("]")) {

                QString action;
                if (line.contains(" U ")) {
                    action = "[Update]";
                } else if (line.contains(" R ")) {
                    action = "[Reinstall]";
                } else if (line.contains(" S ")) {
                    action = "[Sync]";
                } else if (line.contains(" NS ")) {
                    action = "[New Slot]";
                } else if (line.contains(" N ")) {
                    action = "[New]";
                } else {
                    action = "[Other]";
                }

                QString packageInfo = line.section("]", 1).trimmed();
                QString formattedLine = QString("%1 %2").arg(action, packageInfo);

                ui->lw_packagesToUpdate->addItem(formattedLine);
            }

            if (line.contains("Total: ")) {
                ui->statusbar->showMessage(line);
            }
        }

        qDebug().noquote() << output;
    });

        updateProcess->start("sh", QStringList() << "-c" << "emerge --ask --verbose --update --deep --newuse --pretend @world");
}

void MainWindow::updateAll()
{
    executeCommand("emerge --verbose --update --deep --newuse @world", true);
    connect (process, &QProcess::finished, this, &MainWindow::checkForUpdates);
}

void MainWindow::showFile(QString filePath)
{
    QProcess process;

    process.start("cat", QStringList() << filePath);
    process.waitForFinished();

    ui->l_file->setText(QString("Now you're editing %1").arg(filePath));
    ui->pte_fileEdit->setPlainText(process.readAllStandardOutput());

    disconnect(ui->b_saveFile, &QPushButton::clicked, nullptr, nullptr);

    connect(ui->b_saveFile, &QPushButton::clicked, this, [this, filePath]() {
        saveFile(filePath);
    });

    disconnect(ui->b_refreshFile, &QPushButton::clicked, nullptr, nullptr);

    connect(ui->b_refreshFile, &QPushButton::clicked, this, [this, filePath]() {
        showFile(filePath);
    });
}

void MainWindow::saveFile(QString filePath)
{
    executeCommand(QString("echo '%1' > %2").arg(ui->pte_fileEdit->toPlainText(), filePath), true);
    ui->statusbar->showMessage("File saved successfully", 5000);
}

void MainWindow::executeCommand(const QString &cmd, const bool &runAsRoot)
{
    qDebug() << "Executing command:" << cmd;

    if (runAsRoot == true) {
        QString pkexecPath = QStandardPaths::findExecutable("pkexec");
        process->start(pkexecPath, QStringList() << "sh" << "-c" << cmd);
    } else {
        process->start("sh", QStringList() << "-c" << cmd);
    }

    connect(process, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = process->readAllStandardOutput();
        QStringList lines = output.split("\n", Qt::SkipEmptyParts);

        for (const QString& line : lines) {
            if (line.startsWith(">>>")) {
                if (line.contains("Unmerging in: ")) {
                    ui->statusbar->showMessage("Unmerging in: 5 4 3 2 1");
                } else {
                    ui->statusbar->showMessage(line.mid(4).trimmed());
                }
            }
        }
        qDebug().noquote() << output;
    });

    connect(process, &QProcess::readyReadStandardError, this, [this]() {
        QString error = process->readAllStandardError();
        qDebug().noquote() << error;
    });
}

