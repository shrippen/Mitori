#include <QFile>
#include <QTest>

#include "icsreader.h"

using namespace Qt::StringLiterals;

// The course calendar's events from an iCalendar file (tests/data/lehrgaenge.ics, made up).
class TstIcs : public QObject
{
    Q_OBJECT

    static QList<IcsReader::Event> events()
    {
        QFile file(QStringLiteral(TEST_ICS));
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return IcsReader::read(file.readAll());
    }

private slots:
    // Events without a uid are left out; the order is the file's.
    void eventsWithUid()
    {
        const QList<IcsReader::Event> list = events();
        QCOMPARE(list.size(), 3);
        QCOMPARE(list.at(0).uid, u"aaa-1"_s);
        QCOMPARE(list.at(1).uid, u"bbb-2"_s);
        QCOMPARE(list.at(2).uid, u"ccc-3"_s);
    }

    // DTEND of a date is the day after the last one; a date-time ends on its own day; without
    // DTEND the event has one day.
    void days()
    {
        const QList<IcsReader::Event> list = events();
        QCOMPARE(list.at(0).first, QDate(2026, 6, 6));
        QCOMPARE(list.at(0).last, QDate(2026, 6, 7));
        QCOMPARE(list.at(1).first, QDate(2026, 4, 25));
        QCOMPARE(list.at(1).last, QDate(2026, 4, 25));
        QCOMPARE(list.at(2).last, QDate(2026, 10, 3));
    }

    // Folded lines joined, escapes resolved, the text and not the ALTREP's HTML, not the
    // reminder's description.
    void texts()
    {
        const IcsReader::Event e = events().at(0);
        QCOMPARE(e.title, u"Daitoryu Aiki Jujutsu"_s);
        QCOMPARE(e.place, u"Siebeldingen"_s);
        QVERIFY2(e.description.startsWith(u"mit M. D. Nakajima Shihan\n\nSamstag\n10.30-11.00 Meditation\n"_s),
                 qPrintable(e.description));
        QVERIFY(!e.description.contains(u"Mozilla"_s));
        QVERIFY(!e.description.contains(u"<div"_s));
        QCOMPARE(events().at(1).title, u"Aikido, Kyu-Lehrgang"_s);
    }

    // "Ausschreibung: URL": the URL as written (& and %2F unchanged); only http(s).
    void announcement()
    {
        const QList<IcsReader::Event> list = events();
        QCOMPARE(list.at(0).announcement, u"https://www.shinkiryu.de/a.php?x=1&y=%2Fz"_s);
        QVERIFY(list.at(1).announcement.isEmpty());
        QVERIFY(list.at(2).announcement.isEmpty());
    }

    void empty()
    {
        QVERIFY(IcsReader::read({}).isEmpty());
        QVERIFY(IcsReader::read("garbage\nno: calendar").isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstIcs)
#include "tst_ics.moc"
