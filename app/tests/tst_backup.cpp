#include <QBuffer>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

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

        // Again: nothing more to do.
        const QVariantMap again = tabletBackup.inspect(QUrl::fromLocalFile(zip));
        QCOMPARE(again.value(u"notes"_s).toMap().value(u"added"_s).toInt(), 0);
        QCOMPARE(again.value(u"notes"_s).toMap().value(u"changed"_s).toInt(), 0);
        QCOMPARE(again.value(u"entries"_s).toMap().value(u"added"_s).toInt(), 0);
        QCOMPARE(again.value(u"media"_s).toInt(), 0);
        tabletBackup.discard();
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
