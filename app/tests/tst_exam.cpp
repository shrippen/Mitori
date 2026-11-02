#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include "exam.h"

namespace {

QString field(const QVariant &point, const char *key)
{
    return point.toMap().value(QLatin1String(key)).toString();
}

QStringList ids(const QVariantList &points)
{
    QStringList result;
    for (const QVariant &p : points) {
        result << field(p, "id");
    }
    return result;
}

// What a point shows: the same technique under another id (an earlier grade asks it again).
QString content(const QVariant &point)
{
    QStringList parts;
    for (const char *key : {"group", "attack", "attackExtra", "title", "extra"}) {
        parts << field(point, key);
    }
    return parts.join(u'|');
}

const QString aikido = QStringLiteral("aikido");
const QString shinkiToho = QStringLiteral("shinkitoho");

} // namespace

class TstExam : public QObject
{
    Q_OBJECT

    static Catalog loaded()
    {
        Catalog c;
        c.load(QStringLiteral(MITORI_DATEN_JSON));
        return c;
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
    }

    // A grade's material: its practicable entries in programme order, with group and attack.
    void materialOfAGrade()
    {
        Exam exam(loaded(), 1);
        const QVariantList points = exam.material(aikido, QStringLiteral("k5"));
        QCOMPARE(points.size(), 20);
        for (const QVariant &p : points) {
            QVERIFY(!field(p, "group").isEmpty());
            QCOMPARE(field(p, "gradeName"), QStringLiteral("5. Kyu"));
            QCOMPARE(p.toMap().value(QStringLiteral("ids")).toStringList(), QStringList{field(p, "id")});
        }
        QVERIFY(std::any_of(points.cbegin(), points.cend(), [](const QVariant &p) { return !field(p, "attack").isEmpty(); }));
        QVERIFY(exam.material(aikido, QStringLiteral("nope")).isEmpty());
    }

    // Shinki Toho: Shoden asks the Kamae as one point (it sets every Kamae) and Ô-dachi 1–11,
    // Chûden only its forms 12–21; Suburi and Kiriotoshi are not asked.
    void shinkiTohoMaterial()
    {
        Exam exam(loaded(), 1);
        const QVariantList shoden = exam.material(shinkiToho, QStringLiteral("shoden"));
        QCOMPARE(shoden.size(), 12);
        QCOMPARE(field(shoden.first(), "id"), QString::fromLatin1(Exam::kKamaeId));
        QCOMPARE(field(shoden.first(), "jp"), QStringLiteral("構"));
        const QStringList kamae = shoden.first().toMap().value(QStringLiteral("ids")).toStringList();
        QCOMPARE(kamae.size(), 26);
        QCOMPARE(kamae.first(), QStringLiteral("shinkitoho/kamae/1"));
        QCOMPARE(field(shoden.at(1), "id"), QStringLiteral("shinkitoho/odachi/1"));
        QCOMPARE(field(shoden.last(), "id"), QStringLiteral("shinkitoho/odachi/11"));

        const QVariantList chuden = exam.material(shinkiToho, QStringLiteral("chuden"));
        QCOMPARE(chuden.size(), 10);
        QCOMPARE(field(chuden.first(), "id"), QStringLiteral("shinkitoho/odachi/12"));

        // Grades with material only, each with its number of points.
        const QVariantList grades = exam.grades(shinkiToho);
        QCOMPARE(grades.size(), 4);
        QCOMPARE(grades.first().toMap().value(QStringLiteral("count")).toInt(), 12);
        QVERIFY(!exam.hasEarlier(shinkiToho, QStringLiteral("shoden")));
        QVERIFY(exam.hasEarlier(shinkiToho, QStringLiteral("chuden")));
    }

    void countChoices()
    {
        Exam exam(loaded(), 1);
        QCOMPARE(exam.countChoices(20), (QVariantList{0, 8, 12, 15}));
        QCOMPARE(exam.countChoices(12), (QVariantList{0, 8}));
        QCOMPARE(exam.countChoices(8), QVariantList{0});
    }

    // Default: all points in the order of the exam programme, no samples.
    void drawAllInOrder()
    {
        Exam exam(loaded(), 1);
        const QVariantList round = exam.draw(aikido, QStringLiteral("k4"), 0, Exam::Order::Programme, Exam::Samples::None);
        QCOMPARE(ids(round), ids(exam.material(aikido, QStringLiteral("k4"))));
    }

