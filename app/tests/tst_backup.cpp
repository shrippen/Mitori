#include <QBuffer>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include "backup.h"
#include "backuparchive.h"
#include "coursebook.h"
#include "mediastore.h"
#include "practicelog.h"
#include "zipreader.h"
#include "zipwriter.h"

using namespace Qt::StringLiterals;

// The backup: the ZIP reader, and two devices merged through a backup file (store and service).
class TstBackup : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    // One device's files: its databases, media folder and work folder.
    struct Device {
        QString practice;
        QString courses;
        QString media;
        QString work;
    };

    Device device(const QString &name) const
    {
        const QString root = m_dir.filePath(name);
        QDir().mkpath(root);
        return {root + u"/mitori.sqlite"_s, root + u"/lehrgaenge.sqlite"_s, root + u"/medien"_s, root + u"/work"_s};
    }

    static QDateTime at(int hour)
    {
        return QDateTime(QDate(2026, 11, 1), QTime(hour, 0), QTimeZone::UTC);
    }

    enum class Failure { DamagedMedia, MediaUnwritable, SecondDatabase, ArchiveGoneBad };

    static constexpr QByteArrayView kOnlyPhoneBytes = "nur-telefon-ton";

    // One row of each kind and two files on the phone; returns the note both will share.
    QString seedPhone(const Device &phone) const
    {
        MediaStore media(phone.media);
        media.adopt(u"beide.jpg"_s, "beide-bild");
        media.adopt(u"nur-telefon.m4a"_s, kOnlyPhoneBytes.toByteArray());
        PracticeLog log;
        log.open(phone.practice);
        log.add(u"aikido/k5/a"_s, 3, at(9), u"telefon"_s);
        const QString shared = log.addNote(u"aikido/k5/a"_s, u"Telefon"_s, at(9), u"telefon"_s, u"beide.jpg"_s).uid;
        log.addNote(u"aikido/k5/b"_s, {}, at(9), u"telefon"_s, u"nur-telefon.m4a"_s);
        PracticeLog::ExamRound round;
        round.at = at(9);
        round.discipline = u"aikido"_s;
        round.grade = u"k5"_s;
        round.count = 1;
        round.average = 3;
        round.device = u"telefon"_s;
        log.addRound(round, {});

        CourseBook book;
        book.open(phone.courses);
        CourseBook::Course c;
        c.title = u"Telefon-Kurs"_s;
        c.first = c.last = QDate(2026, 7, 4);
        const QString course = book.addCourse(c, at(9), u"telefon"_s).uid;
        book.addEntry(course, u"vom Telefon"_s, at(9), u"telefon"_s);
        CourseBook::Grade g{{}, u"aikido"_s, u"k5"_s, QDate(2026, 7, 4), u"Mainz"_s};
        book.addGrade(g, at(9), u"telefon"_s);
        book.setStart(u"aikido"_s, QDate(2024, 9, 1), at(9), u"telefon"_s);
        return shared;
    }

    // The tablet: all of the phone's rows, then its own of each kind, and its files.
    void seedTablet(const Device &phone, const Device &tablet, const QString &shared) const
    {
        QFile::copy(phone.practice, tablet.practice);
        QFile::copy(phone.courses, tablet.courses);
        MediaStore media(tablet.media);
        media.adopt(u"beide.jpg"_s, "beide-bild");
        media.adopt(u"tablet.jpg"_s, "tablet-bild");
        media.adopt(u"eintrag.jpg"_s, "eintrag-bild");
        media.adopt(u"ungenannt.jpg"_s, "ungenannt");
        PracticeLog log;
        log.open(tablet.practice);
        log.add(u"aikido/k5/z"_s, 4, at(10), u"tablet"_s);
        log.changeNote(shared, u"Tablet neuer"_s, at(11));
        log.addNote(u"aikido/k5/z"_s, u"nur Tablet"_s, at(10), u"tablet"_s, u"tablet.jpg"_s);
        PracticeLog::ExamRound round;
        round.at = at(10);
        round.discipline = u"aikido"_s;
        round.grade = u"k4"_s;
        round.count = 1;
        round.average = 4;
        round.device = u"tablet"_s;
        log.addRound(round, {});

        CourseBook book;
        book.open(tablet.courses);
        CourseBook::Course c;
        c.title = u"Tablet-Kurs"_s;
        c.first = c.last = QDate(2026, 8, 1);
        const QString course = book.addCourse(c, at(10), u"tablet"_s).uid;
        book.addEntry(course, u"vom Tablet"_s, at(10), u"tablet"_s, u"eintrag.jpg"_s);
        CourseBook::Grade g{{}, u"daitoryu"_s, u"k5"_s, QDate(2026, 8, 1), u"Bonn"_s};
        book.addGrade(g, at(10), u"tablet"_s);
        book.setStart(u"daitoryu"_s, QDate(2025, 1, 1), at(10), u"tablet"_s);
    }

    QString saved(const Device &d, const QString &name) const
    {
        Backup backup(d.practice, d.courses, d.media, d.work);
        const QString zip = m_dir.filePath(name);
        return backup.save(QUrl::fromLocalFile(zip)) ? zip : QString();
    }

    // Every row of every table a backup holds, and the media files: to compare whole devices.
    static QStringList state(const Device &d)
    {
        QStringList lines;
        const QString connection = u"state-"_s + QUuid::createUuid().toString(QUuid::WithoutBraces);
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, connection);
            for (const auto &[path, tables] : {std::pair{d.practice, PracticeLog::tables()},
                                               std::pair{d.courses, CourseBook::tables()}}) {
                db.setDatabaseName(path);
                db.open();
                QSqlQuery q(db);
                for (const TableMerge::Table &t : tables) {
                    q.exec(u"SELECT %1 FROM %2 ORDER BY %3"_s.arg(t.columns.join(u", "), t.name, t.columns.first()));
                    while (q.next()) {
                        QStringList row{u"row"_s, t.name};
                        for (int i = 0; i < t.columns.size(); ++i) {
                            row << q.value(i).toString();
                        }
                        lines << row.join(u' ');
                    }
                }
                q.finish();
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(connection);
        for (const QString &name : MediaStore(d.media).names()) {
            lines << u"file "_s + name;
        }
        return lines;
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
        QVERIFY(m_dir.isValid());
    }

    // What ZipWriter writes, ZipReader reads; a changed byte fails the CRC.
    void zipRoundTrip()
    {
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        ZipWriter writer(&buffer);
        QVERIFY(writer.add(u"a.txt"_s, "eins"));
        QVERIFY(writer.add(u"medien/ü.jpg"_s, QByteArray(5000, 'x')));
        QVERIFY(writer.add(u"leer"_s, {}));
        QVERIFY(writer.finish());

        ZipReader reader;
        QVERIFY2(reader.open(&buffer), qPrintable(reader.errorString()));
        QCOMPARE(reader.names(), (QStringList{u"a.txt"_s, u"medien/ü.jpg"_s, u"leer"_s}));
        QByteArray data;
        QVERIFY(reader.read(u"a.txt"_s, &data));
        QCOMPARE(data, "eins");
        QVERIFY(reader.read(u"medien/ü.jpg"_s, &data));
        QCOMPARE(data.size(), 5000);
        QVERIFY(reader.read(u"leer"_s, &data));
        QVERIFY(data.isEmpty());
        QVERIFY(!reader.read(u"fehlt"_s, &data));

        QByteArray raw = buffer.data();
        raw[raw.indexOf("eins")] = 'E';
        QBuffer changed(&raw);
        changed.open(QIODevice::ReadOnly);
        QVERIFY(reader.open(&changed));
        QVERIFY(!reader.read(u"a.txt"_s, &data));

        QBuffer junk;
        junk.setData(QByteArray(100, 'j'));
        junk.open(QIODevice::ReadOnly);
        QVERIFY(!reader.open(&junk));
    }

    // Telefon writes a backup; Tablet, with data of its own, reads it back: the preview counts and
    // changes nothing, the restore merges row by row.
    void mergeDevices()
    {
        const Device phone = device(u"telefon"_s);
        const Device tablet = device(u"tablet"_s);
        MediaStore phoneMedia(phone.media);
        MediaStore tabletMedia(tablet.media);
        QVERIFY(phoneMedia.adopt(u"foto-telefon.jpg"_s, "jpeg"));
        QVERIFY(tabletMedia.adopt(u"alt-tablet.jpg"_s, "alt"));

        QString shared;         // a note on both devices, newer on the phone
        QString tabletOnly;
        QString deleted;        // on both, deleted on the phone later
        QString olderOnPhone;   // on both, newer on the tablet
        {
            PracticeLog log;
            QVERIFY(log.open(phone.practice));
            QVERIFY(!log.add(u"aikido/k5/a"_s, 3, at(9), u"telefon"_s).uid.isEmpty());
            shared = log.addNote(u"aikido/k5/a"_s, u"Hüfte"_s, at(9), u"telefon"_s).uid;
            deleted = log.addNote(u"aikido/k5/b"_s, u"weg"_s, at(9), u"telefon"_s, u"alt-tablet.jpg"_s).uid;
            olderOnPhone = log.addNote(u"aikido/k5/c"_s, u"Telefon alt"_s, at(9), u"telefon"_s).uid;
        }
        // The tablet had the same notes once (an earlier backup); then each device went on.
        QVERIFY(QFile::copy(phone.practice, tablet.practice));
        {
            PracticeLog log;
            QVERIFY(log.open(phone.practice));
            QVERIFY(log.changeNote(shared, u"Hüfte tiefer"_s, at(11)));
            QVERIFY(log.deleteNote(deleted, at(11)));
            QVERIFY(!log.addNote(u"aikido/k5/d"_s, u"mit Foto"_s, at(11), u"telefon"_s, u"foto-telefon.jpg"_s).uid.isEmpty());
        }
        {
            PracticeLog log;
            QVERIFY(log.open(tablet.practice));
            QVERIFY(log.changeNote(shared, u"Tablet"_s, at(10)));
            QVERIFY(log.changeNote(olderOnPhone, u"Tablet neu"_s, at(12)));
            tabletOnly = log.addNote(u"aikido/k5/e"_s, u"nur Tablet"_s, at(10), u"tablet"_s).uid;
            QVERIFY(!log.add(u"aikido/k5/a"_s, 4, at(10), u"tablet"_s).uid.isEmpty());
        }

        // Notebooks: one calendar event on both devices (two notebooks), a grade on both.
        {
            CourseBook book;
            QVERIFY(book.open(phone.courses));
            CourseBook::Course c;
            c.calendarUid = u"ev-1"_s;
            c.title = u"Sommer"_s;
            c.first = c.last = QDate(2026, 7, 4);
            const QString uid = book.addCourse(c, at(8), u"telefon"_s).uid;
            QVERIFY(!book.addEntry(uid, u"vom Telefon"_s, at(9), u"telefon"_s).uid.isEmpty());
            CourseBook::Grade g{{}, u"aikido"_s, u"k5"_s, QDate(2026, 7, 4), u"Mainz"_s};
            QVERIFY(!book.addGrade(g, at(9), u"telefon"_s).uid.isEmpty());
            QVERIFY(book.setStart(u"shinkitoho"_s, QDate(2024, 9, 1), at(9), u"telefon"_s));
        }
        QString tabletCourse;
        {
            CourseBook book;
            QVERIFY(book.open(tablet.courses));
            CourseBook::Course c;
            c.calendarUid = u"ev-1"_s;
            c.title = u"Sommer"_s;
            c.first = c.last = QDate(2026, 7, 4);
            tabletCourse = book.addCourse(c, at(7), u"tablet"_s).uid;
            QVERIFY(!book.addEntry(tabletCourse, u"vom Tablet"_s, at(10), u"tablet"_s).uid.isEmpty());
            CourseBook::Grade g{{}, u"aikido"_s, u"k5"_s, QDate(2026, 7, 5), u"Bonn"_s};
            QVERIFY(!book.addGrade(g, at(12), u"tablet"_s).uid.isEmpty());
        }

        // The phone's backup.
        Backup phoneBackup(phone.practice, phone.courses, phone.media, phone.work);
        QVERIFY(phoneBackup.fileName().startsWith(u"mitori-sicherung-"_s));
        const QString zip = m_dir.filePath(phoneBackup.fileName());
        QVERIFY2(phoneBackup.save(QUrl::fromLocalFile(zip)), qPrintable(phoneBackup.errorString()));
        QVERIFY(!QFile::exists(phone.work + u"/kopie-mitori.sqlite"_s));

        // Preview: the counts, nothing changed yet.
        Backup tabletBackup(tablet.practice, tablet.courses, tablet.media, tablet.work);
        const QByteArray before = [&] {
            QFile f(tablet.practice);
            f.open(QIODevice::ReadOnly);
            return f.readAll();
        }();
        const QVariantMap summary = tabletBackup.inspect(QUrl::fromLocalFile(zip));
        QVERIFY2(!summary.isEmpty(), qPrintable(tabletBackup.errorString()));
        QCOMPARE(summary.value(u"ratings"_s).toMap().value(u"added"_s).toInt(), 0);   // the phone's one is there
        QCOMPARE(summary.value(u"notes"_s).toMap().value(u"added"_s).toInt(), 1);     // "mit Foto"
        QCOMPARE(summary.value(u"notes"_s).toMap().value(u"changed"_s).toInt(), 2);   // shared, deleted
        QCOMPARE(summary.value(u"notebooks"_s).toMap().value(u"added"_s).toInt(), 1);
        QCOMPARE(summary.value(u"entries"_s).toMap().value(u"added"_s).toInt(), 1);
        QCOMPARE(summary.value(u"grades"_s).toMap().value(u"added"_s).toInt(), 1);
        QCOMPARE(summary.value(u"media"_s).toInt(), 1);
        QCOMPARE(summary.value(u"starts"_s).toMap().value(u"added"_s).toInt(), 1);
        QVERIFY(summary.value(u"created"_s).toDateTime().isValid());
        QFile f(tablet.practice);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), before);
        f.close();
        QVERIFY(!tabletMedia.has(u"foto-telefon.jpg"_s));

        // Restore: newer wins, nothing of the tablet's own is lost, doubles join.
        QSignalSpy restored(&tabletBackup, &Backup::restored);
        QVERIFY2(tabletBackup.restore(), qPrintable(tabletBackup.errorString()));
        QCOMPARE(restored.count(), 1);
        QVERIFY(!QFile::exists(tablet.work + u"/eingelesen.zip"_s));
        QVERIFY(tabletMedia.has(u"foto-telefon.jpg"_s));
        QVERIFY(!tabletMedia.has(u"alt-tablet.jpg"_s));       // its note was deleted on the phone

        PracticeLog log;
        QVERIFY(log.open(tablet.practice));
        QHash<QString, QString> texts;
        for (const PracticeLog::Note &n : log.allNotes()) {
            texts.insert(n.uid, n.text);
        }
        QCOMPARE(texts.size(), 4);
        QCOMPARE(texts.value(shared), u"Hüfte tiefer"_s);
        QCOMPARE(texts.value(olderOnPhone), u"Tablet neu"_s);
        QCOMPARE(texts.value(tabletOnly), u"nur Tablet"_s);
        QVERIFY(!texts.contains(deleted));
        QCOMPARE(log.history(u"aikido/k5/a"_s).size(), 2);

        CourseBook book;
        QVERIFY(book.open(tablet.courses));
        const QList<CourseBook::Course> courses = book.courses();
        QCOMPARE(courses.size(), 1);
        QCOMPARE(courses.first().uid, tabletCourse);            // the older one stays
        QCOMPARE(book.entries(tabletCourse).size(), 2);
        const QList<CourseBook::Grade> grades = book.grades();
        QCOMPARE(grades.size(), 1);
        QCOMPARE(grades.first().place, u"Bonn"_s);              // the newer row
        QCOMPARE(book.starts().value(u"shinkitoho"_s), QDate(2024, 9, 1));

        // Again: nothing more to do.
        const QVariantMap again = tabletBackup.inspect(QUrl::fromLocalFile(zip));
        QCOMPARE(again.value(u"notes"_s).toMap().value(u"added"_s).toInt(), 0);
        QCOMPARE(again.value(u"notes"_s).toMap().value(u"changed"_s).toInt(), 0);
        QCOMPARE(again.value(u"entries"_s).toMap().value(u"added"_s).toInt(), 0);
        QCOMPARE(again.value(u"media"_s).toInt(), 0);
        tabletBackup.discard();
    }

    // Back and forth: A takes B's notebook of the same event into its own; B writes on in its
    // notebook, then reads A's backup. Every entry ends up in the one notebook left.
    void notebookBackAndForth()
    {
        const Device a = device(u"hin-a"_s);
        const Device b = device(u"hin-b"_s);
        CourseBook::Course c;
        c.calendarUid = u"ev-9"_s;
        c.title = u"Herbst"_s;
        c.first = c.last = QDate(2026, 10, 3);
        QString courseA;
        QString courseB;
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            courseA = book.addCourse(c, at(7), u"a"_s).uid;
        }
        {
            CourseBook book;
            QVERIFY(book.open(b.courses));
            courseB = book.addCourse(c, at(8), u"b"_s).uid;
            QVERIFY(!book.addEntry(courseB, u"e1"_s, at(9), u"b"_s).uid.isEmpty());
        }
        Backup backupA(a.practice, a.courses, a.media, a.work);
        Backup backupB(b.practice, b.courses, b.media, b.work);
        const QString fromB = m_dir.filePath(u"von-b.zip"_s);
        QVERIFY(backupB.save(QUrl::fromLocalFile(fromB)));
        QVERIFY(!backupA.inspect(QUrl::fromLocalFile(fromB)).isEmpty());
        QVERIFY(backupA.restore());
        {
            CourseBook book;
            QVERIFY(book.open(b.courses));
            QVERIFY(!book.addEntry(courseB, u"e2"_s, at(10), u"b"_s).uid.isEmpty());
        }
        const QString fromA = m_dir.filePath(u"von-a.zip"_s);
        QVERIFY(backupA.save(QUrl::fromLocalFile(fromA)));
        QVERIFY(!backupB.inspect(QUrl::fromLocalFile(fromA)).isEmpty());
        QVERIFY(backupB.restore());

        CourseBook book;
        QVERIFY(book.open(b.courses));
        QCOMPARE(book.courses().size(), 1);
        QStringList texts;
        for (const CourseBook::Entry &e : book.entries(courseA)) {
            texts << e.text;
        }
        QCOMPARE(texts, (QStringList{u"e1"_s, u"e2"_s}));
    }

    // A notebook deleted on A while B writes in it: an entry newer than the deletion brings the
    // notebook back; one older than it goes with it (and its photo from A's media folder).
    void entryInDeletedNotebook()
    {
        const Device a = device(u"weg-a"_s);
        const Device b = device(u"weg-b"_s);
        MediaStore mediaA(a.media);
        QVERIFY(mediaA.adopt(u"alt.jpg"_s, "jpeg"));
        CourseBook::Course c;
        c.calendarUid = u"ev-7"_s;
        c.title = u"Winter"_s;
        c.first = c.last = QDate(2026, 12, 5);
        QString course;
        QString old;
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            course = book.addCourse(c, at(7), u"a"_s).uid;
        }
        QVERIFY(QFile::copy(a.courses, b.courses));
        {
            CourseBook book;
            QVERIFY(book.open(b.courses));
            QVERIFY(book.deleteCourse(course, at(9)));
        }
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            old = book.addEntry(course, u"vor der Löschung"_s, at(8), u"a"_s, u"alt.jpg"_s).uid;
            QVERIFY(!old.isEmpty());
        }

        // B's deletion (9:00) is newer than A's entry (8:00): both go.
        Backup backupA(a.practice, a.courses, a.media, a.work);
        Backup backupB(b.practice, b.courses, b.media, b.work);
        const QString fromB = m_dir.filePath(u"weg-b.zip"_s);
        QVERIFY(backupB.save(QUrl::fromLocalFile(fromB)));
        QVERIFY(!backupA.inspect(QUrl::fromLocalFile(fromB)).isEmpty());
        QVERIFY(backupA.restore());
        {
            CourseBook book;
            QVERIFY(book.open(a.courses));
            QCOMPARE(book.courses().size(), 0);
            QCOMPARE(book.entries(course).size(), 0);
        }
        QVERIFY(!mediaA.has(u"alt.jpg"_s));

        // B writes after the deletion (10:00): the notebook comes back on A.
        {
            CourseBook book;
            QVERIFY(book.open(b.courses));
            QVERIFY(!book.addEntry(course, u"nach der Löschung"_s, at(10), u"b"_s).uid.isEmpty());
        }
        QVERIFY(backupB.save(QUrl::fromLocalFile(fromB)));
        QVERIFY(!backupA.inspect(QUrl::fromLocalFile(fromB)).isEmpty());
        QVERIFY(backupA.restore());
        CourseBook book;
        QVERIFY(book.open(a.courses));
        QCOMPARE(book.courses().size(), 1);
        QCOMPARE(book.entries(course).size(), 1);
        QCOMPARE(book.entries(course).first().text, u"nach der Löschung"_s);
    }

    // A photo whose note was deleted after the backup does not come back from it.
    void deletedMediaStays()
    {
        const Device d = device(u"medien-weg"_s);
        MediaStore media(d.media);
        QVERIFY(media.adopt(u"x.jpg"_s, "jpeg"));
        QString uid;
        {
            PracticeLog log;
            QVERIFY(log.open(d.practice));
            uid = log.addNote(u"aikido/k5/a"_s, {}, at(9), u"d"_s, u"x.jpg"_s).uid;
        }
        Backup backup(d.practice, d.courses, d.media, d.work);
        const QString zip = m_dir.filePath(u"medien-weg.zip"_s);
        QVERIFY(backup.save(QUrl::fromLocalFile(zip)));
        {
            PracticeLog log;
            QVERIFY(log.open(d.practice));
            QVERIFY(log.deleteNote(uid, at(10)));
        }
        QVERIFY(media.remove(u"x.jpg"_s));

        const QVariantMap summary = backup.inspect(QUrl::fromLocalFile(zip));
        QCOMPARE(summary.value(u"media"_s).toInt(), 0);
        QVERIFY(backup.restore());
        QVERIFY(!media.has(u"x.jpg"_s));
    }

    // Replace: the tablet ends with the phone's backup, its own data and files gone; the preview
    // counts it and changes nothing.
    void replaceTakesBackup()
    {
        const Device phone = device(u"ersetzen-telefon"_s);
        const Device tablet = device(u"ersetzen-tablet"_s);
        const QString shared = seedPhone(phone);
        seedTablet(phone, tablet, shared);
        const QString zip = saved(phone, u"ersetzen.zip"_s);

        Backup backup(tablet.practice, tablet.courses, tablet.media, tablet.work);
        const QStringList before = state(tablet);
        const QVariantMap s = backup.inspect(QUrl::fromLocalFile(zip));
        QVERIFY2(!s.isEmpty(), qPrintable(backup.errorString()));
        QCOMPARE(state(tablet), before);

        const auto number = [&s](const QString &table, const QString &key) {
            return s.value(table).toMap().value(key).toInt();
        };
        QCOMPARE(number(u"ratings"_s, u"holds"_s), 1);
        QCOMPARE(number(u"notes"_s, u"holds"_s), 2);
        QCOMPARE(number(u"rounds"_s, u"holds"_s), 1);
        QCOMPARE(number(u"notebooks"_s, u"holds"_s), 1);
        QCOMPARE(number(u"entries"_s, u"holds"_s), 1);
        QCOMPARE(number(u"grades"_s, u"holds"_s), 1);
        QCOMPARE(number(u"starts"_s, u"holds"_s), 1);
        QCOMPARE(s.value(u"mediaHolds"_s).toInt(), 2);
        QCOMPARE(number(u"ratings"_s, u"lost"_s), 1);
        QCOMPARE(number(u"notes"_s, u"lost"_s), 2);     // the tablet's own, and the shared one set back
        QCOMPARE(number(u"rounds"_s, u"lost"_s), 1);
        QCOMPARE(number(u"notebooks"_s, u"lost"_s), 1);
        QCOMPARE(number(u"entries"_s, u"lost"_s), 1);
        QCOMPARE(number(u"grades"_s, u"lost"_s), 1);
        QCOMPARE(number(u"starts"_s, u"lost"_s), 1);
        QCOMPARE(s.value(u"mediaLost"_s).toInt(), 2);

        QSignalSpy restored(&backup, &Backup::restored);
        QVERIFY2(backup.replace(), qPrintable(backup.errorString()));
        QCOMPARE(restored.count(), 1);
        QVERIFY(!QFile::exists(tablet.work + u"/eingelesen.zip"_s));

        // Row for row and file for file as the phone. A file no row named stays: it is no one's data.
        QCOMPARE(state(tablet).filter(u"row "_s), state(phone).filter(u"row "_s));
        QCOMPARE(MediaStore(tablet.media).names(), (QStringList{u"beide.jpg"_s, u"nur-telefon.m4a"_s, u"ungenannt.jpg"_s}));
        PracticeLog log;
        QVERIFY(log.open(tablet.practice));
        QCOMPARE(log.notes(u"aikido/k5/a"_s).first().text, u"Telefon"_s);
        QVERIFY(log.notes(u"aikido/k5/z"_s).isEmpty());
        QCOMPARE(log.history(u"aikido/k5/z"_s).size(), 0);
        QVERIFY(!backup.replace());      // the inspected backup is used up
    }

    // Each way a replace can fail, before and after the first file's rows are written: the device
    // stays as it was, nothing of the backup stays behind, no restored().
    void replaceFails_data()
    {
        QTest::addColumn<int>("failure");
        QTest::newRow("damaged photo") << int(Failure::DamagedMedia);
        QTest::newRow("disk refuses files") << int(Failure::MediaUnwritable);
        QTest::newRow("second database fails") << int(Failure::SecondDatabase);
        QTest::newRow("archive gone bad") << int(Failure::ArchiveGoneBad);
    }

    void replaceFails()
    {
        QFETCH(int, failure);
        const QString tag = QString::fromLatin1(QTest::currentDataTag()).replace(u' ', u'-');
        const Device phone = device(u"fehler-telefon-"_s + tag);
        const Device tablet = device(u"fehler-tablet-"_s + tag);
        const QString shared = seedPhone(phone);
        seedTablet(phone, tablet, shared);
        const QString zip = saved(phone, tag + u".zip"_s);

        if (failure == int(Failure::DamagedMedia)) {
            // Stored, not compressed: one byte of the last photo fails its CRC, only when read.
            QFile f(zip);
            QVERIFY(f.open(QIODevice::ReadWrite));
            QByteArray raw = f.readAll();
            const qsizetype at = raw.indexOf(kOnlyPhoneBytes);
            QVERIFY(at >= 0);
            raw[at] = 'X';
            f.seek(0);
            f.write(raw);
            f.close();

            // The tablet lacks the first photo: it comes in before the damaged one stops all, and must go again.
            QVERIFY(MediaStore(tablet.media).remove(u"beide.jpg"_s));
        }
        if (failure == int(Failure::SecondDatabase)) {
            // The courses' rows fail after the practice log's are written.
            QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, tag);
            db.setDatabaseName(tablet.courses);
            QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec(u"CREATE TRIGGER voll BEFORE INSERT ON grades BEGIN SELECT RAISE(ABORT, 'voll'); END"_s));
            q.finish();
            db.close();
        }
        if (failure == int(Failure::MediaUnwritable)) {
            // The media folder cannot be written: a file stands in its place.
            QVERIFY(QDir(tablet.media).removeRecursively());
            QFile f(tablet.media);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.close();
        }

        Backup backup(tablet.practice, tablet.courses, tablet.media, tablet.work);
        QVERIFY2(!backup.inspect(QUrl::fromLocalFile(zip)).isEmpty(), qPrintable(backup.errorString()));
        if (failure == int(Failure::ArchiveGoneBad)) {
            QFile f(tablet.work + u"/eingelesen.zip"_s);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("kein zip");
            f.close();
        }

        const QStringList before = state(tablet);
        QVERIFY(before.contains(u"file tablet.jpg"_s) || failure == int(Failure::MediaUnwritable));
        QSignalSpy restored(&backup, &Backup::restored);
        QVERIFY(!backup.replace());
        QVERIFY(!backup.errorString().isEmpty());
        QCOMPARE(restored.count(), 0);
        QCOMPARE(state(tablet), before);
    }

    // A name typed without the suffix: the open dialog's filter would never show the file again.
    void addsTheSuffix()
    {
        const Device d = device(u"endung"_s);
        Backup backup(d.practice, d.courses, d.media, d.work);
        QVERIFY(backup.save(QUrl::fromLocalFile(m_dir.filePath(u"sicherung"_s))));
        QVERIFY(QFile::exists(m_dir.filePath(u"sicherung.zip"_s)));
        QVERIFY(!QFile::exists(m_dir.filePath(u"sicherung"_s)));
        QCOMPARE(backup.savedTo(), m_dir.filePath(u"sicherung.zip"_s));
        QVERIFY(backup.save(QUrl::fromLocalFile(m_dir.filePath(u"b.ZIP"_s))));
        QVERIFY(QFile::exists(m_dir.filePath(u"b.ZIP"_s)));
    }

    // A file that is no backup, and one of a newer format, are refused with a reason.
    void refused()
    {
        const Device d = device(u"fremd"_s);
        Backup backup(d.practice, d.courses, d.media, d.work);
        const QString junk = m_dir.filePath(u"keine.zip"_s);
        QFile f(junk);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("kein zip");
        f.close();
        QVERIFY(backup.inspect(QUrl::fromLocalFile(junk)).isEmpty());
        QVERIFY(!backup.errorString().isEmpty());
        QVERIFY(!backup.restore());

        const QString newer = m_dir.filePath(u"neuer.zip"_s);
        QFile out(newer);
        QVERIFY(out.open(QIODevice::WriteOnly));
        ZipWriter zip(&out);
        QVERIFY(zip.add(u"manifest.json"_s, R"({"app": "Mitori", "format": 99, "version": "9.0.0"})"));
        QVERIFY(zip.add(BackupArchive::kPractice, {}));
        QVERIFY(zip.add(BackupArchive::kCourses, {}));
        QVERIFY(zip.finish());
        out.close();
        QVERIFY(backup.inspect(QUrl::fromLocalFile(newer)).isEmpty());
        QVERIFY2(backup.errorString().contains(u"9.0.0"_s), qPrintable(backup.errorString()));

        QVERIFY(!backup.save(QUrl::fromLocalFile(m_dir.filePath(u"no/such/dir/x.zip"_s))));
        QVERIFY(!backup.errorString().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstBackup)
#include "tst_backup.moc"
