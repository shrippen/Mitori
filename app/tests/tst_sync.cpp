#include <QMap>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

#include "coursebook.h"
#include "mediastore.h"
#include "practicelog.h"
#include "sync.h"
#include "webdav.h"

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
    };

    Device device(const QString &name) const
    {
        const QString root = m_dir.filePath(name);
        QDir().mkpath(root);
        return {root + u"/mitori.sqlite"_s, root + u"/lehrgaenge.sqlite"_s, root + u"/medien"_s, root + u"/sync"_s};
    }

    static bool synced(Sync &sync)
    {
        if (!sync.syncNow()) {
            return false;
        }
        if (!QTest::qWaitFor([&] { return !sync.busy(); }, 20000)) {
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
        SyncAccount account(dir.path());
        account.setAccount({QUrl(u"https://example.org/dav/"_s), u"user"_s, u"geheim"_s});
        const QFile::Permissions p = QFile::permissions(dir.filePath(u"konto.ini"_s));
        QVERIFY(p != QFile::Permissions());
        QVERIFY(!(p & (QFile::ReadGroup | QFile::ReadOther | QFile::WriteGroup | QFile::WriteOther)));
    }

    void twoDevices()
    {
        FakeDav dav;
        const Device a = device(u"telefon"_s);
        const Device b = device(u"tablet"_s);
        Sync phone(a.practice, a.courses, a.media, a.folder);
        Sync tablet(b.practice, b.courses, b.media, b.folder);
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
        Sync phone(a.practice, a.courses, a.media, a.folder);
        Sync tablet(b.practice, b.courses, b.media, b.folder);
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
        Sync phone(a.practice, a.courses, a.media, a.folder);
        Sync tablet(b.practice, b.courses, b.media, b.folder);
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
        Sync phone(a.practice, a.courses, a.media, a.folder);
        Sync tablet(b.practice, b.courses, b.media, b.folder);
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
        Sync one(a.practice, a.courses, a.media, a.folder);
        Sync two(b.practice, b.courses, b.media, b.folder);
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
        Sync sync(d.practice, d.courses, d.media, d.folder);
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
