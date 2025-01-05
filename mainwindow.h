#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include "emergemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void switchToSearchUninstallPage();
    void switchToInstallPage();
    void switchToMainMenu();

    void handlePackageSelected(int row);
    void handlePackageRemoval();
    void handlePackageListUpdated(const QList<Package>& packages);

    void handlePackageInstallation();

    void handleOperationCompleted(bool success, const QString& message);
    void filterPackages(const QString &text);
    void refreshPackageList();

private:
    Ui::MainWindow *ui;
    EmergeManager *emergeManager;
    QList<Package> allPackages;
    Package selectedPackage;

    void setUIEnabled(bool enabled);
};

#endif // MAINWINDOW_H
