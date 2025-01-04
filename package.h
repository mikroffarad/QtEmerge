#ifndef PACKAGE_H
#define PACKAGE_H

#include <QString>

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
        return category + "/" + name;
    }
};

#endif // PACKAGE_H
