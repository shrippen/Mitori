#include <QBuffer>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <csignal>
#include <sys/resource.h>

#include "coursebook.h"
#include "mediastore.h"
#include "practicelog.h"
#include "sync.h"
#include "webdav.h"
#include "zipreader.h"

using namespace Qt::StringLiterals;

// A WebDAV server in memory, enough for WebDav: MKCOL, PROPFIND (depth 1), GET, PUT, Basic auth.
// One request per connection.
class FakeDav : public QTcpServer
{
public:
    QSet<QString> folders{u"/dav"_s};
    QMap<QString, QByteArray> files;
    QByteArray auth = "Basic " + QByteArray("ich:geheim").toBase64();
    int requests = 0;

    FakeDav()
    {
        listen(QHostAddress::LocalHost);
        connect(this, &QTcpServer::newConnection, this, [this] {
            QTcpSocket *s = nextPendingConnection();
            connect(s, &QTcpSocket::readyRead, s, [this, s] { take(s); });
            connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
        });
    }

    QString url() const { return u"http://127.0.0.1:%1/dav/Mitori/"_s.arg(serverPort()); }

private:
    QHash<QTcpSocket *, QByteArray> m_in;

    static QString trimmed(QString p)
    {
        while (p.size() > 1 && p.endsWith(u'/')) {
            p.chop(1);
        }
        return p;
    }

    static QString parentOf(const QString &p) { return p.section(u'/', 0, -2); }

    void answer(QTcpSocket *s, int status, const QByteArray &body = {}, const QByteArray &type = "text/plain")
    {
        QByteArray head = "HTTP/1.1 " + QByteArray::number(status) + " X\r\nContent-Length: "
                          + QByteArray::number(body.size()) + "\r\nContent-Type: " + type + "\r\nConnection: close\r\n\r\n";
        s->write(head + body);
        s->disconnectFromHost();
    }

    void take(QTcpSocket *s)
    {
        QByteArray &in = m_in[s];
        in += s->readAll();
        const qsizetype end = in.indexOf("\r\n\r\n");
        if (end < 0) {
            return;
        }
        const QList<QByteArray> lines = in.left(end).split('\n');
        QHash<QByteArray, QByteArray> headers;
        for (qsizetype i = 1; i < lines.size(); ++i) {
            const QByteArray l = lines.at(i).trimmed();
            const qsizetype colon = l.indexOf(':');
            headers.insert(l.left(colon).toLower(), l.mid(colon + 1).trimmed());
        }
        const qsizetype length = headers.value("content-length").toLongLong();
        if (in.size() < end + 4 + length) {
            return;
        }
        const QByteArray body = in.mid(end + 4, length);
        const QList<QByteArray> first = lines.first().trimmed().split(' ');
        m_in.remove(s);
        ++requests;

        const QByteArray verb = first.at(0);
        const QString path = trimmed(QUrl::fromPercentEncoding(first.at(1)));
        if (headers.value("authorization") != auth) {
            answer(s, 401);
            return;
        }
        if (verb == "MKCOL") {
            if (folders.contains(path) || files.contains(path)) {
                answer(s, 405);
            } else if (!folders.contains(parentOf(path))) {
                answer(s, 409);
            } else {
                folders.insert(path);
                answer(s, 201);
            }
        } else if (verb == "PUT") {
            if (!folders.contains(parentOf(path))) {
                answer(s, 409);
                return;
            }
            const bool known = files.contains(path);
            files.insert(path, body);
            answer(s, known ? 204 : 201);
        } else if (verb == "GET") {
            files.contains(path) ? answer(s, 200, files.value(path), "application/octet-stream") : answer(s, 404);
        } else if (verb == "PROPFIND") {
            if (!folders.contains(path)) {
                answer(s, 404);
                return;
            }
            QByteArray xml = "<?xml version=\"1.0\"?><d:multistatus xmlns:d=\"DAV:\">";
            auto item = [&](const QString &p, bool folder, qint64 size) {
                xml += "<d:response><d:href>" + QUrl::toPercentEncoding(p, "/") + (folder ? "/" : "")
                       + "</d:href><d:propstat><d:prop><d:resourcetype>" + (folder ? "<d:collection/>" : "")
                       + "</d:resourcetype><d:getlastmodified>Mon, 02 Nov 2026 18:00:00 GMT</d:getlastmodified>"
                       + "<d:getcontentlength>" + QByteArray::number(size)
                       + "</d:getcontentlength></d:prop></d:propstat></d:response>";
            };
            item(path, true, 0);
            for (const QString &f : std::as_const(folders)) {
                if (parentOf(f) == path) {
                    item(f, true, 0);
                }
            }
            for (auto it = files.cbegin(); it != files.cend(); ++it) {
                if (parentOf(it.key()) == path) {
                    item(it.key(), false, it.value().size());
                }
            }
            answer(s, 207, xml + "</d:multistatus>", "application/xml");
        } else {
            answer(s, 405);
        }
    }
};

// A key store in memory, late as the real one. `answer` is what it says; with `held` it waits for
// release(), like a wallet waiting to be unlocked. It can also lie as the real ones do:
//   losesWrites   says Ok, stores nothing (KWallet: writePassword fails, QtKeychain reports no error)
//   readsEmpty    says Ok with no text (KWallet: readPassword fails)
//   hidesEntry    says NotFound though it has it (libsecret: unlocking refused; another backend)
class FakeSecretStore : public SecretStore
{
public:
    QHash<QString, QString> entries;
    Result answer = Result::Ok;
    bool held = false;
    bool losesWrites = false;
    bool readsEmpty = false;
    bool hidesEntry = false;
    int asked = 0;

