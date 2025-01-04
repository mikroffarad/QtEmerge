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

    process->start("emerge", QStringList() << "world" << "-ep");
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
    process->start(pkexecPath, arguments);
    emit operationProgress("Removing package: " + packageName);
}

void EmergeManager::cancelCurrentOperation()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
    }
}

QList<Package> EmergeManager::parseEmergeOutput(const QString &output)
{
    QList<Package> packages;
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    bool isCalculating = false;

    for (const QString& line : lines) {
        if (line.contains("Calculating dependencies")) {
            isCalculating = true;
            continue;
        }

        if (line.contains("Dependency resolution took")) {
            isCalculating = false;
            continue;
        }

        if (!isCalculating && line.startsWith("[")) {
            QRegularExpression rx("\\[(binary|ebuild)\\s+[R\\s]+[~]?\\s*\\]\\s+([^\\s]+)");
            QRegularExpressionMatch match = rx.match(line);

            if (match.hasMatch()) {
                QString fullPackageName = match.captured(2);

                QRegularExpression packageRx("([^/]+)/([^-]+(?:-[^-]+)*)-([^-]+(?:-r\\d+(?:-\\d+)?)?)$");
                QRegularExpressionMatch packageMatch = packageRx.match(fullPackageName);

                if (packageMatch.hasMatch()) {
                    Package pkg;
                    pkg.category = packageMatch.captured(1);
                    pkg.name = packageMatch.captured(2);
                    pkg.version = packageMatch.captured(3);
                    pkg.isInstalled = true;
                    packages.append(pkg);
                }
            }
        }
    }

    if (!packages.isEmpty()) {
        emit operationProgress(QString("Found %1 installed packages").arg(packages.count()));
    }

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
        if (process->program().endsWith("emerge")) {
            QString output = QString::fromUtf8(process->readAllStandardOutput());
            QList<Package> packages = parseEmergeOutput(output);
            emit packageListUpdated(packages);
            emit operationCompleted(true, "Operation completed successfully");
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

