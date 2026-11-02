#include <QTest>

#include "schedule.h"

using Schedule::Candidate;

class TstSchedule : public QObject
{
    Q_OBJECT

    static PracticeLog::Rating rated(const QString &id, int level, const QDate &day)
    {
        return {QStringLiteral("u-") + id, id, level, QDateTime(day, QTime(18, 0)).toUTC(), QStringLiteral("d")};
    }

    static QStringList ids(const QList<Schedule::Due> &due)
    {
        QStringList r;
        for (const auto &d : due) {
            r << d.id;
        }
        return r;
    }

private slots:
    void intervalsPerLevel()
    {
        const QDate today(2026, 11, 2);
        QHash<QString, PracticeLog::Rating> latest;
        latest.insert(QStringLiteral("seen1"), rated(QStringLiteral("seen1"), 1, today.addDays(-1)));     // 1 < 2
        latest.insert(QStringLiteral("seen2"), rated(QStringLiteral("seen2"), 1, today.addDays(-2)));     // due
        latest.insert(QStringLiteral("sure13"), rated(QStringLiteral("sure13"), 3, today.addDays(-13)));  // 13 < 14
        latest.insert(QStringLiteral("sure14"), rated(QStringLiteral("sure14"), 3, today.addDays(-14)));  // due
        latest.insert(QStringLiteral("ready40"), rated(QStringLiteral("ready40"), 4, today.addDays(-40))); // due
        latest.insert(QStringLiteral("today"), rated(QStringLiteral("today"), 1, today));                   // practised
        QList<Candidate> c;
        for (const QString &id : latest.keys()) {
            c << Candidate{id, false};
        }
        const auto due = Schedule::due(c, latest, {}, today, Schedule::defaultIntervals());
        // Lower level first, then longer ago.
        QCOMPARE(ids(due), (QStringList{QStringLiteral("seen2"), QStringLiteral("sure14"), QStringLiteral("ready40")}));
        QCOMPARE(due.first().level, 1);
        QCOMPARE(due.first().days, 2);
    }

    void newOnesFromTheCurrentGradeOnly()
    {
        const QDate today(2026, 11, 2);
        const QList<Candidate> c{{QStringLiteral("old-new"), false}, {QStringLiteral("a"), true},
                                 {QStringLiteral("b"), true}, {QStringLiteral("c"), true}};
        const auto due = Schedule::due(c, {}, {}, today, Schedule::defaultIntervals());
        QCOMPARE(ids(due), (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
        QCOMPARE(due.first().level, 0);
    }

    void ratedComeFirstAndAreCapped()
    {
        const QDate today(2026, 11, 2);
        QHash<QString, PracticeLog::Rating> latest;
        QList<Candidate> c;
        for (int i = 0; i < 12; ++i) {
            const QString id = QStringLiteral("r%1").arg(i);
            latest.insert(id, rated(id, 2, today.addDays(-10 - i)));
            c << Candidate{id, true};
        }
        c << Candidate{QStringLiteral("n"), true};
        const auto due = Schedule::due(c, latest, {}, today, Schedule::defaultIntervals());
        QCOMPARE(due.size(), 9);
        QCOMPARE(due.first().id, QStringLiteral("r11"));     // longest ago
        QCOMPARE(due.last().id, QStringLiteral("n"));
    }

    // What was practised today counts towards the day's limits: two new ones rated today leave
    // no room for more new ones, and rated ones practised today use up the eight.
    void todaysPracticeCounts()
    {
        const QDate today(2026, 11, 2);
        QHash<QString, PracticeLog::Rating> latest;
        QHash<QString, QDate> first;
        latest.insert(QStringLiteral("a"), rated(QStringLiteral("a"), 2, today));
        first.insert(QStringLiteral("a"), today);
        latest.insert(QStringLiteral("b"), rated(QStringLiteral("b"), 1, today));
        first.insert(QStringLiteral("b"), today);
        const QList<Candidate> fresh{{QStringLiteral("a"), true}, {QStringLiteral("b"), true},
                                     {QStringLiteral("c"), true}, {QStringLiteral("d"), true}};
        QVERIFY(Schedule::due(fresh, latest, first, today, Schedule::defaultIntervals()).isEmpty());

        QList<Candidate> old;
        for (int i = 0; i < 12; ++i) {
            const QString id = QStringLiteral("r%1").arg(i);
            // r0..r4 practised today (first rated long ago), r5..r11 due.
            latest.insert(id, rated(id, 2, i < 5 ? today : today.addDays(-10)));
            first.insert(id, today.addDays(-30));
            old << Candidate{id, false};
        }
        QCOMPARE(Schedule::due(old, latest, first, today, Schedule::defaultIntervals()).size(), 3);
    }

    void ownIntervals()
    {
        const QDate today(2026, 11, 2);
        QHash<QString, PracticeLog::Rating> latest;
        latest.insert(QStringLiteral("a"), rated(QStringLiteral("a"), 4, today.addDays(-3)));
        const QList<Candidate> c{{QStringLiteral("a"), true}};
        QVERIFY(Schedule::due(c, latest, {}, today, Schedule::defaultIntervals()).isEmpty());
        QCOMPARE(Schedule::due(c, latest, {}, today, {1, 1, 1, 3}).size(), 1);
    }
};

QTEST_GUILESS_MAIN(TstSchedule)
#include "tst_schedule.moc"
