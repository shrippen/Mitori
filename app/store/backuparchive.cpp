#include "backuparchive.h"

#include "mediastore.h"
#include "zipwriter.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const QString kManifest = QStringLiteral("manifest.json");
const QString kMediaDir = QStringLiteral("medien/");
const QString kApp = QStringLiteral("Mitori");

QString noBackup()
{
    return QCoreApplication::translate("BackupArchive", "this is no Mitori backup");
}

bool readFile(const QString &path, QByteArray *out, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = file.errorString();
        return false;
    }
    *out = file.readAll();
    return true;
}

} // namespace

const QString BackupArchive::kPractice = QStringLiteral("mitori.sqlite");
const QString BackupArchive::kCourses = QStringLiteral("lehrgaenge.sqlite");

bool BackupArchive::write(QIODevice *out, const QString &practice, const QString &courses, const MediaStore &media,
                          const Manifest &manifest, QString *error)
{
    ZipWriter zip(out);
    const QJsonObject json{{QStringLiteral("app"), kApp},
                           {QStringLiteral("format"), kFormat},
                           {QStringLiteral("version"), manifest.version},
                           {QStringLiteral("created"), manifest.created.toUTC().toString(Qt::ISODate)},
                           {QStringLiteral("device"), manifest.device}};
    bool ok = zip.add(kManifest, QJsonDocument(json).toJson());

    // The databases, then the media; a file that cannot be read fails the backup (it would be
    // incomplete without a word).
    for (const auto &[name, path] : {std::pair{kPractice, practice}, std::pair{kCourses, courses}}) {
        QByteArray data;
        if (!ok || !readFile(path, &data, error)) {
            return false;
        }
        ok = zip.add(name, data);
    }
    for (const QString &name : media.names()) {
        QByteArray data;
        if (!ok || !readFile(media.path(name), &data, error)) {
            return false;
        }
        ok = zip.add(kMediaDir + name, data);
    }
    ok = ok && zip.finish();
    if (!ok) {
        *error = out->errorString();
    }
    return ok;
}

bool BackupArchive::open(QIODevice *in)
{
    m_manifest = {};
    m_media.clear();
    m_error.clear();
    QByteArray raw;
    if (!m_zip.open(in) || !m_zip.read(kManifest, &raw)) {
        m_error = noBackup();
        return false;
    }
    const QJsonObject json = QJsonDocument::fromJson(raw).object();
    if (json.value(QStringLiteral("app")).toString() != kApp) {
        m_error = noBackup();
        return false;
    }
    m_manifest.format = json.value(QStringLiteral("format")).toInt();
    m_manifest.version = json.value(QStringLiteral("version")).toString();
    m_manifest.created = QDateTime::fromString(json.value(QStringLiteral("created")).toString(), Qt::ISODate);
    m_manifest.device = json.value(QStringLiteral("device")).toString();
    if (m_manifest.format > kFormat) {
        m_error = QCoreApplication::translate("BackupArchive", "it comes from a newer Mitori (%1); update Mitori")
                      .arg(m_manifest.version);
        return false;
    }

    const QStringList names = m_zip.names();
    if (!names.contains(kPractice) || !names.contains(kCourses)) {
        m_error = noBackup();
        return false;
    }
    for (const QString &name : names) {
        if (name.startsWith(kMediaDir) && MediaStore::kindOf(name) != MediaStore::Kind::None) {
            m_media << name.mid(kMediaDir.size());
        }
    }
    return true;
}

bool BackupArchive::extract(const QString &name, const QString &path)
{
    QByteArray data;
    if (!m_zip.read(name, &data)) {
        m_error = m_zip.errorString();
        return false;
    }
    QFile file(path);
    const bool ok = file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(data) == data.size()
                    && file.flush();
    if (!ok) {
        m_error = file.errorString();
    }
    return ok;
}

bool BackupArchive::readMedia(const QString &name, QByteArray *out)
{
    if (!m_zip.read(kMediaDir + name, out)) {
        m_error = m_zip.errorString();
        return false;
    }
    return true;
}
