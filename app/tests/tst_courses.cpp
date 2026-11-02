#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "coursebook.h"
#include "courses.h"
#include "devicecalendar.h"
#include "icsreader.h"

using namespace Qt::StringLiterals;

namespace {

// A calendar with one past and two coming events, dates relative to today.
QByteArray calendar()
{
    const auto day = [](int offset) { return QDate::currentDate().addDays(offset).toString(u"yyyyMMdd"_s).toUtf8(); };
    return "BEGIN:VCALENDAR\r\nVERSION:2.0\r\n"
           "BEGIN:VEVENT\r\nUID:past\r\nSUMMARY:Vorbei\r\nDTSTART;VALUE=DATE:" + day(-20) + "\r\nDTEND;VALUE=DATE:" + day(-18) + "\r\nEND:VEVENT\r\n"
           "BEGIN:VEVENT\r\nUID:later\r\nSUMMARY:Daitoryu\r\nDTSTART;VALUE=DATE:" + day(30) + "\r\nDTEND;VALUE=DATE:" + day(32) + "\r\n"
           "LOCATION:Siebeldingen\r\nDESCRIPTION:mit M. D. Nakajima Shihan\\nSamstag\\nAusschreibung: https://example.org/a?b=1&c=%2F\r\nEND:VEVENT\r\n"
           "BEGIN:VEVENT\r\nUID:soon\r\nSUMMARY:Aikido\r\nDTSTART;VALUE=DATE:" + day(-1) + "\r\nDTEND;VALUE=DATE:" + day(2) + "\r\nEND:VEVENT\r\n"
           "END:VCALENDAR\r\n";
}

} // namespace

