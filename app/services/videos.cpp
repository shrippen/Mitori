#include "videos.h"

#include "bundleshelf.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {
constexpr auto kFolderKey = "videos/folder";
}

Videos::Videos(QObject *parent)
    : Videos(Catalog::bundled(), parent)
{
}

Videos::Videos(const Catalog &catalog, QObject *parent)
    : QObject(parent)
    , m_catalog(catalog)
    , m_folder(QSettings().value(QLatin1String(kFolderKey), defaultFolder()).toString())
{
    // Videos of a bundle came or went: bindings that ask available() follow (they read folder).
    connect(&BundleShelf::notifier(), &BundleShelfNotifier::changed, this, &Videos::folderChanged);
}

QString Videos::defaultFolder()
{
    const QString fromEnv = qEnvironmentVariable("MITORI_VIDEOS");
    if (!fromEnv.isEmpty()) {
        return fromEnv;
    }
    const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for (const QString &location : locations) {
        const QString candidate = QDir(location).filePath(QStringLiteral("videos"));
        if (QFileInfo(candidate).isDir()) {
            return candidate;
        }
    }
    // Android lists the external files folder last: there adb can put the videos.
    return locations.isEmpty() ? QString() : QDir(locations.last()).filePath(QStringLiteral("videos"));
}

void Videos::setFolder(const QString &folder)
{
    if (folder == m_folder) {
        return;
    }
    m_folder = folder;
    QSettings().setValue(QLatin1String(kFolderKey), folder);
    emit folderChanged();
}

// A loaded bundle first, else the folder of the settings.
QString Videos::path(const QString &video) const
{
    const QString bundled = BundleShelf::shared().path(video);
    if (!bundled.isEmpty()) {
        return bundled;
    }
    const QString file = m_catalog.videos().value(video).toMap().value(QStringLiteral("datei")).toString();
    return file.isEmpty() ? QString() : QDir(m_folder).filePath(file);
}

bool Videos::available(const QString &video) const
{
    const QString p = path(video);
    return !p.isEmpty() && QFileInfo::exists(p);
}

QUrl Videos::source(const QString &video) const
{
    return available(video) ? QUrl::fromLocalFile(path(video)) : QUrl();
}

// A bundle's file is cut already: its picture is the whole frame, at the same aspect ratio.
QVariantMap Videos::crop(const QString &video) const
{
    const QVariantMap crop = m_catalog.videos().value(video).toMap().value(QStringLiteral("beschnitt")).toMap();
    if (!BundleShelf::shared().cropped(video) || BundleShelf::shared().path(video).isEmpty()) {
        return crop;
    }
    return {{QStringLiteral("verhaeltnis"), crop.value(QStringLiteral("verhaeltnis"))}};
}
