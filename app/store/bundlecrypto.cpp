#include "bundlecrypto.h"

#include <QIODevice>

#include <memory>

extern "C" {
#include "monocypher.h"
}

namespace {

const QString kAlphabet = QStringLiteral("0123456789ABCDEFGHJKMNPQRSTVWXYZ");   // Crockford-Base32
constexpr int kRandom = 19;                 // characters, then the check
constexpr int kGroup = 5;
constexpr int kKey = 32;
constexpr int kNonce = 24;
constexpr int kPrefix = 16;
const QByteArray kMagic = QByteArrayLiteral("MITORIV1");
// Argon2id beyond this would make a damaged or hostile manifest hang the phone.
constexpr int kMaxKibibytes = 256 * 1024;
constexpr int kMaxPasses = 16;

// Luhn mod 32 over the characters' values: one wrong character or two swapped ones show.
QChar check(const QString &characters)
{
    int sum = 0;
    int factor = 2;
    for (qsizetype i = characters.size() - 1; i >= 0; --i) {
        const int value = factor * int(kAlphabet.indexOf(characters.at(i)));
        sum += value / 32 + value % 32;
        factor = factor == 2 ? 1 : 2;
    }
    return kAlphabet.at((32 - sum % 32) % 32);
}

QByteArray le64(quint64 value)
{
    QByteArray out(8, Qt::Uninitialized);
    for (int i = 0; i < 8; ++i) {
        out[i] = char((value >> (8 * i)) & 0xff);
    }
    return out;
}

QByteArray chunkAd(quint64 index, bool last)
{
    return QByteArrayLiteral("mitori-chunk") + le64(index) + char(last ? 1 : 0);
}

const uint8_t *bytes(const QByteArray &a)
{
    return reinterpret_cast<const uint8_t *>(a.constData());
}

} // namespace

