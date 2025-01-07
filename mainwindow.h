#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QMessageBox>
#include <QStandardPaths>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // UI Controls
    void goToSearchUninstallPage();
    void goToInstallPage();
    void goToUpdatePage();
    void goToMainMenu();

    // searchInstall Page
    void refreshInstalledPackages();
    void removePackage();
    void filterPackages();


private:
    Ui::MainWindow *ui;
    QProcess *process;
    QString currentStatus;
    QStringList allPackages;
    void executeCommand(const QString &cmd);
};

#endif // MAINWINDOW_H
