#pragma once

#include <QByteArray>
#include <QString>

class QIODevice;

// The cryptography of video bundles (format in tools/bundle.py, the tool that makes them):
//
//   code (20 Crockford-Base32 characters, the last a check)  ──BLAKE2b──►  lookup id in the manifest
//        └──Argon2id(salt, t, m)──► key-encryption key ──XChaCha20-Poly1305──► the bundle's content key
//   content key ──XChaCha20-Poly1305, blocks of 1 MiB──► the video files
//
// The app holds no secret: the code the user types is the only one. Monocypher does the work.
namespace BundleCrypto {

// The 20 characters of a code as typed ("7kq2m-x9hdv …": case, hyphens and spaces do not matter;
// O counts as 0, I and L as 1), or "" when length, characters or check do not fit.
QString normalized(const QString &input);
// "7KQ2M-X9HDV-3TAPR-WE6YC"
QString grouped(const QString &code);
// The code's entry in the manifest: hex(BLAKE2b-128("mitori-code:" + code)).
QString lookupId(const QString &code);

// The bundle's content key from a code and its manifest entry ({salz, t, m, schluessel}); empty
// when the code does not open it (wrong code, another bundle, damaged entry, parameters too large).
QByteArray unwrapKey(const QString &code, const QString &bundle, const QByteArray &salt, int passes,
                     int kibibytes, const QByteArray &wrapped);

// Decrypts a bundle file as it arrives (a download): feed() the encrypted bytes in any pieces,
// finish() at the end. The plain text goes to `out` block by block. `total` is the encrypted
// file's size (the manifest's): it tells the last block, so a shortened file fails.
class Decryptor
{
public:
    Decryptor(const QByteArray &key, qint64 total, QIODevice *out);
    ~Decryptor();
    Decryptor(const Decryptor &) = delete;
    Decryptor &operator=(const Decryptor &) = delete;

    // False once a block does not open or more bytes come than `total`; it stays false.
    bool feed(const QByteArray &data);
    // True when exactly `total` bytes came and every block opened.
    bool finish();

    static constexpr qint64 kBlock = 1 << 20;   // plain text per block
    static constexpr int kTag = 16;

private:
    bool open(bool last);

    QByteArray m_key;
    qint64 m_total;
    QIODevice *m_out;
    QByteArray m_buffer;
    QByteArray m_prefix;
    qint64 m_received = 0;
    quint64 m_index = 0;
    bool m_failed = false;
    bool m_done = false;
};

// For tests and tools: the same format the other way (tools/bundle.py writes the real ones).
QByteArray encrypted(const QByteArray &plain, const QByteArray &key, const QByteArray &prefix);

}
