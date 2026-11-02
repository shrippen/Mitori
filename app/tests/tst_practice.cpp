#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "practice.h"
#include "practicelog.h"

class TstPractice : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
    }

    void logKeepsEveryRating()
    {
        QTemporaryDir dir;
        PracticeLog log;
        QVERIFY2(log.open(dir.filePath(QStringLiteral("log.sqlite"))), qPrintable(log.errorString()));
        const QDateTime t = QDateTime(QDate(2026, 11, 1), QTime(18, 30), QTimeZone::UTC);
        QVERIFY(!log.add(QStringLiteral("a"), 2, t, QStringLiteral("phone")).uid.isEmpty());
        QVERIFY(!log.add(QStringLiteral("a"), 3, t.addDays(1), QStringLiteral("tablet")).uid.isEmpty());
        QVERIFY(!log.add(QStringLiteral("b"), 1, t, QStringLiteral("phone")).uid.isEmpty());
        QVERIFY(log.add(QStringLiteral("b"), 5, t, QStringLiteral("phone")).uid.isEmpty());
        QVERIFY(log.add(QStringLiteral("b"), 0, t, QStringLiteral("phone")).uid.isEmpty());

        const auto latest = log.latest();
        QCOMPARE(latest.size(), 2);
        QCOMPARE(latest.value(QStringLiteral("a")).level, 3);
        QCOMPARE(latest.value(QStringLiteral("a")).device, QStringLiteral("tablet"));
        QCOMPARE(latest.value(QStringLiteral("a")).at, t.addDays(1));
        const auto history = log.history(QStringLiteral("a"));
        QCOMPARE(history.size(), 2);
        QCOMPARE(history.first().level, 2);
    }

    // The same time stamp twice: the later insert wins (two taps within a millisecond).
    void sameTimeLaterWins()
    {
        QTemporaryDir dir;
        PracticeLog log;
        QVERIFY(log.open(dir.filePath(QStringLiteral("log.sqlite"))));
        const QDateTime t = QDateTime::currentDateTimeUtc();
        log.add(QStringLiteral("a"), 2, t, QStringLiteral("d"));
        log.add(QStringLiteral("a"), 4, t, QStringLiteral("d"));
        QCOMPARE(log.latest().value(QStringLiteral("a")).level, 4);
    }

    void reopenKeepsData()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("log.sqlite"));
        {
            PracticeLog log;
            QVERIFY(log.open(path));
            log.add(QStringLiteral("a"), 3, QDateTime::currentDateTimeUtc(), QStringLiteral("d"));
        }
        PracticeLog again;
        QVERIFY(again.open(path));
        QCOMPARE(again.latest().value(QStringLiteral("a")).level, 3);
    }

    // A log written by a newer Mitori is not touched.
    void newerSchemaRefused()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("log.sqlite"));
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("newer"));
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery(db).exec(QStringLiteral("PRAGMA user_version = 99"));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("newer"));
        PracticeLog log;
        QVERIFY(!log.open(path));
        QVERIFY(log.errorString().contains(QStringLiteral("99")));
    }

    // A merged or edited log may carry other levels: they do not count (and cannot overrun counts()).
    void levelsOutsideOneToFourIgnored()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("log.sqlite"));
        {
            PracticeLog log;
            QVERIFY(log.open(path));
            log.add(QStringLiteral("a"), 2, QDateTime::currentDateTimeUtc().addSecs(-60), QStringLiteral("d"));
        }
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("edit"));
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("INSERT INTO ratings VALUES ('x', 'a', 5, %1, 'd'), ('y', 'b', -1, %1, 'd')")
                               .arg(QDateTime::currentMSecsSinceEpoch())));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("edit"));
        Practice practice(path);
        QCOMPARE(practice.level(QStringLiteral("a")), 2);
        QCOMPARE(practice.level(QStringLiteral("b")), 0);
        QCOMPARE(practice.counts({QStringLiteral("a"), QStringLiteral("b")}), (QVariantList{1, 0, 1, 0, 0}));
    }

    // A log that cannot be opened, or a rating that cannot be saved, says so.
    void errorsAreReported()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("log.sqlite"));
        {
            Practice practice(path);
            QSignalSpy errors(&practice, &Practice::errorChanged);
            QVERIFY(practice.rate(QStringLiteral("a"), 1));
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("break"));
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QVERIFY(QSqlQuery(db).exec(QStringLiteral("DROP TABLE ratings")));
            db.close();
            QSqlDatabase::removeDatabase(QStringLiteral("break"));
            QVERIFY(!practice.rate(QStringLiteral("a"), 2));
            QCOMPARE(errors.count(), 1);
            QVERIFY(!practice.errorString().isEmpty());
            QCOMPARE(practice.level(QStringLiteral("a")), 1);
        }
        QFile::remove(path);
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("newer2"));
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery(db).exec(QStringLiteral("PRAGMA user_version = 99"));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("newer2"));
        Practice newer(path);
        QVERIFY(newer.errorString().contains(QStringLiteral("99")));
        QVERIFY(!newer.rate(QStringLiteral("a"), 1));
    }

    void serviceRatesAndCounts()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("mitori.sqlite"));
        {
            Practice practice(path);
            QCOMPARE(practice.errorString(), QString());
            QSignalSpy spy(&practice, &Practice::changed);
            QCOMPARE(practice.level(QStringLiteral("a")), 0);
            QVERIFY(!practice.lastRated(QStringLiteral("a")).isValid());
            practice.rate(QStringLiteral("a"), 3);
            practice.rate(QStringLiteral("b"), 1);
            practice.rate(QStringLiteral("b"), 1);
            QVERIFY(!practice.rate(QStringLiteral("c"), 9));
            QCOMPARE(spy.count(), 3);
            QCOMPARE(practice.revision(), 3);
            QCOMPARE(practice.level(QStringLiteral("a")), 3);
            QVERIFY(practice.lastRated(QStringLiteral("a")).secsTo(QDateTime::currentDateTime()) < 5);
            const QVariantList n = practice.counts({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d")});
            QCOMPARE(n, (QVariantList{2, 1, 0, 1, 0}));
            QCOMPARE(practice.levelNames().size(), 5);
        }
        Practice again(path);
        QCOMPARE(again.level(QStringLiteral("b")), 1);
    }

    void dueAndIntervals()
    {
        QTemporaryDir dir;
        Practice practice(dir.filePath(QStringLiteral("mitori.sqlite")));
        QSignalSpy spy(&practice, &Practice::intervalsChanged);
        QCOMPARE(practice.intervals(), (QVariantList{2, 4, 14, 35}));
        practice.setIntervals({1, 0, 500, 7});
        QCOMPARE(spy.count(), 1);
        QCOMPARE(practice.intervals(), (QVariantList{1, 1, 365, 7}));
        practice.setIntervals({1, 2});
        QCOMPARE(spy.count(), 1);

        // Rated today: practised, not due; two new ones of the current grade.
        practice.rate(QStringLiteral("a"), 1);
        QVERIFY(practice.ratedToday(QStringLiteral("a")));
        QVERIFY(!practice.ratedToday(QStringLiteral("b")));
        const QVariantList candidates{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("a")}, {QStringLiteral("current"), true}},
            QVariantMap{{QStringLiteral("id"), QStringLiteral("b")}, {QStringLiteral("current"), true}, {QStringLiteral("title"), QStringLiteral("B")}},
            QVariantMap{{QStringLiteral("id"), QStringLiteral("c")}, {QStringLiteral("current"), false}},
            QVariantMap{{QStringLiteral("id"), QStringLiteral("d")}, {QStringLiteral("current"), true}}};
        // "a" was new and is practised today: one of the day's two new places is taken.
        const QVariantList due = practice.due(candidates);
        QCOMPARE(due.size(), 1);
        QCOMPARE(due.first().toMap().value(QStringLiteral("id")).toString(), QStringLiteral("b"));
        QCOMPARE(due.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("B"));
        QCOMPARE(due.first().toMap().value(QStringLiteral("level")).toInt(), 0);
        practice.setIntervals({2, 4, 14, 35});
    }

    // After a restart the log still knows what was new today (firstRated from the database).
    void newTodayAfterRestart()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("mitori.sqlite"));
        const QVariantList candidates{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("a")}, {QStringLiteral("current"), true}},
            QVariantMap{{QStringLiteral("id"), QStringLiteral("b")}, {QStringLiteral("current"), true}},
            QVariantMap{{QStringLiteral("id"), QStringLiteral("c")}, {QStringLiteral("current"), true}}};
        {
            Practice practice(path);
            practice.rate(QStringLiteral("a"), 1);
            practice.rate(QStringLiteral("b"), 2);
        }
        Practice again(path);
        QVERIFY(again.due(candidates).isEmpty());
    }

    void notesKeepTheirHistory()
    {
        QTemporaryDir dir;
        PracticeLog log;
        QVERIFY(log.open(dir.filePath(QStringLiteral("log.sqlite"))));
        const QDateTime t = QDateTime(QDate(2026, 11, 2), QTime(18, 0), QTimeZone::UTC);
        const auto a = log.addNote(QStringLiteral("e"), QStringLiteral("  Hüfte früher drehen  "), t, QStringLiteral("phone"));
        QVERIFY(!a.uid.isEmpty());
        QCOMPARE(a.text, QStringLiteral("Hüfte früher drehen"));
        QVERIFY(log.addNote(QStringLiteral("e"), QStringLiteral("   "), t, QStringLiteral("phone")).uid.isEmpty());
        const auto b = log.addNote(QStringLiteral("e"), QStringLiteral("Blick beim Partner"), t.addSecs(60), QStringLiteral("phone"));
        QVERIFY(log.changeNote(a.uid, QStringLiteral("Hüfte früher"), t.addSecs(120)));
        QVERIFY(!log.changeNote(a.uid, QStringLiteral(" "), t.addSecs(130)));
        QCOMPARE(log.notes(QStringLiteral("e")).size(), 2);
        QCOMPARE(log.notes(QStringLiteral("e")).first().text, QStringLiteral("Hüfte früher"));
        QCOMPARE(log.notes(QStringLiteral("e")).first().changed, t.addSecs(120));
        QCOMPARE(log.lastNotes().value(QStringLiteral("e")).uid, b.uid);
        QVERIFY(log.deleteNote(b.uid, t.addSecs(180)));
        QVERIFY(!log.deleteNote(b.uid, t.addSecs(190)));
        QVERIFY(!log.changeNote(b.uid, QStringLiteral("again"), t.addSecs(200)));
        QCOMPARE(log.notes(QStringLiteral("e")).size(), 1);
        QCOMPARE(log.lastNotes().value(QStringLiteral("e")).uid, a.uid);
    }

    // A log of schema 1 (ratings only) gets the notes table and keeps its ratings.
    void migratesFromSchemaOne()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("log.sqlite"));
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("v1"));
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("CREATE TABLE ratings (uid TEXT PRIMARY KEY, entry TEXT NOT NULL,"
                                          " level INTEGER NOT NULL, at INTEGER NOT NULL, device TEXT NOT NULL)")));
            QVERIFY(q.exec(QStringLiteral("INSERT INTO ratings VALUES ('x', 'e', 3, %1, 'd')").arg(QDateTime::currentMSecsSinceEpoch())));
            QVERIFY(q.exec(QStringLiteral("PRAGMA user_version = 1")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("v1"));
        Practice practice(path);
        QCOMPARE(practice.errorString(), QString());
        QCOMPARE(practice.level(QStringLiteral("e")), 3);
        QSignalSpy spy(&practice, &Practice::notesChanged);
        QVERIFY(practice.addNote(QStringLiteral("e"), QStringLiteral("Kreis größer")));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(practice.lastNote(QStringLiteral("e")).value(QStringLiteral("text")).toString(), QStringLiteral("Kreis größer"));
        const QString uid = practice.notes(QStringLiteral("e")).first().toMap().value(QStringLiteral("uid")).toString();
        QVERIFY(practice.changeNote(uid, QStringLiteral("Kreis viel größer")));
        QCOMPARE(practice.lastNote(QStringLiteral("e")).value(QStringLiteral("text")).toString(), QStringLiteral("Kreis viel größer"));
        QVERIFY(practice.deleteNote(uid));
        QVERIFY(practice.lastNote(QStringLiteral("e")).isEmpty());
        QCOMPARE(spy.count(), 3);
    }

    void searchNotes()
    {
        QTemporaryDir dir;
        Practice practice(dir.filePath(QStringLiteral("mitori.sqlite")));
        practice.addNote(QStringLiteral("a"), QStringLiteral("Hüfte früher drehen"));
        practice.addNote(QStringLiteral("b"), QStringLiteral("Blick beim Partner, Hüfte tief"));
        practice.addNote(QStringLiteral("c"), QStringLiteral("Kreis größer"));
        QCOMPARE(practice.searchNotes(QStringLiteral("hufte")).size(), 2);       // without the umlaut
        const QVariantList both = practice.searchNotes(QStringLiteral("HÜFTE blick"));
        QCOMPARE(both.size(), 1);
        QCOMPARE(both.first().toMap().value(QStringLiteral("entry")).toString(), QStringLiteral("b"));
        QCOMPARE(practice.searchNotes(QStringLiteral("hüfte")).first().toMap().value(QStringLiteral("entry")).toString(),
                 QStringLiteral("b"));                                         // newest first
        const QString uid = practice.notes(QStringLiteral("c")).first().toMap().value(QStringLiteral("uid")).toString();
        practice.deleteNote(uid);
        QVERIFY(practice.searchNotes(QStringLiteral("kreis")).isEmpty());
        practice.addNote(QStringLiteral("d"), QStringLiteral("Fuß nach außen"));
        QCOMPARE(practice.searchNotes(QStringLiteral("fuss aussen")).size(), 1);   // ß as ss
        practice.addNote(QStringLiteral("e"), QStringLiteral("Shiho-nage: Arm über den Kopf"));
        QCOMPARE(practice.searchNotes(QStringLiteral("shihonage")).size(), 1);    // the hyphen does not matter
        // Joined across a space only from the start of a word: "rein" is not in "nur ein".
        practice.addNote(QStringLiteral("f"), QStringLiteral("Fuß nur ein wenig drehen"));
        QVERIFY(practice.searchNotes(QStringLiteral("rein")).isEmpty());
        QCOMPARE(practice.searchNotes(QStringLiteral("nurein")).size(), 1);
        // Across a space, the match also ends where a word ends: "mitte" is not in "mit Tenkan".
        practice.addNote(QStringLiteral("g"), QStringLiteral("Erst mit Tenkan ausweichen"));
        QVERIFY(practice.searchNotes(QStringLiteral("mitte")).isEmpty());
        QCOMPARE(practice.searchNotes(QStringLiteral("mittenkan")).size(), 1);
        // Punctuation separates words: a word after "(" or before "," still starts or ends there.
        practice.addNote(QStringLiteral("h"), QStringLiteral("Wie im Kurs (Shiho-nage) gezeigt. Irimi nage, dann „Kote gaeshi“."));
        QCOMPARE(practice.searchNotes(QStringLiteral("shihonage")).size(), 2);   // with "e"
        QCOMPARE(practice.searchNotes(QStringLiteral("iriminage")).size(), 1);
        QCOMPARE(practice.searchNotes(QStringLiteral("kotegaeshi")).size(), 1);
        // An emoji ends a word like any symbol, also outside the BMP.
        practice.addNote(QStringLiteral("j"), QStringLiteral("Sumi otoshi👍🏽 endlich"));
        QCOMPARE(practice.searchNotes(QStringLiteral("sumiotoshi")).size(), 1);
        // A soft hyphen is invisible.
        practice.addNote(QStringLiteral("i"), QStringLiteral("Mit\u00ADte halten"));
        QCOMPARE(practice.searchNotes(QStringLiteral("mitte")).size(), 1);
        // Capital ẞ as ß.
        QCOMPARE(practice.searchNotes(QStringLiteral("NUR EIN FUẞ")).size(), 1);
        QVERIFY(practice.searchNotes(QStringLiteral(" ")).isEmpty());
    }

    // All notes, newest first, each with its technique; deleted ones left out.
    void allNotes()
    {
        QTemporaryDir dir;
        Practice practice(dir.filePath(QStringLiteral("mitori.sqlite")));
        QVERIFY(practice.allNotes().isEmpty());
        practice.addNote(QStringLiteral("a"), QStringLiteral("erste"));
        QTest::qWait(5);
        practice.addNote(QStringLiteral("b"), QStringLiteral("zweite"));
        QTest::qWait(5);
        practice.addNote(QStringLiteral("a"), QStringLiteral("dritte"));
        QVariantList all = practice.allNotes();
        QCOMPARE(all.size(), 3);
        QCOMPARE(all.first().toMap().value(QStringLiteral("text")).toString(), QStringLiteral("dritte"));
        QCOMPARE(all.first().toMap().value(QStringLiteral("entry")).toString(), QStringLiteral("a"));
        QCOMPARE(all.last().toMap().value(QStringLiteral("text")).toString(), QStringLiteral("erste"));
        practice.deleteNote(all.at(1).toMap().value(QStringLiteral("uid")).toString());
        all = practice.allNotes();
        QCOMPARE(all.size(), 2);
        QVERIFY(all.first().toMap().value(QStringLiteral("created")).toDateTime().isValid());
    }

    void deviceIdStays()
    {
        const QString id = Practice::deviceId();
        QVERIFY(!id.isEmpty());
        QCOMPARE(Practice::deviceId(), id);
    }
};

QTEST_GUILESS_MAIN(TstPractice)
#include "tst_practice.moc"
