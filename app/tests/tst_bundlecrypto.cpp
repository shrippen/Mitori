#include <QBuffer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTest>

#include "bundlecrypto.h"

using namespace Qt::StringLiterals;

// The bundle format against files made by tools/bundle.py (PyNaCl/libsodium): tests/data/bundle.
class TstBundleCrypto : public QObject
{
    Q_OBJECT

    static QByteArray read(const QString &name)
    {
        QFile f(QStringLiteral(TEST_BUNDLE_DIR "/") + name);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }
    static QByteArray testKey()
    {
        QByteArray k(32, Qt::Uninitialized);
        for (int i = 0; i < 32; ++i) {
            k[i] = char(i);
        }
        return k;
    }
    static QByteArray random(qsizetype size)
    {
        QByteArray out(size, Qt::Uninitialized);
        QRandomGenerator::global()->fillRange(reinterpret_cast<quint32 *>(out.data()), size / 4);
        return out;
    }
    // Decrypts in pieces of `step` bytes; empty when it fails.
    static QByteArray decrypted(const QByteArray &file, const QByteArray &key, qint64 total, qsizetype step)
    {
        QBuffer out;
        out.open(QIODevice::WriteOnly);
        BundleCrypto::Decryptor d(key, total, &out);
        for (qsizetype at = 0; at < file.size(); at += step) {
            if (!d.feed(file.mid(at, step))) {
                return {};
            }
        }
        return d.finish() ? out.data() : QByteArray("FAILED");
    }

private slots:
    void codes()
    {
        const QString code = u"7KQ2MX9HDV3TAPRWE6YT"_s;
        QCOMPARE(BundleCrypto::normalized(u"7kq2m-x9hdv 3tapr-we6yt"_s), code);
        QCOMPARE(BundleCrypto::grouped(code), u"7KQ2M-X9HDV-3TAPR-WE6YT"_s);
        QVERIFY(BundleCrypto::normalized(u"7KQ2MX9HDV3TAPRWE6YA"_s).isEmpty());     // wrong check
        QVERIFY(BundleCrypto::normalized(u"K7Q2MX9HDV3TAPRWE6YT"_s).isEmpty());     // swapped
        QVERIFY(BundleCrypto::normalized(u"7KQ2MX9HDV3TAPRWE6Y"_s).isEmpty());      // short
        QVERIFY(BundleCrypto::normalized(u"7KQ2MX9HDV3TAPRWE6YU"_s).isEmpty());     // U
        // O is 0, I and L are 1 (both sides alike).
        QCOMPARE(BundleCrypto::normalized(u"7KQ2MX9HDV3TAPRWE6YT"_s.replace(u'0', u'O')), code);
    }

    // The manifest's entry: same lookup id and the same key as the tool's.
    void unwrapLikeTheTool()
    {
        const QJsonObject manifest = QJsonDocument::fromJson(read(u"manifest.json"_s)).object();
        const QString code = BundleCrypto::normalized(u"7KQ2M-X9HDV-3TAPR-WE6YT"_s);
        const QString id = BundleCrypto::lookupId(code);
        QCOMPARE(id, u"c6ed7df09b097d62c971802d6c4d6906"_s);
        const QJsonObject entry = manifest.value(u"codes"_s).toObject().value(id).toObject();
        QVERIFY(!entry.isEmpty());
        const auto unwrap = [&](const QString &c, const QString &bundle) {
            return BundleCrypto::unwrapKey(c, bundle, QByteArray::fromBase64(entry.value(u"salz"_s).toString().toLatin1()),
                                           entry.value(u"t"_s).toInt(), entry.value(u"m"_s).toInt(),
                                           QByteArray::fromBase64(entry.value(u"schluessel"_s).toString().toLatin1()));
        };
        QCOMPARE(unwrap(code, u"test-bundle"_s), testKey());
        QVERIFY(unwrap(code, u"another-bundle"_s).isEmpty());
        QVERIFY(unwrap(u"7KQ2MX9HDV3TAPRWE6Y0"_s, u"test-bundle"_s).isEmpty());
        // Parameters out of bounds do not run.
        QVERIFY(BundleCrypto::unwrapKey(code, u"test-bundle"_s, "0123456789abcdef", 1, 1 << 30, QByteArray(72, 'x')).isEmpty());
    }

    // The tool's file, fed in pieces of any size.
    void decryptToolFile()
    {
        const QByteArray file = read(u"test.mvb"_s);
        const QByteArray plain = read(u"klar.bin"_s);
        QVERIFY(!file.isEmpty());
        for (const qsizetype step : {1, 7, 24, 100, 4096}) {
            QCOMPARE(decrypted(file, testKey(), file.size(), step), plain);
        }
        QByteArray wrong = testKey();
        wrong[0] = 'x';
        QVERIFY(decrypted(file, wrong, file.size(), 4096).isEmpty());
    }

    // Several blocks: whole, short, damaged, reordered, with bytes too many.
    void blocks()
    {
        const QByteArray key = random(32);
        const QByteArray prefix = random(16);
        for (const qint64 size : {qint64(0), qint64(5), BundleCrypto::Decryptor::kBlock, BundleCrypto::Decryptor::kBlock + 1,
                                  2 * BundleCrypto::Decryptor::kBlock + 17}) {
            const QByteArray plain = random(size);
            const QByteArray file = BundleCrypto::encrypted(plain, key, prefix);
            QCOMPARE(decrypted(file, key, file.size(), 65536), plain);
        }

        const QByteArray plain = random(2 * BundleCrypto::Decryptor::kBlock + 17);
        const QByteArray file = BundleCrypto::encrypted(plain, key, prefix);
        const qsizetype sealed = BundleCrypto::Decryptor::kBlock + BundleCrypto::Decryptor::kTag;
        // Without the last block: the second counts as last now, its AD does not fit.
        const QByteArray shortened = file.left(24 + 2 * sealed);
        QVERIFY(decrypted(shortened, key, shortened.size(), 65536).isEmpty());
        // Shorter than the manifest says: finish() fails.
        QCOMPARE(decrypted(shortened, key, file.size(), 65536), QByteArray("FAILED"));
        QByteArray damaged = file;
        damaged[100] = char(damaged.at(100) ^ 1);
        QVERIFY(decrypted(damaged, key, damaged.size(), 65536).isEmpty());
        const QByteArray swapped = file.left(24) + file.mid(24 + sealed, sealed) + file.mid(24, sealed) + file.mid(24 + 2 * sealed);
        QVERIFY(decrypted(swapped, key, swapped.size(), 65536).isEmpty());
        QVERIFY(decrypted(file + "x", key, file.size(), 65536).isEmpty());
        QVERIFY(decrypted("NOTMITORI0123456789abcdefghij", key, 29, 4).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstBundleCrypto)
#include "tst_bundlecrypto.moc"
