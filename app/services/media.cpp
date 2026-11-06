#include "media.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

Media::Media(QObject *parent)
    : Media(&MediaStore::shared(), parent)
{
}

Media::Media(MediaStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
}

QUrl Media::url(const QString &name) const
{
    return m_store->url(name);
}

Media::Kind Media::kind(const QString &name) const
{
    switch (MediaStore::kindOf(name)) {
    case MediaStore::Kind::Photo:
        return Kind::Photo;
    case MediaStore::Kind::Voice:
        return Kind::Voice;
    default:
        return Kind::None;
    }
}

bool Media::exists(const QString &name) const
{
    const QString p = m_store->path(name);
    return !p.isEmpty() && QFile::exists(p);
}

QString Media::importPhoto(const QUrl &source)
{
    return m_store->importPhoto(source);
}

QVariantMap Media::newVoice()
{
    const QString name = m_store->newVoiceName();
    return {{QStringLiteral("name"), name}, {QStringLiteral("url"), m_store->url(name)}};
}

bool Media::discard(const QString &name)
{
    return m_store->remove(name);
}

QString Media::cameraFile() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("kamera.jpg"));
}

QString Media::importCapture(const QString &path)
{
    const QString name = m_store->importPhoto(QUrl::fromLocalFile(path));
    QFile::remove(path);
    return name;
}

qint64 Media::size() const
{
    return m_store->size();
}
