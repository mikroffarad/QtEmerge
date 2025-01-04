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
    void removePackage(const QString& packageName);
    void cancelCurrentOperation();

signals:
    void packageListUpdated(const QList<Package>& packages);
    void operationCompleted(bool success, const QString& message);
    void operationProgress(const QString& status);

private:
    QProcess* process;
    QList<Package> parseEmergeOutput(const QString& output);
    QString getPolkitPath();

private slots:
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);
};

#endif // EMERGEMANAGER_H
