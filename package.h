#ifndef PACKAGE_H
#define PACKAGE_H

#include <QString>

struct Package {
    QString category;
    QString name;

    QString getFullName() const {
        return category + "/" + name;
    }
};

#endif // PACKAGE_H