    // A part of the grade keeps programme order; shuffled it is another order. The same seed
    // draws the same round.
    void drawSomeAndShuffled()
    {
        Exam exam(loaded(), 7);
        const QStringList all = ids(exam.material(aikido, QStringLiteral("k5")));
        const QStringList some = ids(exam.draw(aikido, QStringLiteral("k5"), 8, Exam::Order::Programme, Exam::Samples::None));
        QCOMPARE(some.size(), 8);
        for (int i = 1; i < some.size(); ++i) {
            QVERIFY(all.indexOf(some.at(i - 1)) < all.indexOf(some.at(i)));
        }

        const QStringList shuffled = ids(exam.draw(aikido, QStringLiteral("k5"), 0, Exam::Order::Shuffled, Exam::Samples::None));
        QCOMPARE(shuffled.size(), all.size());
        QVERIFY(shuffled != all);
        QStringList sorted = shuffled;
        std::sort(sorted.begin(), sorted.end());
        QStringList allSorted = all;
        std::sort(allSorted.begin(), allSorted.end());
        QCOMPARE(sorted, allSorted);

        Exam again(loaded(), 7);
        QCOMPARE(ids(again.draw(aikido, QStringLiteral("k5"), 8, Exam::Order::Programme, Exam::Samples::None)), some);
        // A count not below the material means all.
        QCOMPARE(ids(again.draw(aikido, QStringLiteral("k5"), 99, Exam::Order::Programme, Exam::Samples::None)), all);
    }

    // Samples: 2–5 from earlier grades, always first, marked with their grade; none for the first grade.
    void samplesFirst()
    {
        Exam exam(loaded(), 3);
        const QStringList earlier = ids(exam.material(aikido, QStringLiteral("k5")));
        const QStringList own = ids(exam.material(aikido, QStringLiteral("k4")));
        QSet<int> sizes;
        for (int run = 0; run < 40; ++run) {
            const QVariantList round = exam.draw(aikido, QStringLiteral("k4"), 0, Exam::Order::Programme, Exam::Samples::Earlier);
            const int n = int(round.size()) - int(own.size());
            QVERIFY2(n >= Exam::kMinSamples && n <= Exam::kMaxSamples, qPrintable(QString::number(n)));
            sizes.insert(n);
            for (int i = 0; i < round.size(); ++i) {
                const bool sample = round.at(i).toMap().value(QStringLiteral("sample")).toBool();
                QCOMPARE(sample, i < n);
                if (sample) {
                    QVERIFY(earlier.contains(field(round.at(i), "id")));
                    QCOMPARE(field(round.at(i), "gradeName"), QStringLiteral("5. Kyu"));
                }
            }
            QCOMPARE(ids(round).mid(n), own);
        }
        QCOMPARE(sizes.size(), Exam::kMaxSamples - Exam::kMinSamples + 1);

        const QVariantList first = exam.draw(aikido, QStringLiteral("k5"), 0, Exam::Order::Programme, Exam::Samples::Earlier);
        QCOMPARE(ids(first), earlier);
    }

    // No technique twice in a round: samples skip what the grade asks again (Daitoryu 3. Kyu
    // repeats Aiki-otoshi of the 5. Kyu, 1. Dan Aiki-nage of the 1. Kyu) and each other.
    void samplesNotRepeated_data()
    {
        QTest::addColumn<QString>("discipline");
        QTest::addColumn<QString>("grade");
        QTest::newRow("daitoryu k3") << QStringLiteral("daitoryu") << QStringLiteral("k3");
        QTest::newRow("daitoryu d1") << QStringLiteral("daitoryu") << QStringLiteral("d1");
        QTest::newRow("aikido d4") << aikido << QStringLiteral("d4");
    }

    void samplesNotRepeated()
    {
        QFETCH(QString, discipline);
        QFETCH(QString, grade);
        for (quint32 seed = 1; seed <= 60; ++seed) {
            Exam exam(loaded(), seed);
            const QVariantList round = exam.draw(discipline, grade, 0, Exam::Order::Programme, Exam::Samples::Earlier);
            QSet<QString> seen;
            for (const QVariant &p : round) {
                QVERIFY2(!seen.contains(content(p)), qPrintable(QStringLiteral("seed %1: %2 twice").arg(seed).arg(field(p, "id"))));
                seen.insert(content(p));
            }
        }
    }

    // The settings stay across instances (starts).
    void settingsKept()
    {
        {
            Exam exam(loaded(), 1);
            QCOMPARE(exam.count(), 0);
            QVERIFY(!exam.shuffled());
            QVERIFY(exam.samples());
            QSignalSpy spy(&exam, &Exam::settingsChanged);
            exam.setCount(12);
            exam.setShuffled(true);
            exam.setSamples(false);
            exam.setSamples(false);
            QCOMPARE(spy.count(), 3);
        }
        Exam exam(loaded(), 1);
        QCOMPARE(exam.count(), 12);
        QVERIFY(exam.shuffled());
        QVERIFY(!exam.samples());
        const QVariantList round = exam.draw(aikido, QStringLiteral("k4"));
        QCOMPARE(round.size(), 12);
        QVERIFY(std::none_of(round.cbegin(), round.cend(), [](const QVariant &p) { return p.toMap().value(QStringLiteral("sample")).toBool(); }));
        QSettings().clear();
    }
};

QTEST_GUILESS_MAIN(TstExam)
#include "tst_exam.moc"
