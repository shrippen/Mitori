#include "syncjob.h"

#include "backuparchive.h"
#include "coursebook.h"
#include "mediastore.h"
#include "practicelog.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QSet>

namespace {

const QString kDevices = QStringLiteral("geraete");
const QString kMedia = QStringLiteral("medien");
const QString kSuffix = QStringLiteral(".zip");
const QString kDownload = QStringLiteral("abgleich.zip");
const QString kPractice = QStringLiteral("abgleich-mitori.sqlite");
const QString kCourses = QStringLiteral("abgleich-lehrgaenge.sqlite");

SyncJob::Output failed(SyncJob::Output out, const QString &why)
{
    out.ok = false;
    out.error = why;
    return out;
}

bool writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(data) == data.size() && file.flush();
}

// The open conflicts: one per row, the other side's newest version.
void collect(QHash<QString, TableMerge::Conflict> *open, const QList<TableMerge::Conflict> &found)
{
    for (const TableMerge::Conflict &c : found) {
        const auto it = open->constFind(c.uid);
        if (it == open->cend() || it->theirsChanged < c.theirsChanged) {
            open->insert(c.uid, c);
        }
    }
}

} // namespace

SyncJob::Output SyncJob::run(const Input &in)
{
    Output out;
    out.started = QDateTime::currentDateTimeUtc();
    WebDav dav(in.account);
    QDir().mkpath(in.work);
    QDir work(in.work);
    MediaStore media(in.media);

    // 1  The folders and what is in them.
    QList<WebDav::Item> archives;
    QList<WebDav::Item> files;
    if (!dav.ensureFolder({}) || !dav.ensureFolder(kDevices) || !dav.ensureFolder(kMedia)
        || !dav.list(kDevices, &archives) || !dav.list(kMedia, &files)) {
        return failed(out, dav.errorString());
    }
    QSet<QString> remoteMedia;
    for (const WebDav::Item &f : std::as_const(files)) {
        remoteMedia.insert(f.name);
    }

    // 2  Every other device's data merged in.
    QStringList held;
    for (const TableMerge::Conflict &c : in.held) {
        held << c.uid;
    }
    const TableMerge::Watch watch{in.base, held};
    QHash<QString, TableMerge::Conflict> open;
    QSet<QString> named;
    QStringList orphans;
    for (const WebDav::Item &a : std::as_const(archives)) {
        if (a.folder || !a.name.endsWith(kSuffix) || a.name == in.id + kSuffix) {
            continue;
        }
        QByteArray data;
        if (!dav.get(kDevices + QLatin1Char('/') + a.name, &data) || !writeFile(work.filePath(kDownload), data)) {
            return failed(out, dav.errorString());
        }
        QFile file(work.filePath(kDownload));
        BackupArchive archive;
        if (!file.open(QIODevice::ReadOnly) || !archive.open(&file)
            || !archive.unpack(work.filePath(kPractice), work.filePath(kCourses))) {
            return failed(out, a.name + QStringLiteral(": ") + archive.errorString());
        }
        const BackupArchive::Manifest m = archive.manifest();
        out.devices << SyncAccount::Device{a.name.chopped(kSuffix.size()), m.name, m.created};

        PracticeLog log;
        CourseBook book;
        TableMerge::Result fromLog;
        TableMerge::Result fromBook;
        if (!log.open(in.practice) || !log.merge(work.filePath(kPractice), TableMerge::Mode::Apply, &fromLog, watch)) {
            return failed(out, log.errorString());
        }
        if (!book.open(in.courses) || !book.merge(work.filePath(kCourses), TableMerge::Mode::Apply, &fromBook, watch)) {
            return failed(out, book.errorString());
        }
        // A conflict names the device whose archive brought it.
        QList<TableMerge::Conflict> found = fromLog.conflicts + fromBook.conflicts;
        for (TableMerge::Conflict &c : found) {
            c.theirDevice = m.name.isEmpty() ? c.theirDevice : m.name;
        }
        collect(&open, found);
        for (const TableMerge::Result *r : {&fromLog, &fromBook}) {
            for (auto it = r->added.cbegin(); it != r->added.cend(); ++it) {
                out.received += it.value() + r->changed.value(it.key());
            }
        }
        named = QSet<QString>(fromLog.media.cbegin(), fromLog.media.cend())
                + QSet<QString>(fromBook.media.cbegin(), fromBook.media.cend());
        orphans << fromLog.orphans << fromBook.orphans;
    }
    for (const QString &name : {kDownload, kPractice, kCourses}) {
        work.remove(name);
    }
    out.conflicts = open.values();

    // 3  Media: the merged rows' files down, this device's files up; a dropped one goes.
    for (const QString &name : std::as_const(named)) {
        if (media.has(name) || !remoteMedia.contains(name)) {
            continue;
        }
        QByteArray data;
        if (!dav.get(kMedia + QLatin1Char('/') + name, &data)) {
            return failed(out, dav.errorString());
        }
        media.adopt(name, data);
    }
    for (const QString &name : std::as_const(orphans)) {
        if (!named.contains(name)) {
            media.remove(name);
        }
    }
    for (const QString &name : media.names()) {
        if (remoteMedia.contains(name)) {
            continue;
        }
        QFile file(media.path(name));
        if (!file.open(QIODevice::ReadOnly) || !dav.put(kMedia + QLatin1Char('/') + name, file.readAll())) {
            return failed(out, dav.errorString().isEmpty() ? file.errorString() : dav.errorString());
        }
    }

    // 4  This device's data (with what came in) for the others.
    const QString practice = work.filePath(kPractice);
    const QString courses = work.filePath(kCourses);
    QString why;
    {
        PracticeLog log;
        CourseBook book;
        if (!log.open(in.practice) || !log.snapshot(practice)) {
            why = log.errorString();
        } else if (!book.open(in.courses) || !book.snapshot(courses)) {
            why = book.errorString();
        }
    }
    // The base for the next sync: the versions in the archive the others get.
    if (why.isEmpty()) {
        PracticeLog log;
        CourseBook book;
        if (log.open(practice) && book.open(courses)) {
            out.base = log.noteVersions();
            out.base.insert(book.entryVersions());
        }
    }
    // An open conflict has seen the other side's version: after the choice only a newer one clashes.
    for (const TableMerge::Conflict &c : std::as_const(out.conflicts)) {
        out.base.insert(c.uid, std::max(out.base.value(c.uid), c.theirsChanged));
    }
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    const BackupArchive::Manifest manifest{BackupArchive::kFormat, QCoreApplication::applicationVersion(),
                                           QDateTime::currentDateTimeUtc(), in.rowsDevice, in.name};
    const bool written = why.isEmpty()
                         && BackupArchive::write(&buffer, practice, courses, media, manifest, &why,
                                                 BackupArchive::Media::Leave);
    work.remove(kPractice);
    work.remove(kCourses);
    if (!written) {
        return failed(out, why);
    }
    if (!dav.put(kDevices + QLatin1Char('/') + in.id + kSuffix, buffer.data())) {
        return failed(out, dav.errorString());
    }
    out.ok = true;
    return out;
}
