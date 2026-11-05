#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "catalog.h"

// The exam videos on this device. For now from a local folder with the layout of
// "Mitori Videos" (Aikido/5. Kyu.mkv …); video bundles will fill the same folder later.
// The folder: the setting videos/folder, else MITORI_VIDEOS, else "videos" in the app's data
// locations (Android: also the app's external files folder, reachable with adb).
class Videos : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    // Inside a Flatpak sandbox, which reads only ~/Videos (packaging/flatpak).
    Q_PROPERTY(bool sandboxed READ sandboxed CONSTANT)

public:
    explicit Videos(QObject *parent = nullptr);
    explicit Videos(const Catalog &catalog, QObject *parent = nullptr);

    QString folder() const { return m_folder; }
    void setFolder(const QString &folder);
    bool sandboxed() const { return qEnvironmentVariableIsSet("FLATPAK_ID"); }

    // Whether the file of a video key ("aikido/k5") is on the device.
    Q_INVOKABLE bool available(const QString &video) const;
    Q_INVOKABLE QUrl source(const QString &video) const;
    // Picture area without the black bars: {hoehe, y, h, verhaeltnis} (see tools/kapitel.py).
    Q_INVOKABLE QVariantMap crop(const QString &video) const;

    Q_INVOKABLE static QString defaultFolder();

signals:
    void folderChanged();

private:
    QString path(const QString &video) const;

    const Catalog &m_catalog;
    QString m_folder;
};
