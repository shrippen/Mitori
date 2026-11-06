#include "mediastore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QStandardPaths>
#include <QUuid>

namespace {
const QString kPhoto = QStringLiteral(".jpg");
const QString kVoice = QStringLiteral(".m4a");

QString newName(const QString &suffix)
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces) + suffix;
}
}

MediaStore::MediaStore(const QString &root)
    : m_root(root)
{
}

MediaStore &MediaStore::shared()
{
    static MediaStore store(defaultRoot());
    return store;
}

QString MediaStore::defaultRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/medien");
}

MediaStore::Kind MediaStore::kindOf(const QString &name)
{
    if (name.endsWith(kPhoto)) {
        return Kind::Photo;
    }
    if (name.endsWith(kVoice)) {
        return Kind::Voice;
    }
    return Kind::None;
}

// Only plain names of this folder (a name from a synced or imported note never leaves it).
QString MediaStore::path(const QString &name) const
{
    if (name.isEmpty() || name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')) || name.startsWith(QLatin1Char('.'))) {
        return {};
    }
    return QDir(m_root).filePath(name);
}

QUrl MediaStore::url(const QString &name) const
{
    const QString p = path(name);
    return p.isEmpty() ? QUrl() : QUrl::fromLocalFile(p);
}

// The camera's pictures are large (12 MP and more): the long side to kMaxSide, turned upright
// by its EXIF orientation.
QString MediaStore::importPhoto(const QUrl &source)
{
    QFile file(source.isLocalFile() ? source.toLocalFile() : source.toString());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QImageReader reader(&file);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.isValid() && std::max(size.width(), size.height()) > kMaxSide) {
        reader.setScaledSize(size.scaled(kMaxSide, kMaxSide, Qt::KeepAspectRatio));
    }
    QImage image = reader.read();
    if (image.isNull()) {
        return {};
    }
    if (std::max(image.width(), image.height()) > kMaxSide) {      // the reader knew no size
        image = image.scaled(kMaxSide, kMaxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QDir().mkpath(m_root);
    const QString name = newName(kPhoto);
    return image.save(path(name), "JPG", kQuality) ? name : QString();
}

QString MediaStore::newVoiceName()
{
    QDir().mkpath(m_root);
    return newName(kVoice);
}

bool MediaStore::remove(const QString &name)
{
    const QString p = path(name);
    return !p.isEmpty() && QFile::remove(p);
}

QStringList MediaStore::names() const
{
    QStringList list;
    for (const QString &name : QDir(m_root).entryList(QDir::Files, QDir::Name)) {
        if (kindOf(name) != Kind::None) {
            list << name;
        }
    }
    return list;
}

bool MediaStore::has(const QString &name) const
{
    const QString p = path(name);
    return !p.isEmpty() && QFile::exists(p);
}

bool MediaStore::adopt(const QString &name, const QByteArray &data)
{
    const QString p = path(name);
    if (p.isEmpty() || kindOf(name) == Kind::None) {
        return false;
    }
    if (QFile::exists(p)) {
        return true;
    }
    QDir().mkpath(m_root);
    QFile file(p);
    const bool ok = file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.flush();
    file.close();
    if (!ok || file.error() != QFileDevice::NoError) {
        file.remove();
        return false;
    }
    return true;
}

qint64 MediaStore::size() const
{
    qint64 total = 0;
    const auto files = QDir(m_root).entryInfoList(QDir::Files);
    for (const QFileInfo &file : files) {
        total += file.size();
    }
    return total;
}
