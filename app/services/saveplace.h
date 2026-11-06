#pragma once

#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// Where a save dialog's answer goes (SaveFileDialog.qml): the path the user typed
// (platform/typedpath), and whether it would replace a file.
class SavePlace : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // The app asks before replacing a file, not the dialog: Qt's own dialog checks the name in the
    // current folder, not the typed path. In the Flatpak the portal's dialog asks (a path there
    // may exist from the moment it is picked); on Android the system's picker.
    Q_PROPERTY(bool asksItself READ asksItself CONSTANT)

public:
    using QObject::QObject;

    bool asksItself() const;

    Q_INVOKABLE QUrl resolve(const QUrl &picked, const QUrl &folder) const;
    // A file is there and asksItself: the user has to say yes first.
    Q_INVOKABLE bool taken(const QUrl &target) const;
};