    void read(const QString &key, QObject *context, ReadDone done) override
    {
        ++asked;
        respond(context, [this, key, done = std::move(done)] {
            if (readsEmpty) {
                done(Result::Ok, QString());
                return;
            }
            const bool hidden = hidesEntry || !entries.contains(key);
            const Result result = answer == Result::Ok && hidden ? Result::NotFound : answer;
            done(result, result == Result::Ok ? entries.value(key) : QString());
        });
    }

    void write(const QString &key, const QString &secret, QObject *context, WriteDone done) override
    {
        ++asked;
        respond(context, [this, key, secret, done = std::move(done)] {
            if (answer == Result::Ok && !losesWrites) {
                entries.insert(key, secret);
            }
            done(answer);
        });
    }

    void release()
    {
        held = false;
        for (const auto &answerNow : std::exchange(m_held, {})) {
            answerNow();
        }
    }

private:
    QList<std::function<void()>> m_held;

    void respond(QObject *context, std::function<void()> answerNow)
    {
        if (held) {
            m_held << [guard = QPointer(context), answerNow] {
                if (guard) {
                    answerNow();
                }
            };
            return;
        }
        QTimer::singleShot(0, context, answerNow);
    }
};

// The sync: two devices through one WebDAV folder, a conflict on both, and a wrong password.
class TstSync : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    struct Device {
        QString practice;
        QString courses;
        QString media;
        QString folder;
        std::shared_ptr<FakeSecretStore> store = std::make_shared<FakeSecretStore>();
    };

    Device device(const QString &name) const
    {
        const QString root = m_dir.filePath(name);
        QDir().mkpath(root);
        return {root + u"/mitori.sqlite"_s, root + u"/lehrgaenge.sqlite"_s, root + u"/medien"_s, root + u"/sync"_s};
    }

    static QString accountFile(const Device &d) { return d.folder + u"/konto.ini"_s; }

    // konto.ini as an older version left it: enabled, with the password in plain text.
    static void oldAccount(const Device &d, const QString &server, const QString &password)
    {
        QDir().mkpath(d.folder);
        QSettings s(accountFile(d), QSettings::IniFormat);
        s.setValue(u"konto/server"_s, server);
        s.setValue(u"konto/user"_s, u"ich"_s);
        s.setValue(u"konto/password"_s, password);
        s.setValue(u"konto/enabled"_s, true);
    }

    // An account whose password went to the store.
    static void saved(const Device &d, const QString &server)
    {
        Sync first(d.practice, d.courses, d.media, d.folder, d.store);
        first.setAccount(server, u"ich"_s, u"geheim"_s, u"X"_s);
        first.setEnabled(true);
        QVERIFY(QTest::qWaitFor([&] { return !d.store->entries.isEmpty() && fileHas(d, "passwordInStore"); }, 5000));
    }

    // While it lives no file can be written, as on a full disk: the account stays readable. Works
    // for root too (the CI's container), unlike file rights.
    class WriteBlock
    {
    public:
        WriteBlock()
        {
            signal(SIGXFSZ, SIG_IGN);
            getrlimit(RLIMIT_FSIZE, &m_before);
            rlimit none = m_before;
            none.rlim_cur = 0;
            setrlimit(RLIMIT_FSIZE, &none);
        }
        ~WriteBlock() { lift(); }
        void lift() { setrlimit(RLIMIT_FSIZE, &m_before); }

    private:
        rlimit m_before;
    };

    static bool fileHas(const Device &d, const QByteArray &text)
    {
        QFile file(accountFile(d));
        return file.open(QIODevice::ReadOnly) && file.readAll().contains(text);
    }

    static bool synced(Sync &sync)
    {
        if (!sync.syncNow()) {
            return false;
        }
        const auto waits = [&] { return sync.errorString().contains(u"Waiting"_s); };   // for the key store
        if (!QTest::qWaitFor([&] { return !sync.busy() && !waits(); }, 20000)) {
            return false;
        }
        return sync.errorString().isEmpty();
    }

    // The texts of the notes that are not deleted, sorted.
    static QStringList texts(const QString &practice)
    {
        PracticeLog log;
        log.open(practice);
        QStringList list;
        for (const PracticeLog::Note &n : log.allNotes()) {
            list << n.text;
        }
        list.sort();
        return list;
    }

    static QDateTime now()
    {
        QTest::qWait(5);        // a later millisecond than the last step
        return QDateTime::currentDateTimeUtc();
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
        QVERIFY(m_dir.isValid());
    }

    // The app password: for its owner only.
    void accountFileIsPrivate()
    {
        QTemporaryDir dir;
        auto store = std::make_shared<FakeSecretStore>();
        store->answer = SecretStore::Result::Failed;
        SyncAccount account(dir.path(), store);
        account.setAccount({QUrl(u"https://example.org/dav/"_s), u"user"_s, u"geheim"_s});
        QTRY_COMPARE(account.secret(), SyncAccount::Secret::File);
        const QFile::Permissions p = QFile::permissions(dir.filePath(u"konto.ini"_s));
        QVERIFY(p != QFile::Permissions());
        QVERIFY(!(p & (QFile::ReadGroup | QFile::ReadOther | QFile::WriteGroup | QFile::WriteOther)));
    }

    // Migration: the password of an older version moves to the store, the file is clean after.
    void passwordMoves()
    {
        const Device d = device(u"umzug"_s);
        oldAccount(d, u"https://example.org/dav/"_s, u"geheim"_s);
        d.store->held = true;
        {
            Sync sync(d.practice, d.courses, d.media, d.folder, d.store);

            // Not confirmed yet: still in the file, and usable.
            QTest::qWait(10);
            QVERIFY(fileHas(d, "geheim"));
            QVERIFY(sync.passwordInFile());

            d.store->release();
            QTRY_VERIFY(!sync.passwordInFile());
            QVERIFY(sync.hasPassword());
        }
        QVERIFY(!fileHas(d, "geheim"));
        QCOMPARE(d.store->entries.values(), QStringList{u"geheim"_s});

        // The next start reads it from the store: a sync goes through.
        FakeDav dav;
        QSettings(accountFile(d), QSettings::IniFormat).setValue(u"konto/server"_s, dav.url());
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QVERIFY2(synced(sync), qPrintable(sync.errorString()));
    }

    // Migration with a failing store: nothing lost, the file stays, the note shows, the sync works.
    // A later start with a working store moves it.
    void passwordStaysOnFailure()
    {
        FakeDav dav;
        const Device d = device(u"ohnespeicher"_s);
        oldAccount(d, dav.url(), u"geheim"_s);
        d.store->answer = SecretStore::Result::Failed;
        {
            Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
            QTRY_VERIFY(d.store->asked > 0);
            QTest::qWait(10);
            QVERIFY(sync.passwordInFile());
            QVERIFY(fileHas(d, "geheim"));
            QVERIFY2(synced(sync), qPrintable(sync.errorString()));
        }
        QVERIFY(fileHas(d, "geheim"));
        QVERIFY(d.store->entries.isEmpty());

        d.store->answer = SecretStore::Result::Ok;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(!sync.passwordInFile());
        QVERIFY(!fileHas(d, "geheim"));
        QCOMPARE(d.store->entries.values(), QStringList{u"geheim"_s});
    }

    // A new password goes to the store and never touches the file; a failing store: into the file.
    void newPassword()
    {
        const Device d = device(u"neu"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"erstes"_s, u"X"_s);
        QVERIFY(sync.hasPassword());
        QTRY_COMPARE(d.store->entries.values(), QStringList{u"erstes"_s});
        QTest::qWait(10);
        QVERIFY(!fileHas(d, "erstes"));
        QVERIFY(!sync.passwordInFile());

        d.store->answer = SecretStore::Result::Failed;
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"zweites"_s, u"X"_s);
        QTRY_VERIFY(sync.passwordInFile());
        QVERIFY(fileHas(d, "zweites"));
    }

    // Two passwords in a row while the store waits: the last one counts, in the store and in memory.
    void newPasswordWins()
    {
        const Device d = device(u"zweimal"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        d.store->held = true;
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"alt"_s, u"X"_s);
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"neu"_s, u"X"_s);
        d.store->release();
        QTRY_COMPARE(d.store->entries.values(), QStringList{u"neu"_s});
        QTRY_VERIFY(!sync.passwordInFile());
        QVERIFY(!fileHas(d, "neu"));
    }

    // The race on start: a sync asked for before the store answered waits, with a word, and runs
    // with the password afterwards; never with an empty one.
    void syncWaitsForStore()
    {
        FakeDav dav;
        const Device d = device(u"wartet"_s);
        {
            Sync first(d.practice, d.courses, d.media, d.folder, d.store);
            first.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
            first.setEnabled(true);
            QTRY_VERIFY(!d.store->entries.isEmpty());
            QTest::qWait(10);
        }

        d.store->held = true;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QVERIFY(sync.hasPassword());
        QVERIFY(sync.syncNow());
        QVERIFY(!sync.busy());
        QVERIFY(!sync.errorString().isEmpty());
        QTest::qWait(20);
        QCOMPARE(dav.requests, 0);

        d.store->release();
        QTRY_VERIFY(sync.lastSync().isValid());
        QVERIFY2(sync.errorString().isEmpty(), qPrintable(sync.errorString()));
        QVERIFY(dav.requests > 0);
    }

    // The store does not give it out (locked, refused): no request with an empty password, the
    // note shows, entering it again works.
    void storeUnreadable()
    {
        FakeDav dav;
        const Device d = device(u"gesperrt"_s);
        {
            Sync first(d.practice, d.courses, d.media, d.folder, d.store);
            first.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
            first.setEnabled(true);
            QTRY_VERIFY(!d.store->entries.isEmpty());
            QTest::qWait(10);
        }

        d.store->answer = SecretStore::Result::Failed;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(sync.passwordUnreadable());
        QVERIFY(sync.syncNow());
        QTRY_VERIFY(!sync.errorString().contains(u"Waiting"_s));
        QVERIFY2(sync.errorString().contains(u"key store"_s), qPrintable(sync.errorString()));
        QCOMPARE(dav.requests, 0);

        // Unlocked again: the same button finds it.
        d.store->answer = SecretStore::Result::Ok;
        QVERIFY2(synced(sync), qPrintable(sync.errorString()));
        QVERIFY(!sync.passwordUnreadable());
    }

    // The store says Ok and stored nothing: read back, the password stays in the file.
    void writeThatLies()
    {
        const Device d = device(u"schreiblug"_s);
        oldAccount(d, u"https://example.org/dav/"_s, u"geheim"_s);
        d.store->losesWrites = true;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(d.store->asked >= 2);
        QTest::qWait(10);
        QVERIFY(fileHas(d, "geheim"));
        QVERIFY(sync.passwordInFile());
    }

    // Read back as Ok with no text: the same, it is no proof.
    void readBackEmpty()
    {
        const Device d = device(u"leerlesen"_s);
        oldAccount(d, u"https://example.org/dav/"_s, u"geheim"_s);
        d.store->readsEmpty = true;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(d.store->asked >= 2);
        QTest::qWait(10);
        QVERIFY(fileHas(d, "geheim"));
        QVERIFY(sync.passwordInFile());
    }

    // A new password the store only pretends to take lands in the file.
    void newPasswordLost()
    {
        const Device d = device(u"neulug"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        d.store->losesWrites = true;
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"geheim"_s, u"X"_s);
        QTRY_VERIFY(sync.passwordInFile());
        QVERIFY(fileHas(d, "geheim"));
    }

    // Marked, read gives Ok with no text: unreadable, the mark stays, no empty login.
    void readEmptyOnStart()
    {
        FakeDav dav;
        const Device d = device(u"leerstart"_s);
        saved(d, dav.url());
        d.store->readsEmpty = true;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(sync.passwordUnreadable());
        QVERIFY(fileHas(d, "passwordInStore"));
        QCOMPARE(dav.requests, 0);
    }

    // NotFound can be a refused unlock or another backend: the mark stays, it shows as unreadable,
    // the next start asks again and finds it.
    void notFoundKeepsMark()
    {
        FakeDav dav;
        const Device d = device(u"versteckt"_s);
        saved(d, dav.url());
        d.store->hidesEntry = true;
        {
            Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
            QTRY_VERIFY(sync.passwordUnreadable());
            QVERIFY(sync.hasPassword());
            QVERIFY(!sync.passwordSaved());     // the field does not claim it
            QVERIFY(fileHas(d, "passwordInStore"));
        }

        d.store->hidesEntry = false;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QVERIFY2(synced(sync), qPrintable(sync.errorString()));
    }

    // Really gone: entering it anew solves it.
    void lostEntryEnteredAnew()
    {
        FakeDav dav;
        const Device d = device(u"verloren"_s);
        saved(d, dav.url());
        d.store->entries.clear();
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(sync.passwordUnreadable());
        sync.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
        QTRY_VERIFY(!sync.passwordUnreadable());
        QVERIFY2(synced(sync), qPrintable(sync.errorString()));
    }

    // Two data folders on one store (profiles, test runs): neither overwrites the other's password.
    void keyPerFolder()
    {
        QTemporaryDir one;
        QTemporaryDir two;
        auto store = std::make_shared<FakeSecretStore>();
        const QUrl server(u"https://example.org/dav/"_s);
        {
            SyncAccount a(one.path(), store);
            SyncAccount b(two.path(), store);
            a.setEnabled(SyncAccount::Toggle::On);
            a.setAccount({server, u"a"_s, u"PASS_A"_s});
            b.setAccount({server, u"b"_s, u"PASS_B"_s});
            QTRY_COMPARE(a.secret(), SyncAccount::Secret::KeyStore);
            QTRY_COMPARE(b.secret(), SyncAccount::Secret::KeyStore);
        }
        SyncAccount a(one.path(), store);
        a.loadSecret();
        QTRY_COMPARE(a.secret(), SyncAccount::Secret::KeyStore);
        QCOMPARE(a.account().password, u"PASS_A"_s);
    }

    // Sync off: no question to the store on start, whatever is saved. Switched on, it asks.
    void offAsksNothing()
    {
        FakeDav dav;
        const Device d = device(u"aus"_s);
        saved(d, dav.url());
        QSettings(accountFile(d), QSettings::IniFormat).setValue(u"konto/enabled"_s, false);
        const int before = d.store->asked;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTest::qWait(10);
        QCOMPARE(d.store->asked, before);
        QVERIFY(sync.hasPassword());

        sync.setEnabled(true);
        QVERIFY2(synced(sync), qPrintable(sync.errorString()));
        QVERIFY(d.store->asked > before);
    }

    // Sync off with a password still in the file: it stays, with the note, until sync is on.
    void offLeavesFile()
    {
        const Device d = device(u"ausdatei"_s);
        oldAccount(d, u"https://example.org/dav/"_s, u"geheim"_s);
        QSettings(accountFile(d), QSettings::IniFormat).setValue(u"konto/enabled"_s, false);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTest::qWait(10);
        QCOMPARE(d.store->asked, 0);
        QVERIFY(sync.passwordInFile());
        QVERIFY(fileHas(d, "geheim"));

        sync.setEnabled(true);
        QTRY_VERIFY(!sync.passwordInFile());
        QVERIFY(!fileHas(d, "geheim"));
    }

    // A refused store is asked again by the button, not by the timers (a prompt every 30 s).
    void timersDoNotAsk()
    {
        FakeDav dav;
        const Device d = device(u"zeitgeber"_s);
        saved(d, dav.url());
        d.store->answer = SecretStore::Result::Failed;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(sync.passwordUnreadable());

        const int before = d.store->asked;
        QVERIFY(QMetaObject::invokeMethod(&sync, "scheduled"));
        QTest::qWait(10);
        QCOMPARE(d.store->asked, before);

        QVERIFY(sync.syncNow());
        QTRY_VERIFY(d.store->asked > before);
    }

    // konto.ini cannot be rewritten: the note says what really is the case, not "no key store".
    void fileNotWritable()
    {
        const Device d = device(u"nichtschreibbar"_s);
        oldAccount(d, u"https://example.org/dav/"_s, u"geheim"_s);
        d.store->held = true;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QCOMPARE(sync.passwordNote(), Sync::PasswordNote::MovingToStore);
        WriteBlock block;
        d.store->release();
        QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::NotRemovedFromFile);
    }

    // P1 in the file, P2 in the store, the file cannot be rewritten. The next start would take P1
    // back (the file counts as newest), so the note asks for P2 again; that repairs it for good.
    void newerNotRecorded()
    {
        FakeDav dav;
        const Device d = device(u"neuerer"_s);
        oldAccount(d, dav.url(), u"alt"_s);
        d.store->answer = SecretStore::Result::Failed;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(d.store->asked > 0);
        QTest::qWait(10);

        d.store->answer = SecretStore::Result::Ok;
        d.store->held = true;
        sync.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
        WriteBlock block;
        d.store->release();
        QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::NotRecorded);
        QVERIFY(!sync.passwordSaved());
        QCOMPARE(d.store->entries.values(), QStringList{u"geheim"_s});

        block.lift();
        sync.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
        QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::NoNote);
        QTRY_VERIFY(!fileHas(d, "alt"));

        Sync next(d.practice, d.courses, d.media, d.folder, d.store);
        QVERIFY2(synced(next), qPrintable(next.errorString()));
    }

    // The same, restarted before "Sync now": P1 is back and replaces P2 in the store, as the note
    // warns; entering P2 still repairs it.
    void newerNotRecordedRestart()
    {
        FakeDav dav;
        const Device d = device(u"neuerer2"_s);
        oldAccount(d, dav.url(), u"alt"_s);
        d.store->answer = SecretStore::Result::Failed;
        {
            Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
            QTRY_VERIFY(d.store->asked > 0);
            QTest::qWait(10);

            d.store->answer = SecretStore::Result::Ok;
            d.store->held = true;
            sync.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
            WriteBlock block;
            d.store->release();
            QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::NotRecorded);
        }
        QVERIFY(fileHas(d, "alt"));

        Sync next(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_COMPARE(d.store->entries.values(), QStringList{u"alt"_s});
        QTRY_VERIFY(!fileHas(d, "alt"));
        d.store->entries.clear();
        next.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
        QTRY_COMPARE(next.passwordNote(), Sync::PasswordNote::NoNote);
        QTRY_VERIFY(!fileHas(d, "alt"));
        QVERIFY2(synced(next), qPrintable(next.errorString()));
    }

    // Sync off and on again is no restart: the newer password in memory stays, and with it the note.
    // With sync off the older one in the file still shows.
    void offOnKeepsNewer()
    {
        const Device d = device(u"ausan"_s);
        oldAccount(d, u"https://example.org/dav/"_s, u"alt"_s);
        d.store->answer = SecretStore::Result::Failed;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(d.store->asked > 0);
        QTest::qWait(10);

        d.store->answer = SecretStore::Result::Ok;
        d.store->held = true;
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"neu"_s, u"X"_s);
        {
            WriteBlock block;
            d.store->release();
            QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::NotRecorded);
        }

        sync.setEnabled(false);
        QVERIFY(sync.passwordInFile());
        sync.setEnabled(true);
        QTest::qWait(10);
        QCOMPARE(sync.passwordNote(), Sync::PasswordNote::NotRecorded);
        QCOMPARE(d.store->entries.values(), QStringList{u"neu"_s});
    }

    // A first password, the file cannot take the mark: the next start would not look in the store.
    void firstNotRecorded()
    {
        const Device d = device(u"erstes"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        d.store->held = true;
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"geheim"_s, u"X"_s);
        QVERIFY(!sync.passwordSaved());         // in memory only, until the store confirms
        WriteBlock block;
        d.store->release();
        QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::NotRecorded);
        QVERIFY(!sync.passwordInFile());
    }

    // The mark stands and no password lies in the file: the file already leads to the store, the
    // write it cannot do changes nothing.
    void markAlreadyStands()
    {
        FakeDav dav;
        const Device d = device(u"markesteht"_s);
        saved(d, dav.url());
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTRY_VERIFY(sync.passwordSaved());
        QTest::qWait(10);
        d.store->held = true;
        sync.setAccount(dav.url(), u"ich"_s, u"neu"_s, u"X"_s);
        WriteBlock block;
        d.store->release();
        QTRY_COMPARE(d.store->entries.values(), QStringList{u"neu"_s});
        QTest::qWait(10);
        QCOMPARE(sync.passwordNote(), Sync::PasswordNote::NoNote);
        QVERIFY(sync.passwordSaved());
    }

    // The store refuses, the file cannot be written, but it already holds this password: not gone.
    void fileStillHoldsIt()
    {
        const Device d = device(u"haeltnoch"_s);
        oldAccount(d, u"https://example.org/dav/"_s, u"geheim"_s);
        d.store->held = true;
        d.store->answer = SecretStore::Result::Failed;
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        WriteBlock block;
        d.store->release();
        QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::RefusedByStore);
        QVERIFY(sync.passwordSaved());
    }

    // Neither store nor file: "in memory only", no claim that it lies in a file.
    void nowhereToSave()
    {
        const Device d = device(u"nirgends"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        d.store->held = true;
        d.store->answer = SecretStore::Result::Failed;
        sync.setAccount(u"https://example.org/dav/"_s, u"ich"_s, u"geheim"_s, u"X"_s);
        WriteBlock block;
        d.store->release();
        QTRY_COMPARE(sync.passwordNote(), Sync::PasswordNote::NotSaved);
        QVERIFY(!sync.passwordInFile());
        QVERIFY(!sync.passwordSaved());

        // Sync on again keeps what only memory holds.
        block.lift();
        sync.setEnabled(true);
        QTest::qWait(10);
        QCOMPARE(sync.passwordNote(), Sync::PasswordNote::NotSaved);
    }

    // Nobody who has no password is bothered by a wallet's unlock prompt.
    void noAccountNoStore()
    {
        const Device d = device(u"niemand"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QTest::qWait(10);
        QVERIFY(!sync.hasPassword());
        QCOMPARE(d.store->asked, 0);
    }

    // What leaves the device holds no password, in any file of the archive, nor does its folder
    // on the server. (Backup and sync share BackupArchive, which reads the databases and media only.)
    void noPasswordInArchive()
    {
        FakeDav dav;
        const Device d = device(u"archiv"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        sync.setAccount(dav.url(), u"ich"_s, u"geheim"_s, u"X"_s);
        sync.setEnabled(true);
        {
            PracticeLog log;
            QVERIFY(log.open(d.practice));
            log.addNote(u"aikido/k5/a"_s, u"Hüfte"_s, now(), u"a"_s);
        }
        QVERIFY2(synced(sync), qPrintable(sync.errorString()));

        int archives = 0;
        for (auto it = dav.files.cbegin(); it != dav.files.cend(); ++it) {
            QVERIFY2(!it.value().contains("geheim"), qPrintable(it.key()));
            QBuffer buffer;
            buffer.setData(it.value());
            buffer.open(QIODevice::ReadOnly);
            ZipReader zip;
            if (!zip.open(&buffer)) {
                continue;
            }
            ++archives;
            for (const QString &name : zip.names()) {
                QByteArray data;
                QVERIFY(zip.read(name, &data));
                QVERIFY2(!data.contains("geheim"), qPrintable(name));
            }
        }
        QCOMPARE(archives, 1);
    }

    void twoDevices()
    {
        FakeDav dav;
        const Device a = device(u"telefon"_s);
        const Device b = device(u"tablet"_s);
        Sync phone(a.practice, a.courses, a.media, a.folder, a.store);
        Sync tablet(b.practice, b.courses, b.media, b.folder, b.store);
        for (Sync *s : {&phone, &tablet}) {
            s->setAccount(dav.url(), u"ich"_s, u"geheim"_s, s == &phone ? u"Telefon"_s : u"Tablet"_s);
            s->setEnabled(true);
        }

        // The phone's note with a photo reaches the tablet, the photo too.
        MediaStore phoneMedia(a.media);
        QVERIFY(phoneMedia.adopt(u"p.jpg"_s, "jpeg"));
        QString uid;
        {
            PracticeLog log;
            QVERIFY(log.open(a.practice));
            uid = log.addNote(u"aikido/k5/a"_s, u"Hüfte"_s, now(), u"a"_s, u"p.jpg"_s).uid;
        }
        QVERIFY2(synced(phone), qPrintable(phone.errorString()));
        QVERIFY(dav.files.contains(u"/dav/Mitori/medien/p.jpg"_s));
        QSignalSpy merged(&tablet, &Sync::merged);
        QVERIFY2(synced(tablet), qPrintable(tablet.errorString()));
        QCOMPARE(merged.count(), 1);
        QCOMPARE(texts(b.practice), QStringList{u"Hüfte"_s});
        QVERIFY(MediaStore(b.media).has(u"p.jpg"_s));
        QCOMPARE(tablet.devices().size(), 1);
        QCOMPARE(tablet.devices().first().toMap().value(u"name"_s).toString(), u"Telefon"_s);

        // Both change the note: the phone syncs first, the tablet gets a conflict and keeps its text.
        {
            PracticeLog log;
            QVERIFY(log.open(a.practice));
            QVERIFY(log.changeNote(uid, u"Telefon-Text"_s, now()));
        }
        {
            PracticeLog log;
            QVERIFY(log.open(b.practice));
            QVERIFY(log.changeNote(uid, u"Tablet-Text"_s, now()));
        }
        QVERIFY(synced(phone));
        QVERIFY(phone.conflicts().isEmpty());
        QVERIFY(synced(tablet));
        QCOMPARE(tablet.conflicts().size(), 1);
        const QVariantMap c = tablet.conflicts().first().toMap();
        QCOMPARE(c.value(u"mine"_s).toString(), u"Tablet-Text"_s);
        QCOMPARE(c.value(u"theirs"_s).toString(), u"Telefon-Text"_s);
        QCOMPARE(c.value(u"device"_s).toString(), u"Telefon"_s);
        QCOMPARE(c.value(u"kind"_s).toString(), u"note"_s);
        QCOMPARE(texts(b.practice), QStringList{u"Tablet-Text"_s});

        // Open, it stays on the next sync (not overwritten by the newer side).
        QVERIFY(synced(tablet));
        QCOMPARE(tablet.conflicts().size(), 1);
        QCOMPARE(texts(b.practice), QStringList{u"Tablet-Text"_s});

        // "Both": the tablet keeps its text and adds the phone's; both devices end alike.
        QVERIFY(tablet.resolve(0, Sync::Choice::Both));
        QVERIFY(tablet.conflicts().isEmpty());
        QVERIFY(synced(tablet));
        QVERIFY(synced(phone));
        const QStringList both{u"Tablet-Text"_s, u"Telefon-Text"_s};
        QCOMPARE(texts(b.practice), both);
        QCOMPARE(texts(a.practice), both);
        QVERIFY(phone.conflicts().isEmpty());
    }

    static void change(const QString &practice, const QString &uid, const QString &text)
    {
        PracticeLog log;
        QVERIFY(log.open(practice));
        QVERIFY(log.changeNote(uid, text, now()));
    }

    // The tablet changes offline and uploads after the phone's sync; then the phone changes the
    // same note without having seen it: a conflict, nothing lost.
    void lateUpload()
    {
        FakeDav dav;
        const Device a = device(u"spaet-a"_s);
        const Device b = device(u"spaet-b"_s);
        Sync phone(a.practice, a.courses, a.media, a.folder, a.store);
        Sync tablet(b.practice, b.courses, b.media, b.folder, b.store);
        for (Sync *s : {&phone, &tablet}) {
            s->setAccount(dav.url(), u"ich"_s, u"geheim"_s, s == &phone ? u"Telefon"_s : u"Tablet"_s);
            s->setEnabled(true);
        }
        QString uid;
        {
            PracticeLog log;
            QVERIFY(log.open(a.practice));
            uid = log.addNote(u"aikido/k5/a"_s, u"alt"_s, now(), u"a"_s).uid;
        }
        QVERIFY(synced(phone));
        QVERIFY(synced(tablet));

        change(b.practice, uid, u"Tablet-Text"_s);
        QVERIFY(synced(phone));
        QVERIFY(synced(tablet));
        change(a.practice, uid, u"Telefon-Text"_s);
        QVERIFY(synced(phone));
        QCOMPARE(phone.conflicts().size(), 1);
        QCOMPARE(texts(a.practice), QStringList{u"Telefon-Text"_s});
    }

    // Solving after a later edit of one's own keeps that edit.
    void resolveAfterEdit()
    {
        FakeDav dav;
        const Device a = device(u"nach-a"_s);
        const Device b = device(u"nach-b"_s);
        Sync phone(a.practice, a.courses, a.media, a.folder, a.store);
        Sync tablet(b.practice, b.courses, b.media, b.folder, b.store);
        for (Sync *s : {&phone, &tablet}) {
            s->setAccount(dav.url(), u"ich"_s, u"geheim"_s, s == &phone ? u"Telefon"_s : u"Tablet"_s);
            s->setEnabled(true);
        }
        QString uid;
        {
            PracticeLog log;
            QVERIFY(log.open(a.practice));
            uid = log.addNote(u"aikido/k5/a"_s, u"alt"_s, now(), u"a"_s).uid;
        }
        QVERIFY(synced(phone));
        QVERIFY(synced(tablet));
        change(a.practice, uid, u"Telefon-Text"_s);
        change(b.practice, uid, u"Tablet-Text"_s);
        QVERIFY(synced(phone));
        QVERIFY(synced(tablet));
        QCOMPARE(tablet.conflicts().size(), 1);

        change(b.practice, uid, u"Tablet-neu"_s);
        // The page shows the text as it is now, the one a choice keeps or drops.
        QCOMPARE(tablet.conflicts().first().toMap().value(u"mine"_s).toString(), u"Tablet-neu"_s);
        QVERIFY(tablet.resolve(0, Sync::Choice::Both));
        QCOMPARE(texts(b.practice), (QStringList{u"Tablet-neu"_s, u"Telefon-Text"_s}));
    }

    // The other side's text is the newer one; after "this device" the conflict does not return.
    void resolvedStays()
    {
        FakeDav dav;
        const Device a = device(u"bleibt-a"_s);
        const Device b = device(u"bleibt-b"_s);
        Sync phone(a.practice, a.courses, a.media, a.folder, a.store);
        Sync tablet(b.practice, b.courses, b.media, b.folder, b.store);
        for (Sync *s : {&phone, &tablet}) {
            s->setAccount(dav.url(), u"ich"_s, u"geheim"_s, s == &phone ? u"Telefon"_s : u"Tablet"_s);
            s->setEnabled(true);
        }
        QString uid;
        {
            PracticeLog log;
            QVERIFY(log.open(a.practice));
            uid = log.addNote(u"aikido/k5/a"_s, u"alt"_s, now(), u"a"_s).uid;
        }
        QVERIFY(synced(phone));
        QVERIFY(synced(tablet));
        change(b.practice, uid, u"Tablet-Text"_s);
        change(a.practice, uid, u"Telefon-Text"_s);
        QVERIFY(synced(phone));
        QVERIFY(synced(tablet));
        QCOMPARE(tablet.conflicts().size(), 1);
        QVERIFY(tablet.resolve(0, Sync::Choice::Mine));
        QVERIFY(synced(tablet));
        QCOMPARE(tablet.conflicts().size(), 0);
        QVERIFY(synced(phone));
        QCOMPARE(phone.conflicts().size(), 0);
        QCOMPARE(texts(a.practice), QStringList{u"Tablet-Text"_s});
    }

    // A conflict on a course entry, solved with the other device's text.
    void entryConflict()
    {
        FakeDav dav;
        const Device a = device(u"e-a"_s);
        const Device b = device(u"e-b"_s);
        Sync one(a.practice, a.courses, a.media, a.folder, a.store);
        Sync two(b.practice, b.courses, b.media, b.folder, b.store);
        for (Sync *s : {&one, &two}) {
            s->setAccount(dav.url(), u"ich"_s, u"geheim"_s, s == &one ? u"Eins"_s : u"Zwei"_s);
            s->setEnabled(true);
        }
        QString entry;
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            CourseBook::Course c;
            c.title = u"Eigener Lehrgang"_s;
            c.first = c.last = QDate(2026, 11, 7);
            const QString course = book.addCourse(c, now(), u"a"_s).uid;
            entry = book.addEntry(course, u"alt"_s, now(), u"a"_s).uid;
        }
        QVERIFY(synced(one));
        QVERIFY(synced(two));
        for (const auto &[path, text] : {std::pair{a.courses, u"eins"_s}, std::pair{b.courses, u"zwei"_s}}) {
            CourseBook book;
            QVERIFY(book.open(path));
            QVERIFY(book.changeEntry(entry, text, now()));
        }
        QVERIFY(synced(one));
        QVERIFY(synced(two));
        QCOMPARE(two.conflicts().size(), 1);
        QCOMPARE(two.conflicts().first().toMap().value(u"kind"_s).toString(), u"entry"_s);
        QVERIFY(two.resolve(0, Sync::Choice::Theirs));
        QVERIFY(synced(two));
        QVERIFY(synced(one));
        for (const QString &path : {a.courses, b.courses}) {
            CourseBook book;
            QVERIFY(book.open(path));
            QCOMPARE(book.allEntries().size(), 1);
            QCOMPARE(book.allEntries().first().text, u"eins"_s);
        }
    }

    // Links travel with the sync: made on one device, removed on the other, gone with the entry.
    void linksTravel()
    {
        FakeDav dav;
        const Device a = device(u"l-a"_s);
        const Device b = device(u"l-b"_s);
        Sync one(a.practice, a.courses, a.media, a.folder);
        Sync two(b.practice, b.courses, b.media, b.folder);
        for (Sync *s : {&one, &two}) {
            s->setAccount(dav.url(), u"ich"_s, u"geheim"_s, s == &one ? u"Eins"_s : u"Zwei"_s);
            s->setEnabled(true);
        }
        const QString tech = u"aikido/t1"_s;
        const auto links = [&](const Device &d) {
            CourseBook book;
            return book.open(d.courses) ? book.linkedEntries(tech).size() : -1;
        };
        QString entry;
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            CourseBook::Course c;
            c.title = u"Eigener Lehrgang"_s;
            c.first = c.last = QDate(2026, 11, 7);
            entry = book.addEntry(book.addCourse(c, now(), u"a"_s).uid, u"Ikkyo"_s, now(), u"a"_s).uid;
            QVERIFY(book.addTag(entry, tech, now(), u"a"_s));
        }
        QVERIFY(synced(one));
        QVERIFY(synced(two));
        QCOMPARE(links(b), 1);

        // A link changes nothing on the entry: no conflict when both edit it and link it.
        for (const auto &[path, device] : {std::pair{a.courses, u"a"_s}, std::pair{b.courses, u"b"_s}}) {
            CourseBook book;
            QVERIFY(book.open(path));
            QVERIFY(book.addTag(entry, u"aikido/t-"_s + device, now(), device));
        }
        QVERIFY(synced(one));
        QVERIFY(synced(two));
        QVERIFY(synced(one));
        QVERIFY(two.conflicts().isEmpty());
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            QCOMPARE(book.linkedEntries(u"aikido/t-b"_s).size(), 1);
        }
        {
            CourseBook book;
            QVERIFY(book.open(b.courses));
            QCOMPARE(book.linkedEntries(u"aikido/t-a"_s).size(), 1);
            QVERIFY(book.removeTag(entry, tech, now()));
        }
        QVERIFY(synced(two));
        QVERIFY(synced(one));
        QCOMPARE(links(a), 0);
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            QVERIFY(book.addTag(entry, tech, now(), u"a"_s));
        }
        QVERIFY(synced(one));
        QVERIFY(synced(two));
        QCOMPARE(links(b), 1);

        {
            CourseBook book;
            QVERIFY(book.open(b.courses));
            QVERIFY(book.deleteEntry(entry, now()));
        }
        QVERIFY(synced(two));
        QVERIFY(synced(one));
        QCOMPARE(links(a), 0);
        QCOMPARE(links(b), 0);
    }

    // A wrong app password: the sync fails with a reason, nothing is sent.
    void wrongPassword()
    {
        FakeDav dav;
        const Device d = device(u"falsch"_s);
        Sync sync(d.practice, d.courses, d.media, d.folder, d.store);
        QVERIFY(!sync.syncNow());               // no account yet
        sync.setAccount(dav.url(), u"ich"_s, u"falsch"_s, u"X"_s);
        sync.setEnabled(true);
        QVERIFY(sync.hasPassword());
        QVERIFY(!synced(sync));
        QVERIFY2(sync.errorString().contains(u"app password"_s), qPrintable(sync.errorString()));
        QVERIFY(dav.files.isEmpty());
        QVERIFY(!sync.lastSync().isValid());
    }
};

QTEST_GUILESS_MAIN(TstSync)
#include "tst_sync.moc"
