#include "zipreader.h"

#include "zipwriter.h"

#include <QCoreApplication>
#include <QIODevice>
#include <QtEndian>

namespace {

constexpr quint32 kLocalHeader = 0x04034b50;
constexpr quint32 kCentralHeader = 0x02014b50;
constexpr quint32 kEndRecord = 0x06054b50;
constexpr quint16 kStored = 0;
constexpr int kEndSize = 22;                // the end record without a comment
constexpr int kMaxComment = 0xFFFF;
constexpr int kCentralSize = 46;            // a directory record without its name
constexpr int kLocalSize = 30;              // a local header without name and extra field

quint16 get16(const char *p)
{
    return qFromLittleEndian<quint16>(p);
}

quint32 get32(const char *p)
{
    return qFromLittleEndian<quint32>(p);
}

QString broken()
{
    return QCoreApplication::translate("ZipReader", "the archive is damaged");
}

} // namespace

bool ZipReader::fail(const QString &why)
{
    m_error = why;
    return false;
}

bool ZipReader::open(QIODevice *in)
{
    m_in = in;
    m_entries.clear();
    m_order.clear();
    m_error.clear();
    if (!in || in->isSequential() || in->size() < kEndSize) {
        return fail(QCoreApplication::translate("ZipReader", "this is no backup file"));
    }

    // The end record: the last one in the file, a comment may follow it.
    const qint64 tail = std::min<qint64>(in->size(), kEndSize + kMaxComment);
    in->seek(in->size() - tail);
    const QByteArray end = in->read(tail);
    qsizetype at = end.size() - kEndSize;
    while (at >= 0 && get32(end.constData() + at) != kEndRecord) {
        --at;
    }
    if (at < 0) {
        return fail(QCoreApplication::translate("ZipReader", "this is no backup file"));
    }
    const char *e = end.constData() + at;
    const int count = get16(e + 10);
    const quint32 dirSize = get32(e + 12);
    const quint32 dirStart = get32(e + 16);
    if (qint64(dirStart) + dirSize > in->size()) {
        return fail(broken());
    }

    // The directory: one record per file.
    in->seek(dirStart);
    const QByteArray dir = in->read(dirSize);
    qsizetype pos = 0;
    for (int i = 0; i < count; ++i) {
        if (pos + kCentralSize > dir.size() || get32(dir.constData() + pos) != kCentralHeader) {
            return fail(broken());
        }
        const char *r = dir.constData() + pos;
        const int nameLength = get16(r + 28);
        const int extra = get16(r + 30);
        const int comment = get16(r + 32);
        if (pos + kCentralSize + nameLength > dir.size()) {
            return fail(broken());
        }
        const QString name = QString::fromUtf8(r + kCentralSize, nameLength);
        m_entries.insert(name, Entry{get32(r + 16), get32(r + 24), get32(r + 42), get16(r + 10)});
        m_order << name;
        pos += kCentralSize + nameLength + extra + comment;
    }
    return true;
}

bool ZipReader::read(const QString &name, QByteArray *out)
{
    const auto it = m_entries.constFind(name);
    if (it == m_entries.cend()) {
        return fail(QCoreApplication::translate("ZipReader", "%1 is missing").arg(name));
    }
    if (it->method != kStored) {
        return fail(QCoreApplication::translate("ZipReader", "%1 is compressed").arg(name));
    }

    // The local header has its own name and extra field lengths; the data follows them.
    m_in->seek(it->offset);
    const QByteArray header = m_in->read(kLocalSize);
    if (header.size() != kLocalSize || get32(header.constData()) != kLocalHeader) {
        return fail(broken());
    }
    m_in->seek(it->offset + kLocalSize + get16(header.constData() + 26) + get16(header.constData() + 28));
    *out = m_in->read(it->size);
    if (out->size() != qsizetype(it->size) || ZipWriter::crc32(*out) != it->crc) {
        return fail(broken());
    }
    return true;
}
