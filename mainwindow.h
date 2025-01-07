#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSettings>


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

    // searchInstall page
    void refreshInstalledPackages();
    void removePackage();
    void filterPackages();

    // Install page
    void installPackages();
    void savePreset();
    void loadPreset();
    void removePreset();

    // Updates page
    void updateGentooRepo();
    void checkForUpdates();
    void parseUpdateList(const QString &output);
    void updateAll();

private:
    Ui::MainWindow *ui;
    QProcess *process;
    QString currentStatus;
    QStringList allPackages;
    QSettings settings;
    void executeCommand(const QString &cmd, const bool &runAsRoot);
};

#endif // MAINWINDOW_H
