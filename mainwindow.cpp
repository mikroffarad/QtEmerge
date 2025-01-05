#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , emergeManager(new EmergeManager(this))
    , presetManager(new PresetManager(this))
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

    connect(ui->b_savePreset, &QPushButton::clicked, this, &MainWindow::handlePresetSave);
    connect(ui->b_loadPreset, &QPushButton::clicked, this, &MainWindow::handlePresetLoad);
    connect(ui->b_deletePreset, &QPushButton::clicked, this, &MainWindow::handlePresetDelete);

    connect(presetManager, &PresetManager::presetAdded, this, &MainWindow::handlePresetAdded);
    connect(presetManager, &PresetManager::presetDeleted, this, &MainWindow::handlePresetDeleted);
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
    ui->pte_packages->clear();
    ui->le_presetName->clear();
    ui->stackedWidget->setCurrentIndex(1);
    loadPresets();
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

void MainWindow::handlePresetSave()
{
    QString presetName = ui->le_presetName->text().trimmed();
    QString packages = ui->pte_packages->toPlainText().trimmed();

    if (presetName.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter a preset name");
        return;
    }

    if (packages.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter package names to save");
        return;
    }

    if (presetManager->savePreset(presetName, packages)) {
        ui->le_presetName->clear();
        ui->statusbar->showMessage("Preset saved successfully", 3000);
    } else {
        QMessageBox::critical(this, "Error", "Failed to save preset");
    }
}

void MainWindow::handlePresetLoad()
{
    QListWidgetItem* currentItem = ui->lw_presets->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "Warning", "Please select a preset to load");
        return;
    }

    QString presetName = currentItem->text();
    QString packages = presetManager->loadPreset(presetName);

    if (!packages.isEmpty()) {
        ui->pte_packages->setPlainText(packages);
        ui->statusbar->showMessage("Preset loaded", 3000);
    } else {
        QMessageBox::critical(this, "Error", "Failed to load preset");
    }
}

void MainWindow::handlePresetDelete()
{
    QListWidgetItem* currentItem = ui->lw_presets->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, "Warning", "Please select a preset to delete");
        return;
    }

    QString presetName = currentItem->text();

    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Confirm Deletion",
                                                              "Are you sure you want to delete preset '" + presetName + "'?",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (presetManager->deletePreset(presetName)) {
            ui->statusbar->showMessage("Preset deleted", 3000);
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete preset");
        }
    }
}

void MainWindow::handlePresetAdded(const QString &name)
{
    ui->lw_presets->addItem(name);
}

void MainWindow::handlePresetDeleted(const QString &name)
{
    QList<QListWidgetItem*> items = ui->lw_presets->findItems(name, Qt::MatchExactly);
    for (QListWidgetItem* item : items) {
        delete ui->lw_presets->takeItem(ui->lw_presets->row(item));
    }
}

void MainWindow::setUIEnabled(bool enabled)
{
    ui->pte_search->setEnabled(enabled);
    ui->lw_packages->setEnabled(enabled);
    ui->b_remove->setEnabled(enabled && ui->lw_packages->currentRow() >= 0);
    ui->b_refresh->setEnabled(enabled);
}

void MainWindow::loadPresets()
{
    ui->lw_presets->clear();
    ui->lw_presets->addItems(presetManager->getAllPresetNames());
}
