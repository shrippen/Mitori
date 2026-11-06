#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>

#include "bundles.h"
#include "catalog.h"
#include "videos.h"

using namespace Qt::StringLiterals;

// Bundles against the files of tests/data/bundle (made by tools/bundle.py): the manifest and the
// "package" lie in a local folder (file://), never the network.
class TstBundles : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    const QString m_code = u"7KQ2M-X9HDV-3TAPR-WE6YT"_s;

    static QByteArray read(const QString &name)
    {
        QFile f(QStringLiteral(TEST_BUNDLE_DIR "/") + name);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }

    // The video folder as Caddy serves it: <id>/<version>/<file>, and a manifest with the file's
    // address; `damage` changes a byte of the video.
    QUrl publish(const QString &name, bool damage = false)
    {
        const QString base = m_dir.filePath(name + u"-packages"_s);
        QDir().mkpath(base + u"/test-bundle/1"_s);
        QByteArray video = read(u"test.mvb"_s);
        if (damage) {
            video[60] = char(video.at(60) ^ 1);
        }
        QFile f(base + u"/test-bundle/1/test.mvb"_s);
        if (!f.open(QIODevice::WriteOnly)) {
            return {};
        }
        f.write(video);
        f.close();
        QJsonObject manifest = QJsonDocument::fromJson(read(u"manifest.json"_s)).object();
        setFirstVideo(manifest, [&](QJsonObject &v) {
            v.insert(u"url"_s, QUrl::fromLocalFile(base + u"/test-bundle/1/test.mvb"_s).toString());
        });
        QFile m(m_dir.filePath(name + u"-manifest.json"_s));
        if (!m.open(QIODevice::WriteOnly)) {
            return {};
        }
        m.write(QJsonDocument(manifest).toJson());
        return QUrl::fromLocalFile(m.fileName());
    }

    // The first video of the first bundle, changed by `edit`.
    template<typename Edit>
    static void setFirstVideo(QJsonObject &manifest, Edit edit)
    {
        QJsonArray list = manifest.value(u"bundles"_s).toArray();
        QJsonObject b = list.first().toObject();
        QJsonArray videos = b.value(u"videos"_s).toArray();
        QJsonObject v = videos.first().toObject();
        edit(v);
        videos[0] = v;
        b.insert(u"videos"_s, videos);
        list[0] = b;
        manifest.insert(u"bundles"_s, list);
    }

    // Unlocks the test bundle and loads it, on `b`.
    void install(Bundles &b)
    {
        waitReady(b);
        QSignalSpy unlocked(&b, &Bundles::unlocked);
        b.unlock(m_code);
        QVERIFY(unlocked.wait(10000));
        QSignalSpy installed(&b, &Bundles::installed);
        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QVERIFY(installed.wait(10000));
    }

    // The manifest of a published test bundle, changed by `edit`.
    template<typename Edit>
    static void editManifest(const QUrl &manifest, Edit edit)
    {
        QFile m(manifest.toLocalFile());
        if (!m.open(QIODevice::ReadOnly)) {
            return;
        }
        QJsonObject o = QJsonDocument::fromJson(m.readAll()).object();
        m.close();
        edit(o);
        if (m.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            m.write(QJsonDocument(o).toJson());
        }
    }

    // A local HTTP server that sends `data` to every request in pieces of 16 KiB, one per turn of
    // the event loop, as a slow network does.
    static void serveInPieces(QTcpServer &server, const QByteArray &data)
    {
        QObject::connect(&server, &QTcpServer::newConnection, &server, [&server, data] {
            QTcpSocket *socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, data] {
                if (!socket->readAll().contains("\r\n\r\n")) {
                    return;
                }
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(data.size())
                              + "\r\nConnection: close\r\n\r\n");
                auto *at = new qsizetype(0);
                auto *timer = new QTimer(socket);
                QObject::connect(timer, &QTimer::timeout, socket, [socket, timer, data, at] {
                    if (*at >= data.size()) {
                        timer->stop();
                        delete at;
                        socket->disconnectFromHost();
                        return;
                    }
                    socket->write(data.mid(*at, 16 << 10));
                    socket->flush();
                    *at += 16 << 10;
                });
                timer->start(0);
            });
        });
    }

    static void waitReady(Bundles &b)
    {
        QSignalSpy spy(&b, &Bundles::changed);
        b.reload();
        while (b.manifestState() == Bundles::ManifestState::Loading && spy.wait(5000)) {
        }
    }

    static QVariantMap first(const Bundles &b)
    {
        return b.bundles().value(0).toMap();
    }
    static Bundles::State state(const Bundles &b)
    {
        return first(b).value(u"state"_s).value<Bundles::State>();
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
        QVERIFY(m_dir.isValid());
    }

    // The storage bar: free space also before the shelf's folder and its parents exist.
    void freeSpaceWithoutFolder()
    {
        Bundles bundles(QUrl(), m_dir.filePath(u"not/yet/there"_s));
        QVERIFY(bundles.freeSpace() > 0);
    }

    // The manifest lists the bundle locked; a bad code fails at once, a valid one unlocks it.
    void unlock()
    {
        Bundles b(publish(u"unlock"_s), m_dir.filePath(u"unlock-shelf"_s));
        waitReady(b);
        QCOMPARE(b.manifestState(), Bundles::ManifestState::Ready);
        QCOMPARE(b.bundles().size(), 1);
        QCOMPARE(state(b), Bundles::State::Locked);
        QCOMPARE(first(b).value(u"title"_s).toString(), u"Test"_s);

        QSignalSpy failed(&b, &Bundles::unlockFailed);
        QSignalSpy unlocked(&b, &Bundles::unlocked);
        b.unlock(u"7KQ2M-X9HDV-3TAPR-WE6YA"_s);             // check character wrong: a typo
        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.takeFirst().at(0).value<Bundles::UnlockError>(), Bundles::UnlockError::Typo);
        b.unlock(u"7KQ2M-X9HDV-3TAPR-WE6YU"_s);             // a character outside the alphabet: a typo too
        QCOMPARE(failed.takeFirst().at(0).value<Bundles::UnlockError>(), Bundles::UnlockError::Typo);
        b.unlock(u"7KQ2M-X9HDV"_s);                         // too short
        QCOMPARE(failed.takeFirst().at(0).value<Bundles::UnlockError>(), Bundles::UnlockError::Invalid);
        b.unlock(u"0000000000000000000"_s + u'0');           // valid, not in the manifest
        QCOMPARE(failed.takeFirst().at(0).value<Bundles::UnlockError>(), Bundles::UnlockError::Unknown);
        QVERIFY(b.codeValid(m_code.toLower()));
        QCOMPARE(b.grouped(u"7kq2mx9hdv3taprwe6yt"_s), m_code);

        b.unlock(m_code.toLower());
        QVERIFY(b.unlocking());
        QVERIFY(unlocked.wait(10000));
        QCOMPARE(unlocked.takeFirst().at(0).toString(), u"test-bundle"_s);
        QCOMPARE(state(b), Bundles::State::Unlocked);
        QCOMPARE(first(b).value(u"code"_s).toString(), m_code);
    }

    // Unlocked, it loads, checks and decrypts its videos; a new start knows them; removing keeps
    // the code, so it loads again without it.
    void loadAndRemove()
    {
        const QUrl manifest = publish(u"load"_s);
        const QString root = m_dir.filePath(u"load-shelf"_s);
        {
            Bundles b(manifest, root);
            waitReady(b);
            QSignalSpy unlocked(&b, &Bundles::unlocked);
            b.unlock(m_code);
            QVERIFY(unlocked.wait(10000));
            QSignalSpy installed(&b, &Bundles::installed);
            b.load(u"test-bundle"_s, Bundles::Network::Any);
            QVERIFY(installed.wait(10000));
            QCOMPARE(state(b), Bundles::State::Installed);
            QCOMPARE(first(b).value(u"installed"_s).toInt(), 1);
        }
        BundleShelf shelf(root);
        const QString file = shelf.path(u"test/v"_s);
        QVERIFY(!file.isEmpty());
        QVERIFY(shelf.cropped(u"test/v"_s));
        QFile plain(file);
        QVERIFY(plain.open(QIODevice::ReadOnly));
        QCOMPARE(plain.readAll(), read(u"klar.bin"_s));
        plain.close();
        QVERIFY(!QDir(root + u"/test-bundle.part"_s).exists());

        Bundles again(manifest, root);
        QCOMPARE(state(again), Bundles::State::Installed);       // the manifest from the device
        QVERIFY(again.size() > 0);
        QVERIFY(again.remove(u"test-bundle"_s));
        QCOMPARE(state(again), Bundles::State::Unlocked);
        QVERIFY(!QFile::exists(file));
        QCOMPARE(again.size(), 0);
        QSignalSpy installed(&again, &Bundles::installed);
        again.load(u"test-bundle"_s, Bundles::Network::Any);
        QVERIFY(installed.wait(10000));
        QCOMPARE(state(again), Bundles::State::Installed);
    }

    // A damaged download fails; nothing half is left; the version loaded before stays.
    void damagedDownload()
    {
        const QString root = m_dir.filePath(u"damaged-shelf"_s);
        Bundles b(publish(u"damaged"_s, true), root);
        waitReady(b);
        QSignalSpy unlocked(&b, &Bundles::unlocked);
        b.unlock(m_code);
        QVERIFY(unlocked.wait(10000));
        QSignalSpy changed(&b, &Bundles::changed);
        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QTRY_COMPARE_WITH_TIMEOUT(state(b), Bundles::State::Failed, 10000);
        QVERIFY(!b.errorString().isEmpty());
        QVERIFY(BundleShelf(root).path(u"test/v"_s).isEmpty());
        QVERIFY(!QDir(root + u"/test-bundle.part"_s).exists());
    }

    // A newer version in the manifest is an update; its hint comes along.
    void update()
    {
        const QString root = m_dir.filePath(u"update-shelf"_s);
        const QUrl manifest = publish(u"update"_s);
        {
            Bundles b(manifest, root);
            waitReady(b);
            QSignalSpy unlocked(&b, &Bundles::unlocked);
            b.unlock(m_code);
            QVERIFY(unlocked.wait(10000));
            QSignalSpy installed(&b, &Bundles::installed);
            b.load(u"test-bundle"_s, Bundles::Network::Any);
            QVERIFY(installed.wait(10000));
            QCOMPARE(b.updates(), 0);
        }
        QFile m(manifest.toLocalFile());
        QVERIFY(m.open(QIODevice::ReadOnly));
        QJsonObject o = QJsonDocument::fromJson(m.readAll()).object();
        m.close();
        QJsonArray list = o.value(u"bundles"_s).toArray();
        QJsonObject bundle = list.first().toObject();
        bundle.insert(u"version"_s, 2);
        bundle.insert(u"hinweis"_s, u"neu geschnitten"_s);
        list[0] = bundle;
        o.insert(u"bundles"_s, list);
        QVERIFY(m.open(QIODevice::WriteOnly | QIODevice::Truncate));
        m.write(QJsonDocument(o).toJson());
        m.close();

        Bundles b(manifest, root);
        waitReady(b);
        QCOMPARE(state(b), Bundles::State::Update);
        QCOMPARE(b.updates(), 1);
        QCOMPARE(first(b).value(u"hint"_s).toString(), u"neu geschnitten"_s);
    }

    // A manifest that is no manifest (an error page) does not replace the one on the device.
    void badManifestKeepsTheOld()
    {
        const QString root = m_dir.filePath(u"bad-shelf"_s);
        {
            Bundles b(publish(u"bad"_s), root);
            waitReady(b);
            QCOMPARE(b.bundles().size(), 1);
        }
        QFile junk(m_dir.filePath(u"junk.json"_s));
        QVERIFY(junk.open(QIODevice::WriteOnly));
        junk.write("<html>Not found</html>");
        junk.close();
        Bundles b(QUrl::fromLocalFile(junk.fileName()), root);
        waitReady(b);
        QCOMPARE(b.manifestState(), Bundles::ManifestState::Failed);
        QCOMPARE(b.bundles().size(), 1);
    }

    // A bundle's id from the manifest goes into a path: one that is no plain name ("..", "/x")
    // is no bundle, so nothing outside the shelf is loaded into or deleted.
    void unsafeIdsAreNoBundles()
    {
        for (const QString &id : {u".."_s, u"/tmp"_s, u"a/b"_s, u"Test"_s, u""_s, u"abc\n"_s}) {
            const QString name = u"unsafe-%1"_s.arg(QString::number(qHash(id)));
            const QUrl manifest = publish(name);
            editManifest(manifest, [&](QJsonObject &o) {
                QJsonArray list = o.value(u"bundles"_s).toArray();
                QJsonObject b = list.first().toObject();
                b.insert(u"id"_s, id);
                list[0] = b;
                o.insert(u"bundles"_s, list);
                QJsonObject codes = o.value(u"codes"_s).toObject();
                QJsonObject entry = codes.begin().value().toObject();
                entry.insert(u"bundle"_s, id);
                codes[codes.begin().key()] = entry;
                o.insert(u"codes"_s, codes);
            });
            const QString root = m_dir.filePath(name + u"/shelf"_s);
            QDir().mkpath(root);
            QFile keep(m_dir.filePath(name + u"/keep.txt"_s));
            QVERIFY(keep.open(QIODevice::WriteOnly));
            keep.close();

            Bundles b(manifest, root);
            waitReady(b);
            QCOMPARE(b.manifestState(), Bundles::ManifestState::Ready);
            QVERIFY2(b.bundles().isEmpty(), qPrintable(id));
            QSignalSpy failed(&b, &Bundles::unlockFailed);
            b.unlock(m_code);
            QCOMPARE(failed.count(), 1);
            QCOMPARE(failed.takeFirst().at(0).value<Bundles::UnlockError>(), Bundles::UnlockError::Unknown);
            b.load(id, Bundles::Network::Any);
            QVERIFY(!b.remove(id));
            QVERIFY(!BundleShelf(root).unlock(id, m_code, QByteArray(32, 'k')));
            QVERIFY(keep.exists());
            QVERIFY(QDir(root).exists());
        }
    }

    // While a video loads, the list changes a few times a second, not with every piece that
    // arrives: each change draws the page anew (a 16 MiB video in pieces of a few KiB).
    void progressFewTimesASecond()
    {
        const QUrl manifest = publish(u"progress"_s);
        const QByteArray key = QByteArray::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        const QByteArray plain(16 << 20, 'v');
        const QByteArray file = BundleCrypto::encrypted(plain, key, QByteArray(16, 'p'));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        serveInPieces(server, file);
        editManifest(manifest, [&](QJsonObject &o) {
            QJsonArray list = o.value(u"bundles"_s).toArray();
            QJsonObject b = list.first().toObject();
            QJsonArray videos = b.value(u"videos"_s).toArray();
            QJsonObject v = videos.first().toObject();
            v.insert(u"url"_s, u"http://127.0.0.1:%1/test.mvb"_s.arg(server.serverPort()));
            v.insert(u"groesse"_s, double(file.size()));
            v.insert(u"klar"_s, double(plain.size()));
            v.insert(u"sha256"_s, QString::fromLatin1(QCryptographicHash::hash(file, QCryptographicHash::Sha256).toHex()));
            videos[0] = v;
            b.insert(u"videos"_s, videos);
            list[0] = b;
            o.insert(u"bundles"_s, list);
        });

        Bundles b(manifest, m_dir.filePath(u"progress-shelf"_s));
        waitReady(b);
        QSignalSpy unlocked(&b, &Bundles::unlocked);
        b.unlock(m_code);
        QVERIFY(unlocked.wait(10000));
        QSignalSpy changed(&b, &Bundles::changed);
        QSignalSpy installed(&b, &Bundles::installed);
        QElapsedTimer clock;
        clock.start();
        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QVERIFY(installed.wait(30000));
        const qint64 allowed = 4 + clock.elapsed() / 100;      // start, end, and ten a second at most
        QVERIFY2(changed.count() <= allowed, qPrintable(u"%1 changes in %2 ms"_s.arg(changed.count()).arg(clock.elapsed())));
    }

    // The address comes from the manifest, whatever the host: the videos are not on Gitea.
    void loadsFromTheManifestsAddress()
    {
        const QString root = m_dir.filePath(u"address-shelf"_s);
        const QUrl manifest = publish(u"address"_s);
        const QString elsewhere = m_dir.filePath(u"elsewhere.mvb"_s);
        QVERIFY(QFile::copy(QStringLiteral(TEST_BUNDLE_DIR "/test.mvb"), elsewhere));
        editManifest(manifest, [&](QJsonObject &o) {
            setFirstVideo(o, [&](QJsonObject &v) { v.insert(u"url"_s, QUrl::fromLocalFile(elsewhere).toString()); });
        });
        Bundles b(manifest, root);
        install(b);
        QCOMPARE(read(u"klar.bin"_s), [&] {
            QFile f(BundleShelf(root).path(u"test/v"_s));
            return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
        }());
    }

    // No address, no download; the bundle fails with a message instead of asking for nothing.
    void missingAddressFails()
    {
        const QUrl manifest = publish(u"noaddress"_s);
        editManifest(manifest, [](QJsonObject &o) { setFirstVideo(o, [](QJsonObject &v) { v.remove(u"url"_s); }); });
        Bundles b(manifest, m_dir.filePath(u"noaddress-shelf"_s));
        waitReady(b);
        QSignalSpy unlocked(&b, &Bundles::unlocked);
        b.unlock(m_code);
        QVERIFY(unlocked.wait(10000));
        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QTRY_COMPARE_WITH_TIMEOUT(state(b), Bundles::State::Failed, 5000);
        QVERIFY(!b.errorString().isEmpty());
    }

    // A format 1 manifest has no addresses: as if it were not there.
    void oldFormatIsNoManifest()
    {
        const QUrl manifest = publish(u"format1"_s);
        editManifest(manifest, [](QJsonObject &o) { o.insert(u"format"_s, 1); });
        Bundles b(manifest, m_dir.filePath(u"format1-shelf"_s));
        waitReady(b);
        QCOMPARE(b.manifestState(), Bundles::ManifestState::Failed);
        QVERIFY(b.bundles().isEmpty());
    }

    // The cut points of a loaded version stay with its file, also when the app starts anew.
    void cutPointsComeWithTheFile()
    {
        const QString root = m_dir.filePath(u"cuts-shelf"_s);
        {
            Bundles b(publish(u"cuts"_s), root);
            install(b);
        }
        const BundleShelf shelf(root);
        const auto cut = shelf.cut(u"test/v"_s, u"test/tech"_s);
        QVERIFY(cut.has_value());
        QCOMPARE(cut->von, 1.5);
        QCOMPARE(cut->bis, 2.5);
        QVERIFY(!shelf.cut(u"test/v"_s, u"test/other"_s).has_value());
        QVERIFY(!shelf.cut(u"other/v"_s, u"test/tech"_s).has_value());
    }

    // A newer manifest moves nothing until its version is loaded: the file on the device is the old.
    void newCutPointsOnlyWithTheNewFile()
    {
        const QString root = m_dir.filePath(u"recut-shelf"_s);
        const QUrl manifest = publish(u"recut"_s);
        Bundles b(manifest, root);
        install(b);
        editManifest(manifest, [](QJsonObject &o) {
            setFirstVideo(o, [](QJsonObject &v) {
                v.insert(u"kapitel"_s, QJsonArray{QJsonObject{{u"id"_s, u"test/tech"_s}, {u"von"_s, 7.0}, {u"bis"_s, 8.0}}});
            });
            QJsonArray list = o.value(u"bundles"_s).toArray();
            QJsonObject bundle = list.first().toObject();
            bundle.insert(u"version"_s, 2);
            list[0] = bundle;
            o.insert(u"bundles"_s, list);
        });
        waitReady(b);
        QCOMPARE(state(b), Bundles::State::Update);
        QCOMPARE(BundleShelf(root).cut(u"test/v"_s, u"test/tech"_s)->von, 1.5);

        QSignalSpy installed(&b, &Bundles::installed);
        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QVERIFY(installed.wait(10000));
        QCOMPARE(BundleShelf(root).cut(u"test/v"_s, u"test/tech"_s)->von, 7.0);
    }

    // Nonsense in the manifest moves no player: the times of the app count then.
    void badCutPointsAreSkipped()
    {
        const QString root = m_dir.filePath(u"badcut-shelf"_s);
        const QUrl manifest = publish(u"badcut"_s);
        editManifest(manifest, [](QJsonObject &o) {
            setFirstVideo(o, [](QJsonObject &v) {
                v.insert(u"kapitel"_s, QJsonArray{QJsonObject{{u"id"_s, u"backwards"_s}, {u"von"_s, 5.0}, {u"bis"_s, 2.0}},
                                                  QJsonObject{{u"id"_s, u"negative"_s}, {u"von"_s, -1.0}, {u"bis"_s, 2.0}},
                                                  QJsonObject{{u"id"_s, u"text"_s}, {u"von"_s, u"a"_s}, {u"bis"_s, u"b"_s}},
                                                  QJsonObject{{u"von"_s, 1.0}, {u"bis"_s, 2.0}},
                                                  QJsonObject{{u"id"_s, u"good"_s}, {u"von"_s, 1.0}, {u"bis"_s, 2.0}}});
            });
        });
        Bundles b(manifest, root);
        install(b);
        const BundleShelf shelf(root);
        QVERIFY(shelf.cut(u"test/v"_s, u"good"_s).has_value());
        for (const QString &id : {u"backwards"_s, u"negative"_s, u"text"_s, u""_s}) {
            QVERIFY2(!shelf.cut(u"test/v"_s, id).has_value(), qPrintable(id));
        }
    }

    // The server keeps only the current version: a file gone means a new manifest. The app asks for
    // it, so that the next try loads what is there now.
    void goneFileReloadsTheManifest()
    {
        const QString root = m_dir.filePath(u"gone-shelf"_s);
        const QUrl manifest = publish(u"gone"_s);
        Bundles b(manifest, root);
        waitReady(b);
        QSignalSpy unlocked(&b, &Bundles::unlocked);
        b.unlock(m_code);
        QVERIFY(unlocked.wait(10000));

        // The server moved on after this manifest was read.
        const QString current = m_dir.filePath(u"gone-v2.mvb"_s);
        QVERIFY(QFile::copy(QStringLiteral(TEST_BUNDLE_DIR "/test.mvb"), current));
        QVERIFY(QFile::remove(m_dir.filePath(u"gone-packages/test-bundle/1/test.mvb"_s)));
        editManifest(manifest, [&](QJsonObject &o) {
            setFirstVideo(o, [&](QJsonObject &v) { v.insert(u"url"_s, QUrl::fromLocalFile(current).toString()); });
            QJsonArray list = o.value(u"bundles"_s).toArray();
            QJsonObject bundle = list.first().toObject();
            bundle.insert(u"version"_s, 2);
            list[0] = bundle;
            o.insert(u"bundles"_s, list);
        });

        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QTRY_COMPARE_WITH_TIMEOUT(state(b), Bundles::State::Failed, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(first(b).value(u"version"_s).toInt(), 2, 5000);

        QSignalSpy installed(&b, &Bundles::installed);
        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QVERIFY(installed.wait(10000));
        QCOMPARE(BundleShelf(root).bundle(u"test-bundle"_s).version, 2);
    }

    // The player asks Videos: a loaded bundle's cut point, else the app's own time.
    void videosPlaceFollowsTheShelf()
    {
        BundleShelf &shelf = BundleShelf::shared();
        const QString id = u"place-test"_s;
        QVERIFY(QDir().mkpath(shelf.folder(id)));
        QFile file(QDir(shelf.folder(id)).filePath(u"v.mkv"_s));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        const QVariantMap own{{u"video"_s, u"place/v"_s}, {u"von"_s, 10.0}, {u"bis"_s, 20.0}};
        const Catalog catalog;
        const Videos videos(catalog);
        QCOMPARE(videos.place(u"tech"_s, own), own);

        QHash<QString, QHash<QString, BundleShelf::Cut>> cuts;
        cuts[u"place/v"_s][u"tech"_s] = {1.5, 2.5};
        QVERIFY(shelf.installed(id, 1, {{u"place/v"_s, u"v.mkv"_s}}, {}, cuts));
        const QVariantMap moved = videos.place(u"tech"_s, own);
        QCOMPARE(moved.value(u"von"_s).toDouble(), 1.5);
        QCOMPARE(moved.value(u"bis"_s).toDouble(), 2.5);
        QCOMPARE(moved.value(u"video"_s).toString(), u"place/v"_s);
        QCOMPARE(videos.place(u"other"_s, own), own);

        QVERIFY(shelf.remove(id));
        QCOMPARE(videos.place(u"tech"_s, own), own);
    }

    // File names from the manifest may not leave the bundle's folder.
    void fileNamesStayInTheFolder()
    {
        const QUrl manifest = publish(u"evil"_s);
        QFile m(manifest.toLocalFile());
        QVERIFY(m.open(QIODevice::ReadOnly));
        QByteArray text = m.readAll();
        m.close();
        text.replace("\"test.mvb\"", "\"../../evil.mvb\"");
        QVERIFY(m.open(QIODevice::WriteOnly | QIODevice::Truncate));
        m.write(text);
        m.close();
        const QString root = m_dir.filePath(u"evil-shelf"_s);
        Bundles b(manifest, root);
        waitReady(b);
        QSignalSpy unlocked(&b, &Bundles::unlocked);
        b.unlock(m_code);
        QVERIFY(unlocked.wait(10000));
        b.load(u"test-bundle"_s, Bundles::Network::Any);
        QTRY_COMPARE_WITH_TIMEOUT(state(b), Bundles::State::Failed, 5000);
        QVERIFY(!QFile::exists(m_dir.filePath(u"evil.mkv"_s)));
    }
};

QTEST_GUILESS_MAIN(TstBundles)
#include "tst_bundles.moc"
