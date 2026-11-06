#pragma once

#include "zipreader.h"

#include <QDateTime>
#include <QString>
#include <QStringList>

class MediaStore;
class QIODevice;

// A backup of everything the user wrote, as one ZIP (Einstellungen → Sicherung): both databases
// as consistent copies and the photos and voice notes. Read back, it is merged into the device's
// data (TableMerge), so an older backup's rows never overwrite newer ones, or replaces it (Backup).
//
//   manifest.json        {"app": "Mitori", "format": 1, "version": "0.3.0",
//                         "created": "2026-11-02T18:00:00Z", "device": "…"}
//   mitori.sqlite        ratings, notes, exam rounds (PracticeLog)
//   lehrgaenge.sqlite    notebooks, entries, grades (CourseBook)
//   medien/3f2a….jpg     every file of the media folder
class BackupArchive
{
public:
    struct Manifest {
        int format = 0;
        QString version;        // of the app that wrote it
        QDateTime created;      // UTC
        QString device;
        QString name;           // the device's name for the user ("Tablet am Mattenrand"), or ""
    };

    // The sync leaves the media out: they travel one by one (Sync).
    enum class Media { Include, Leave };

    static const QString kPractice;
    static const QString kCourses;
    static constexpr int kFormat = 1;

    // Writes an archive from the two database copies and the media folder; false with `error`.
    static bool write(QIODevice *out, const QString &practice, const QString &courses, const MediaStore &media,
                      const Manifest &manifest, QString *error, Media withMedia = Media::Include);

    // Reads an archive's directory and manifest; false (errorString()) when it is no backup or
    // one of a newer format.
    bool open(QIODevice *in);
    QString errorString() const { return m_error; }
    Manifest manifest() const { return m_manifest; }

    // A database of the archive (kPractice, kCourses) as a file.
    bool extract(const QString &name, const QString &path);
    // Both databases as files, migrated to this app's schema (ready for merge()).
    bool unpack(const QString &practice, const QString &courses);
    // The names of its media files, and one file's bytes.
    QStringList media() const { return m_media; }
    bool readMedia(const QString &name, QByteArray *out);

private:
    ZipReader m_zip;
    Manifest m_manifest;
    QStringList m_media;
    QString m_error;
};