class TstCourses : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    QString write(const QString &name, const QByteArray &data)
    {
        QFile file(m_dir.filePath(name));
        if (!file.open(QIODevice::WriteOnly)) {
            return {};
        }
        file.write(data);
        return file.fileName();
    }

    // A service on its own files, the calendar loaded from a local file.
    std::unique_ptr<Courses> made(const QString &name)
    {
        const QUrl source = QUrl::fromLocalFile(write(name + u".ics"_s, calendar()));
        auto c = std::make_unique<Courses>(m_dir.filePath(name + u".sqlite"_s), source, m_dir.filePath(name + u"-cache.ics"_s));
        QSignalSpy spy(c.get(), &Courses::calendarChanged);
        c->reload();
        while (c->calendarState() == Courses::CalendarState::Loading && spy.wait(5000)) {
        }
        return c;
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
        QVERIFY(m_dir.isValid());
    }

    // Notebooks and entries stay across a new start; a deletion keeps the others.
    void bookKeepsNotebooks()
    {
        const QString path = m_dir.filePath(u"book.sqlite"_s);
        const QDateTime at = QDateTime::currentDateTimeUtc();
        QString kept;
        {
            CourseBook book;
            QVERIFY2(book.open(path), qPrintable(book.errorString()));
            CourseBook::Course c;
            c.title = u"Training mit Gastlehrer"_s;
            c.first = QDate(2026, 11, 7);
            c.last = QDate(2026, 11, 6);        // before the first day: one day
            const CourseBook::Course made = book.addCourse(c, at, u"dev"_s);
            QVERIFY2(!made.uid.isEmpty(), qPrintable(book.errorString()));
            QCOMPARE(made.last, made.first);
            QVERIFY(book.addCourse({}, at, u"dev"_s).uid.isEmpty());     // no title
            kept = book.addEntry(made.uid, u"  Hüfte zuerst  "_s, at, u"dev"_s).uid;
            const QString gone = book.addEntry(made.uid, u"weg"_s, at.addSecs(1), u"dev"_s).uid;
            QVERIFY(book.addEntry(made.uid, u"   "_s, at, u"dev"_s).uid.isEmpty());
            QVERIFY(book.deleteEntry(gone, at.addSecs(2)));
            QVERIFY(!book.deleteEntry(gone, at.addSecs(3)));            // already deleted
            QVERIFY(book.changeEntry(kept, u"Hüfte früher"_s, at.addSecs(4)));
        }
        CourseBook book;
        QVERIFY(book.open(path));
        const QList<CourseBook::Course> list = book.courses();
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.first().title, u"Training mit Gastlehrer"_s);
        const QList<CourseBook::Entry> entries = book.entries(list.first().uid);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().uid, kept);
        QCOMPARE(entries.first().text, u"Hüfte früher"_s);
        QVERIFY(entries.first().changed > entries.first().created);
        QCOMPARE(book.entryCounts().value(list.first().uid), 1);
        QVERIFY(book.deleteCourse(list.first().uid, at.addSecs(5)));
        QVERIFY(book.courses().isEmpty());
        // Its entries go with it, as marks (a later sync must not bring them back).
        QVERIFY(book.entries(list.first().uid).isEmpty());
        QVERIFY(book.entryCounts().isEmpty());
    }

    // A newer schema is not opened (it would be changed under a newer Mitori).
    void bookRefusesNewerSchema()
    {
        const QString path = m_dir.filePath(u"newer.sqlite"_s);
        {
            CourseBook book;
            QVERIFY(book.open(path));
        }
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"raise"_s);
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery(db).exec(u"PRAGMA user_version = %1"_s.arg(CourseBook::schemaVersion + 1));
            db.close();
        }
        QSqlDatabase::removeDatabase(u"raise"_s);
        CourseBook book;
        QVERIFY(!book.open(path));
        QVERIFY(book.errorString().contains(u"newer"_s));
    }

    // Coming events (the one going on counts), by first day; the teacher from "mit …".
    void upcomingFromTheCalendar()
    {
        const auto c = made(u"upcoming"_s);
        QCOMPARE(c->calendarState(), Courses::CalendarState::Ready);
        const QVariantList list = c->upcoming();
        QCOMPARE(list.size(), 2);
        const QVariantMap soon = list.at(0).toMap();
        const QVariantMap later = list.at(1).toMap();
        QCOMPARE(soon.value(u"uid"_s).toString(), u"soon"_s);
        QCOMPARE(later.value(u"teacher"_s).toString(), u"M. D. Nakajima Shihan"_s);
        QCOMPARE(later.value(u"announcement"_s).toString(), u"https://example.org/a?b=1&c=%2F"_s);
        QCOMPARE(later.value(u"last"_s).toString(), QDate::currentDate().addDays(31).toString(Qt::ISODate));
        QVERIFY(later.value(u"notebook"_s).toString().isEmpty());
    }

    // The calendar stays on the device: a new start shows it without network; a failed load
    // keeps it.
    void cacheOffline()
    {
        {
            const auto c = made(u"cache"_s);
            QCOMPARE(c->upcoming().size(), 2);
        }
        Courses offline(m_dir.filePath(u"cache.sqlite"_s), QUrl::fromLocalFile(m_dir.filePath(u"missing.ics"_s)),
                        m_dir.filePath(u"cache-cache.ics"_s));
        QCOMPARE(offline.calendarState(), Courses::CalendarState::Ready);
        QCOMPARE(offline.upcoming().size(), 2);
        QSignalSpy spy(&offline, &Courses::calendarChanged);
        offline.reload();
        QVERIFY(spy.wait(5000) || offline.calendarState() != Courses::CalendarState::Loading);
        while (offline.calendarState() == Courses::CalendarState::Loading) {
            QVERIFY(spy.wait(5000));
        }
        QCOMPARE(offline.calendarState(), Courses::CalendarState::Failed);
        QCOMPARE(offline.upcoming().size(), 2);
        // Fresh: Older does not load again.
        offline.reload(Courses::Reload::Older);
        QVERIFY(offline.calendarState() != Courses::CalendarState::Loading);
    }

    // A notebook made from an event keeps the event's data; asked again, the same notebook.
    void notebookFromAnEvent()
    {
        const auto c = made(u"notebook"_s);
        QSignalSpy changed(c.get(), &Courses::changed);
        const QString uid = c->notebookFor(u"later"_s);
        QVERIFY(!uid.isEmpty());
        QCOMPARE(c->notebookFor(u"later"_s), uid);
        QCOMPARE(changed.count(), 1);
        QVERIFY(c->notebookFor(u"no-such-event"_s).isEmpty());
        const QVariantMap n = c->notebook(uid);
        QCOMPARE(n.value(u"title"_s).toString(), u"Daitoryu"_s);
        QCOMPARE(n.value(u"place"_s).toString(), u"Siebeldingen"_s);
        QCOMPARE(n.value(u"teacher"_s).toString(), u"M. D. Nakajima Shihan"_s);
        QCOMPARE(n.value(u"calendarUid"_s).toString(), u"later"_s);

        QVERIFY(c->addEntry(uid, u"Kote-gaeshi: erst ausweichen"_s));
        QVERIFY(!c->addEntry(uid, u"  "_s));
        QCOMPARE(c->entries(uid).size(), 1);
        QCOMPARE(c->notebook(uid).value(u"entries"_s).toInt(), 1);
        const QVariantMap row = c->upcoming().at(1).toMap();
        QCOMPARE(row.value(u"notebook"_s).toString(), uid);
        QCOMPARE(row.value(u"entries"_s).toInt(), 1);
        const QString entry = c->entries(uid).first().toMap().value(u"uid"_s).toString();
        QVERIFY(c->changeEntry(entry, u"Kote-gaeshi: ausweichen, dann führen"_s));
        QCOMPARE(c->entries(uid).first().toMap().value(u"text"_s).toString(), u"Kote-gaeshi: ausweichen, dann führen"_s);
        QVERIFY(c->deleteEntry(entry));
        QVERIFY(c->entries(uid).isEmpty());
    }

    // A notebook without an event; deleting it leaves the calendar alone.
    void ownNotebook()
    {
        const auto c = made(u"own"_s);
        const QString uid = c->create(u"Training mit Gastlehrer"_s, u"2026-11-07"_s, u"2026-11-08"_s,
                                      u" Dojo Weimar "_s, u"Gast"_s);
        QVERIFY(!uid.isEmpty());
        QCOMPARE(c->notebooks().size(), 1);
        QCOMPARE(c->notebook(uid).value(u"place"_s).toString(), u"Dojo Weimar"_s);
        QVERIFY(c->notebook(uid).value(u"calendarUid"_s).toString().isEmpty());
        QCOMPARE(c->notebook(uid).value(u"last"_s).toString(), u"2026-11-08"_s);
        QVERIFY(c->create(u"Ohne Tag"_s, u"7.11.2026"_s, {}, {}, {}).isEmpty());   // not ISO
        QVERIFY(c->create(u""_s, u"2026-11-07"_s, {}, {}, {}).isEmpty());
        QVERIFY(!c->errorString().isEmpty());
        QVERIFY(c->remove(uid));
        QVERIFY(c->errorString().isEmpty());
        QVERIFY(c->notebooks().isEmpty());
        QCOMPARE(c->upcoming().size(), 2);
    }

    // The desktop's calendar gets a whole-day event; folded, escaped, the end exclusive. It
    // reads back as the same event.
    void deviceCalendarIcs()
    {
        const DeviceCalendar::Event e{u"u-1"_s, u"Aikido, Dan; Lehrgang"_s, QDate(2026, 3, 21), QDate(2026, 3, 22),
                                      u"Heidelberg"_s, u"mit der Prüfer-Kommission\nSamstag 09.30 Meditation, "_s + QString(60, QLatin1Char('x'))};
        const QByteArray ics = DeviceCalendar::ics(e);
        QVERIFY(ics.contains("DTSTART;VALUE=DATE:20260321\r\n"));
        QVERIFY(ics.contains("DTEND;VALUE=DATE:20260323\r\n"));
        QVERIFY(ics.contains("SUMMARY:Aikido\\, Dan\\; Lehrgang\r\n"));
        for (const QByteArray &line : ics.split('\n')) {
            QVERIFY2(line.size() <= 76, line.constData());       // 75 octets and the CR
        }
        const QList<IcsReader::Event> back = IcsReader::read(ics);
        QCOMPARE(back.size(), 1);
        QCOMPARE(back.first().title, e.title);
        QCOMPARE(back.first().last, e.last);
        QCOMPARE(back.first().description, e.description);
    }

    // A course of one's own goes to the device's calendar with the notebook's uid (RFC 5545 needs one).
    void ownCourseHasUid()
    {
        const auto c = made(u"uid"_s);
        const QString uid = c->create(u"Training"_s, u"2026-11-07"_s, {}, {}, {});
        QCOMPARE(c->deviceEvent(uid).uid, uid);
        const QString fromEvent = c->notebookFor(u"later"_s);
        QCOMPARE(c->deviceEvent(fromEvent).uid, u"later"_s);
        QVERIFY(DeviceCalendar::ics(c->deviceEvent(uid)).contains("UID:" + uid.toLatin1() + "\r\n"));
    }

    void teacher()
    {
        QCOMPARE(Courses::teacherOf(u"mit Petra und Bernd Hubl"_s), u"Petra und Bernd Hubl"_s);
        QCOMPARE(Courses::teacherOf(u"Mit  M. D. Nakajima Shihan\nSamstag"_s), u"M. D. Nakajima Shihan"_s);
        QVERIFY(Courses::teacherOf(u"Samstag\nmit X"_s).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstCourses)
#include "tst_courses.moc"
