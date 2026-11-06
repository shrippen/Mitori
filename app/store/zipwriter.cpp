#include "zipwriter.h"

#include <QIODevice>
#include <QtEndian>
#include <array>

namespace {

constexpr quint32 kLocalHeader = 0x04034b50;
constexpr quint32 kCentralHeader = 0x02014b50;
constexpr quint32 kEndRecord = 0x06054b50;
constexpr quint16 kVersion = 20;            // 2.0: what a stored file needs
constexpr quint16 kUtf8Names = 0x0800;      // general purpose flag bit 11
constexpr quint16 kStored = 0;

void put16(QByteArray &b, quint16 v)
{
    char raw[2];
    qToLittleEndian(v, raw);
    b.append(raw, 2);
}

void put32(QByteArray &b, quint32 v)
{
    char raw[4];
    qToLittleEndian(v, raw);
    b.append(raw, 4);
}

// MS-DOS time and date of a local time (2-second steps, years from 1980).
quint16 dosTime(const QTime &t)
{
    return quint16((t.hour() << 11) | (t.minute() << 5) | (t.second() / 2));
}
quint16 dosDate(const QDate &d)
{
    return quint16(((std::max(d.year(), 1980) - 1980) << 9) | (d.month() << 5) | d.day());
}

} // namespace

ZipWriter::ZipWriter(QIODevice *out)
    : m_out(out)
{
}

// CRC-32 (IEEE 802.3, reflected, as ZIP and zlib use it).
quint32 ZipWriter::crc32(const QByteArray &data)
{
    static const auto table = [] {
        std::array<quint32, 256> t{};
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            t[i] = c;
        }
        return t;
    }();
    quint32 crc = 0xFFFFFFFFu;
    for (const char ch : data) {
        crc = table[(crc ^ quint8(ch)) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

bool ZipWriter::write(const QByteArray &bytes)
{
    if (!m_ok) {
        return false;
    }
    m_ok = m_out->write(bytes) == bytes.size();
    m_offset += quint32(bytes.size());
    return m_ok;
}

qint64 ZipWriter::sizeOf(const QString &name, qint64 bytes)
{
    constexpr qint64 kLocalSize = 30;
    constexpr qint64 kCentralSize = 46;
    return kLocalSize + kCentralSize + 2 * name.toUtf8().size() + bytes;
}

bool ZipWriter::add(const QString &name, const QByteArray &data, const QDateTime &modified)
{
    const QDateTime local = modified.toLocalTime();
    Entry e{name.toUtf8(), crc32(data), quint32(data.size()), m_offset, dosTime(local.time()), dosDate(local.date())};

    QByteArray header;
    put32(header, kLocalHeader);
    put16(header, kVersion);
    put16(header, kUtf8Names);
    put16(header, kStored);
    put16(header, e.time);
    put16(header, e.date);
    put32(header, e.crc);
    put32(header, e.size);              // compressed: stored as is
    put32(header, e.size);
    put16(header, quint16(e.name.size()));
    put16(header, 0);                   // no extra field
    header += e.name;
    if (!write(header) || !write(data)) {
        return false;
    }
    m_entries << e;
    return true;
}

bool ZipWriter::finish()
{
    const quint32 start = m_offset;
    QByteArray directory;
    for (const Entry &e : std::as_const(m_entries)) {
        put32(directory, kCentralHeader);
        put16(directory, kVersion);     // made by
        put16(directory, kVersion);     // needed
        put16(directory, kUtf8Names);
        put16(directory, kStored);
        put16(directory, e.time);
        put16(directory, e.date);
        put32(directory, e.crc);
        put32(directory, e.size);
        put32(directory, e.size);
        put16(directory, quint16(e.name.size()));
        put16(directory, 0);            // extra
        put16(directory, 0);            // comment
        put16(directory, 0);            // disk
        put16(directory, 0);            // internal attributes
        put32(directory, 0);            // external attributes
        put32(directory, e.offset);
        directory += e.name;
    }
    QByteArray end;
    put32(end, kEndRecord);
    put16(end, 0);
    put16(end, 0);
    put16(end, quint16(m_entries.size()));
    put16(end, quint16(m_entries.size()));
    put32(end, quint32(directory.size()));
    put32(end, start);
    put16(end, 0);                      // comment
    return write(directory) && write(end);
}
