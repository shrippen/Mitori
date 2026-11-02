#pragma once

#include <QSettings>
#include <QString>
#include <QUuid>

// This device's id for the logs (made once, kept in the settings): every rating, note and course
// entry carries it, so that devices can be merged later (sync).
inline QString deviceId()
{
    QSettings settings;
    QString id = settings.value(QStringLiteral("device/id")).toString();
    if (id.isEmpty()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        settings.setValue(QStringLiteral("device/id"), id);
    }
    return id;
}