namespace BundleCrypto {

QString normalized(const QString &input)
{
    QString s = input.toUpper();
    s.remove(QLatin1Char('-'));
    s.remove(QLatin1Char(' '));
    s.replace(QLatin1Char('O'), QLatin1Char('0'));
    s.replace(QLatin1Char('I'), QLatin1Char('1'));
    s.replace(QLatin1Char('L'), QLatin1Char('1'));
    if (s.size() != kRandom + 1) {
        return {};
    }
    for (const QChar c : s) {
        if (!kAlphabet.contains(c)) {
            return {};
        }
    }
    return check(s.left(kRandom)) == s.at(kRandom) ? s : QString();
}

QString grouped(const QString &code)
{
    QStringList parts;
    for (qsizetype i = 0; i < code.size(); i += kGroup) {
        parts << code.mid(i, kGroup);
    }
    return parts.join(QLatin1Char('-'));
}

QString lookupId(const QString &code)
{
    const QByteArray message = QByteArrayLiteral("mitori-code:") + code.toLatin1();
    QByteArray hash(16, Qt::Uninitialized);
    crypto_blake2b(reinterpret_cast<uint8_t *>(hash.data()), size_t(hash.size()), bytes(message), size_t(message.size()));
    return QString::fromLatin1(hash.toHex());
}

QByteArray unwrapKey(const QString &code, const QString &bundle, const QByteArray &salt, int passes, int kibibytes,
                     const QByteArray &wrapped)
{
    if (passes < 1 || passes > kMaxPasses || kibibytes < 8 || kibibytes > kMaxKibibytes
        || wrapped.size() != kNonce + kKey + Decryptor::kTag || salt.size() < 8) {
        return {};
    }
    const QByteArray password = code.toLatin1();

    // Argon2id, one lane (libsodium's crypto_pwhash in the tool).
    std::unique_ptr<uint8_t[]> work(new (std::nothrow) uint8_t[size_t(kibibytes) * 1024]);
    if (!work) {
        return {};
    }
    QByteArray kek(kKey, Qt::Uninitialized);
    crypto_argon2_config config{CRYPTO_ARGON2_ID, uint32_t(kibibytes), uint32_t(passes), 1};
    crypto_argon2_inputs inputs{bytes(password), bytes(salt), uint32_t(password.size()), uint32_t(salt.size())};
    crypto_argon2(reinterpret_cast<uint8_t *>(kek.data()), kKey, work.get(), config, inputs, crypto_argon2_no_extras);

    const QByteArray ad = QByteArrayLiteral("mitori-key:") + bundle.toUtf8();
    QByteArray key(kKey, Qt::Uninitialized);
    const int failed = crypto_aead_unlock(reinterpret_cast<uint8_t *>(key.data()), bytes(wrapped) + kNonce + kKey,
                                          bytes(kek), bytes(wrapped), bytes(ad), size_t(ad.size()),
                                          bytes(wrapped) + kNonce, kKey);
    crypto_wipe(kek.data(), size_t(kek.size()));
    if (failed) {
        crypto_wipe(key.data(), size_t(key.size()));
        return {};
    }
    return key;
}

Decryptor::Decryptor(const QByteArray &key, qint64 total, QIODevice *out)
    : m_key(key)
    , m_total(total)
    , m_out(out)
{
}

Decryptor::~Decryptor()
{
    crypto_wipe(m_key.data(), size_t(m_key.size()));
}

bool Decryptor::feed(const QByteArray &data)
{
    if (m_failed || m_done) {
        return !m_failed && data.isEmpty();
    }
    m_received += data.size();
    if (m_received > m_total) {
        m_failed = true;
        return false;
    }
    m_buffer += data;

    // The header: magic and the nonce prefix.
    if (m_prefix.isEmpty()) {
        if (m_buffer.size() < kMagic.size() + kPrefix) {
            return true;
        }
        if (!m_buffer.startsWith(kMagic)) {
            m_failed = true;
            return false;
        }
        m_prefix = m_buffer.mid(kMagic.size(), kPrefix);
        m_buffer.remove(0, kMagic.size() + kPrefix);
    }

    // Whole blocks; the last one when every byte is in.
    const qint64 sealed = kBlock + kTag;
    while (m_buffer.size() >= sealed) {
        const bool last = m_received == m_total && m_buffer.size() == sealed;
        if (!open(last)) {
            return false;
        }
        if (last) {
            return true;
        }
    }
    if (m_received == m_total && !m_buffer.isEmpty() && !open(true)) {
        return false;
    }
    return true;
}

// Opens the block at the front of the buffer (all of it when `last`).
bool Decryptor::open(bool last)
{
    const qint64 sealed = last ? m_buffer.size() : kBlock + kTag;
    if (sealed < kTag) {
        m_failed = true;
        return false;
    }
    const qint64 plainSize = sealed - kTag;
    const QByteArray nonce = m_prefix + le64(m_index);
    const QByteArray ad = chunkAd(m_index, last);
    QByteArray plain(plainSize, Qt::Uninitialized);
    if (crypto_aead_unlock(reinterpret_cast<uint8_t *>(plain.data()), bytes(m_buffer) + plainSize, bytes(m_key),
                           bytes(nonce), bytes(ad), size_t(ad.size()), bytes(m_buffer), size_t(plainSize))
        != 0) {
        m_failed = true;
        return false;
    }
    if (m_out->write(plain) != plainSize) {
        m_failed = true;
        return false;
    }
    m_buffer.remove(0, sealed);
    ++m_index;
    m_done = last;
    return true;
}

bool Decryptor::finish()
{
    return !m_failed && m_done && m_received == m_total;
}

QByteArray encrypted(const QByteArray &plain, const QByteArray &key, const QByteArray &prefix)
{
    QByteArray out = kMagic + prefix;
    quint64 index = 0;
    qint64 at = 0;
    do {
        const QByteArray block = plain.mid(at, Decryptor::kBlock);
        at += block.size();
        const bool last = at >= plain.size();
        const QByteArray nonce = prefix + le64(index);
        const QByteArray ad = chunkAd(index, last);
        QByteArray sealed(block.size() + Decryptor::kTag, Qt::Uninitialized);
        crypto_aead_lock(reinterpret_cast<uint8_t *>(sealed.data()), reinterpret_cast<uint8_t *>(sealed.data()) + block.size(),
                         bytes(key), bytes(nonce), bytes(ad), size_t(ad.size()), bytes(block), size_t(block.size()));
        out += sealed;
        ++index;
        if (last) {
            break;
        }
    } while (true);
    return out;
}

}
