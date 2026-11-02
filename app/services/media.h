#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "mediastore.h"

// Photos and voice notes for QML (store/mediastore): the page takes a photo (camera or a picked
// file) or records a voice note, gets a name, and gives it to Practice.addNote or
// Courses.addEntry. A note keeps the name; deleting the note deletes the file.
//
//   photo   importPhoto(url) ──► name  (made smaller, JPEG)
//   voice   newVoice() ──► {name, url}  the recorder writes to url; discard(name) if not kept
//   show    url(name), kind(name)
class Media : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum class Kind { None, Photo, Voice };
    Q_ENUM(Kind)

    explicit Media(QObject *parent = nullptr);
    // For tests: a folder of its own.
    Media(MediaStore *store, QObject *parent = nullptr);

    Q_INVOKABLE QUrl url(const QString &name) const;
    Q_INVOKABLE Kind kind(const QString &name) const;
    Q_INVOKABLE QString importPhoto(const QUrl &source);
    // Where the camera page saves its picture (the cache), and taking it in: imported, the
    // camera's file removed.
    Q_INVOKABLE QString cameraFile() const;
    Q_INVOKABLE QString importCapture(const QString &path);
    Q_INVOKABLE QVariantMap newVoice();
    Q_INVOKABLE bool discard(const QString &name);

private:
    MediaStore *m_store;
};
