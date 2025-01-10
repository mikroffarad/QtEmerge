#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QMessageBox>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>

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
    void goToEditConfigPage();
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
    void updateAll();

private:
    Ui::MainWindow *ui;
    QProcess *process;
    QProcess *updateProcess;
    QStringList allPackages;
    QAbstractButton* pb_makeConf;
    QAbstractButton* pb_packageUse;
    QAbstractButton* pb_packageLicense;
    QAbstractButton* pb_packageAcceptKeywords;
    QString filePath;
    QString getPresetsFilePath();
    QJsonObject presetsData;
    void executeCommand(const QString &cmd, const bool &runAsRoot);
    void showFile(QString filePath);
    void saveFile(QString filePath);
    void loadPresetsFromFile();
    void savePresetsToFile();
};

#endif // MAINWINDOW_H
