#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QStringList>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void filterPackages(const QString &text);
    void handleProcessError(QProcess::ProcessError error);
    void processOutput();

private:
    Ui::MainWindow *ui;
    QProcess *emergeProcess;
    QStringList allPackages;
    void loadPackageList();
    void setUIEnabled(bool enabled);
    bool isCalculating;
};

#endif // MAINWINDOW_H
