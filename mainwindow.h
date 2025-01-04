#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QStringList>
#include <QRegularExpression>
#include <QMessageBox>

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
    void loadPackageList();
    void setUIEnabled(bool enabled);
    void processInstalledPackages();
    void filterPackages(const QString &text);
    void onPackageSelected(int row);
    void removeSelectedPackage();
    void handleRemoveProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);;
    void switchToMainMenu();

private:
    Ui::MainWindow *ui;
    QProcess *emergeProcess;
    QProcess *removeProcess;
    QStringList allPackages;
    bool isCalculating;
    QString selectedPackage;
    QString getBasePackageName(const QString &fullPackageName);
    QString getPolkitPath();
};

#endif // MAINWINDOW_H
