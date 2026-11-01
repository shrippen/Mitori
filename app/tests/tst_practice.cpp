#include <QFile>
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
        const QVariantList due = practice.due(candidates);
        QCOMPARE(due.size(), 2);
        QCOMPARE(due.first().toMap().value(QStringLiteral("id")).toString(), QStringLiteral("b"));
        QCOMPARE(due.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("B"));
        QCOMPARE(due.first().toMap().value(QStringLiteral("level")).toInt(), 0);
        practice.setIntervals({2, 4, 14, 35});
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
