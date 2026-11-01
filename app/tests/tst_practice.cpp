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
            practice.rate(QStringLiteral("c"), 9);
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

    void deviceIdStays()
    {
        const QString id = Practice::deviceId();
        QVERIFY(!id.isEmpty());
        QCOMPARE(Practice::deviceId(), id);
    }
};

QTEST_GUILESS_MAIN(TstPractice)
#include "tst_practice.moc"
