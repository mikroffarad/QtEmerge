#ifndef PACKAGE_H
#define PACKAGE_H

#include <QString>
#include <QRegularExpression>

struct Package {
    QString name;
    QString version;
    QString category;
    bool isInstalled;

    QString getFullName() const {
        if (!version.isEmpty()) {
            return category + "/" + name + "-" + version;
        }
        return category + "/" + name;
    }

    QString getBasePackageName() const {
        QRegularExpression versionRegex("-\\d");
        QRegularExpressionMatch match = versionRegex.match(getFullName());
        if (match.hasMatch()) {
            return getFullName().left(match.capturedStart());
        }
        return getFullName();
    }
};

#endif // PACKAGE_H
