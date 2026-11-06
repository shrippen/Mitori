#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>

class QIODevice;

// A ZIP archive written to a device (the notebook export): files stored as they are, without
// compression (photos and voice notes are compressed already), names in UTF-8. Enough for every
// unzip, file manager and Nextcloud; no ZIP64 (an export stays far below 4 GiB).
//
//   local header + data   … for each file
//   central directory     one record per file
//   end record
class ZipWriter
{
public:
    explicit ZipWriter(QIODevice *out);

    // Adds a file; false once the device fails (then the archive is broken).
    bool add(const QString &name, const QByteArray &data, const QDateTime &modified = QDateTime::currentDateTime());
    // Writes the directory; the archive is complete after it.
    bool finish();

    static quint32 crc32(const QByteArray &data);
    // The archive's bytes for a file of this name and size; all files plus kEndSize is its size.
    static qint64 sizeOf(const QString &name, qint64 bytes);
    static constexpr qint64 kEndSize = 22;

private:
    struct Entry {
        QByteArray name;
        quint32 crc = 0;
        quint32 size = 0;
        quint32 offset = 0;
        quint16 time = 0;
        quint16 date = 0;
    };

    bool write(const QByteArray &bytes);

    QIODevice *m_out;
    QList<Entry> m_entries;
    quint32 m_offset = 0;
    bool m_ok = true;
};
