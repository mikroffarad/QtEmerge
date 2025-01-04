#include "emergemanager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QRegularExpression>

EmergeManager::EmergeManager(QObject *parent)
    : QObject{parent}
    , process(new QProcess(this))
{
    connect(process, &QProcess::finished, this, &EmergeManager::handleProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &EmergeManager::handleProcessError);
}

EmergeManager::~EmergeManager()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished();
    }
}

void EmergeManager::listInstalledPackages()
{
    if (process->state() != QProcess::NotRunning) {
        emit operationProgress("Previous operation is still running");
        return;
    }

    process->start("qlist", QStringList() << "-I");
    emit operationProgress("Loading package list...");
}

void EmergeManager::removePackage(const QString &packageName)
{
    if (process->state() != QProcess::NotRunning) {
        emit operationProgress("Previous operation is still running");
        return;
    }

    QString pkexecPath = getPolkitPath();
    if (pkexecPath.isEmpty()) {
        emit operationCompleted(false, "pkexec not found. Please install polkit.");
        return;
    }

    QStringList arguments;
    arguments << "emerge" << "-C" << packageName;

    lastRemovedPackage = packageName;

    process->start(pkexecPath, arguments);
    emit operationProgress("Removing package: " + packageName);
}

void EmergeManager::cancelCurrentOperation()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
    }
}

QList<Package> EmergeManager::parseQlistOutput(const QString &output)
{
    QList<Package> packages;
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.split("/");
        if (parts.size() == 2) {
            Package pkg;
            pkg.category = parts[0];
            pkg.name = parts[1];
            packages.append(pkg);
        }
    }

    emit operationProgress(QString("Found %1 installed packages").arg(packages.count()));
    return packages;
}

QString EmergeManager::getPolkitPath()
{
    QString pkexecPath = QStandardPaths::findExecutable("pkexec");
    if (pkexecPath.isEmpty()) {
        qDebug() << "pkexec not found in PATH";
    }
    return pkexecPath;
}

void EmergeManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        if (process->program().endsWith("qlist")) {
            QString output = QString::fromUtf8(process->readAllStandardOutput());
            QList<Package> packages = parseQlistOutput(output);
            emit packageListUpdated(packages);
            emit operationCompleted(true, "Operation completed successfully");
        } else if (process->program().endsWith("pkexec")) {
            emit operationCompleted(true, "Package removed successfully");
        }
    } else {
        QString errorOutput = QString::fromUtf8(process->readAllStandardError());
        emit operationCompleted(false, "Operation failed: " + errorOutput);
    }
}

void EmergeManager::handleProcessError(QProcess::ProcessError error)
{
    QString errorMessage = "Process error: ";
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage += "Failed to start";
        break;
    case QProcess::Crashed:
        errorMessage += "Process crashed";
        break;
    default:
        errorMessage += "Unknown error";
    }

    emit operationCompleted(false, errorMessage);
}

