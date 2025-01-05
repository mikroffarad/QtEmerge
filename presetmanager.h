#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>

class PresetManager : public QObject
{
    Q_OBJECT
public:
    explicit PresetManager(QObject *parent = nullptr);

    bool savePreset(const QString& name, const QString& packages);
    QString loadPreset(const QString& name);
    bool deletePreset(const QString& name);
    QStringList getAllPresetNames();

signals:
    void presetAdded(const QString& name);
    void presetDeleted(const QString& name);

private:
    QSettings settings;
    const QString PRESET_GROUP = "PackagePresets";
};

#endif // PRESETMANAGER_H
