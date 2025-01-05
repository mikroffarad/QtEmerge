#include "emergemanager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QTimer>

EmergeManager::EmergeManager(QObject *parent)
    : QObject{parent}
    , process(new QProcess(this))
{
    connect(process, &QProcess::finished, this, &EmergeManager::handleProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &EmergeManager::handleProcessError);
    connect(process, &QProcess::readyReadStandardOutput, this, &EmergeManager::handleProcessOutput);
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

    currentOperation = Operation::List;

    QProcess* qlistProcess = new QProcess(this);
    connect(qlistProcess, &QProcess::finished, this, [this, qlistProcess](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            QString output = QString::fromUtf8(qlistProcess->readAllStandardOutput());
            QList<Package> packages = parseQlistOutput(output);
            emit packageListUpdated(packages);
            emit operationCompleted(true, "Package list updated");
        } else {
            QString error = QString::fromUtf8(qlistProcess->readAllStandardError());
            emit operationCompleted(false, "Failed to get package list: " + error);
        }
        qlistProcess->deleteLater();
        currentOperation = Operation::None;
    });

    qlistProcess->start("qlist", QStringList() << "-I");
    emit operationProgress("Loading package list...");
}

void EmergeManager::installPackages(const QString& packages)
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

    QStringList packageList = packages.split(" ", Qt::SkipEmptyParts);
    if (packageList.isEmpty()) {
        emit operationCompleted(false, "No packages specified");
        return;
    }

    QStringList arguments;
    arguments << "emerge";
    arguments << "-q";
    arguments << packageList;

    currentOperation = Operation::Install;
    process->setProcessChannelMode(QProcess::MergedChannels);

    process->start(pkexecPath, arguments);
    emit operationProgress("Starting installation of: " + packages);
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

    currentOperation = Operation::Remove;
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

    qDebug() << "Raw qlist output:" << output;
    qDebug() << "Number of lines:" << lines.size();

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
        switch (currentOperation) {
        case Operation::List: {
            QString output = QString::fromUtf8(process->readAllStandardOutput());
            QList<Package> packages = parseQlistOutput(output);
            emit packageListUpdated(packages);
            break;
        }
        case Operation::Remove: {
            emit operationCompleted(true, "Package removed successfully");
            break;
        }
        case Operation::Install: {
            emit operationCompleted(true, "Package(s) installed successfully");
             QTimer::singleShot(1000, this, &EmergeManager::listInstalledPackages);
            break;
        }
        default:
            emit operationCompleted(true, "Operation completed successfully");
        }
    } else {
        QString errorOutput = QString::fromUtf8(process->readAllStandardError());
        emit operationCompleted(false, "Operation failed: " + errorOutput);
    }

    currentOperation = Operation::None;
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

void EmergeManager::handleProcessOutput()
{
    QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        if (line.startsWith(">>> ")) {
            QString statusMessage = line.mid(4).trimmed();
            emit operationProgress(statusMessage);
        }
    }
}

