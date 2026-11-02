#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

class QIODevice;

// Reads a ZIP archive that ZipWriter wrote (a backup): stored files only, no ZIP64. The directory
// at the end lists the files; each one is checked against its CRC-32 when it is read.
class ZipReader
{
public:
    // Reads the directory of a seekable device; false (errorString()) for anything else.
    bool open(QIODevice *in);
    QString errorString() const { return m_error; }

    QStringList names() const { return m_order; }
    // A file's bytes; false (errorString()) when it is missing, compressed or broken.
    bool read(const QString &name, QByteArray *out);

private:
    struct Entry {
        quint32 crc = 0;
        quint32 size = 0;
        quint32 offset = 0;     // of the local header
        quint16 method = 0;
    };

    bool fail(const QString &why);

    QIODevice *m_in = nullptr;
    QHash<QString, Entry> m_entries;
    QStringList m_order;
    QString m_error;
};
