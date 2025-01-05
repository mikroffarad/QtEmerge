#ifndef EMERGEMANAGER_H
#define EMERGEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QList>
#include "package.h"

class EmergeManager : public QObject
{
    Q_OBJECT
public:
    explicit EmergeManager(QObject *parent = nullptr);
    ~EmergeManager();

    void listInstalledPackages();
    void installPackages(const QString& packages);
    void removePackage(const QString& packageName);
    void cancelCurrentOperation();

signals:
    void packageListUpdated(const QList<Package>& packages);
    void operationCompleted(bool success, const QString& message);
    void operationProgress(const QString& status);

private:
    QProcess* process;
    QList<Package> parseQlistOutput(const QString& output);
    QString getPolkitPath();
    QString lastRemovedPackage;
    enum class Operation {
        None,
        List,
        Remove,
        Install
    };
    Operation currentOperation = Operation::None;

private slots:
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessOutput();
};

#endif // EMERGEMANAGER_H
