#include "presetmanager.h"

PresetManager::PresetManager(QObject *parent)
    : QObject{parent}
    , settings("QtEmerge", "Presets")
{}


bool PresetManager::savePreset(const QString& name, const QString& packages)
{
    if (name.isEmpty() || packages.isEmpty())
        return false;

    settings.beginGroup(PRESET_GROUP);
    settings.setValue(name, packages);
    settings.endGroup();

    emit presetAdded(name);
    return true;
}

QString PresetManager::loadPreset(const QString& name)
{
    settings.beginGroup(PRESET_GROUP);
    QString packages = settings.value(name).toString();
    settings.endGroup();
    return packages;
}

bool PresetManager::deletePreset(const QString& name)
{
    settings.beginGroup(PRESET_GROUP);
    if (!settings.contains(name)) {
        settings.endGroup();
        return false;
    }

    settings.remove(name);
    settings.endGroup();

    emit presetDeleted(name);
    return true;
}

QStringList PresetManager::getAllPresetNames()
{
    settings.beginGroup(PRESET_GROUP);
    QStringList names = settings.childKeys();
    settings.endGroup();
    return names;
}
