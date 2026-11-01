#include <QSettings>
#include <QStandardPaths>
#include <QTest>

#include "programme.h"

namespace {

int count(const QVariantList &rows, const QString &kind)
{
    int n = 0;
    for (const QVariant &r : rows) {
        n += r.toMap().value(QStringLiteral("kind")).toString() == kind;
    }
    return n;
}

QStringList ids(const QVariantList &rows)
{
    QStringList result;
    for (const QVariant &r : rows) {
        if (r.toMap().value(QStringLiteral("kind")) == QLatin1String("entry")) {
            result << r.toMap().value(QStringLiteral("id")).toString();
        }
    }
    return result;
}

} // namespace

class TstProgramme : public QObject
{
    Q_OBJECT

    static Catalog loaded()
    {
        Catalog c;
        c.load(QStringLiteral(MITORI_DATEN_JSON));
        return c;
    }
    Programme programme{loaded()};

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
    }

    // Today's practice draws from every grade up to the chosen one; only the chosen is current.
    void practiceEntriesUpToTheChosenGrade()
    {
        programme.setGrade(QStringLiteral("aikido"), QStringLiteral("k4"));
        const QVariantList rows = programme.practiceEntries(QStringLiteral("aikido"));
        const int k5 = count(programme.entries(QStringLiteral("aikido"), QStringLiteral("k5")), QStringLiteral("entry"));
        const int k4 = count(programme.entries(QStringLiteral("aikido"), QStringLiteral("k4")), QStringLiteral("entry"));
        QCOMPARE(rows.size(), k5 + k4);
        QVERIFY(!rows.first().toMap().value(QStringLiteral("current")).toBool());
        QVERIFY(rows.last().toMap().value(QStringLiteral("current")).toBool());
        QCOMPARE(rows.last().toMap().value(QStringLiteral("gradeName")).toString(), QStringLiteral("4. Kyu"));
        programme.setGrade(QStringLiteral("aikido"), QStringLiteral("k5"));
        QCOMPARE(programme.practiceEntries(QStringLiteral("aikido")).size(), k5);
    }

    void selectionIsKept()
    {
        QCOMPARE(programme.discipline(), QStringLiteral("aikido"));
        QCOMPARE(programme.grade(QStringLiteral("aikido")), QStringLiteral("k5"));
        programme.setDiscipline(QStringLiteral("daitoryu"));
        programme.setGrade(QStringLiteral("daitoryu"), QStringLiteral("k3"));
        Programme other{loaded()};
        QCOMPARE(other.discipline(), QStringLiteral("daitoryu"));
        QCOMPARE(other.grade(QStringLiteral("daitoryu")), QStringLiteral("k3"));
        programme.setGrade(QStringLiteral("daitoryu"), QStringLiteral("nonsense"));
        QCOMPARE(other.grade(QStringLiteral("daitoryu")), QStringLiteral("k5"));
        programme.setDiscipline(QStringLiteral("nonsense"));
        QCOMPARE(other.discipline(), QStringLiteral("daitoryu"));
    }

    void aikidoGradeStartsWithItsFirstGroup()
    {
        const QVariantList rows = programme.entries(QStringLiteral("aikido"), QStringLiteral("k5"));
        QCOMPARE(count(rows, QStringLiteral("entry")), 20);
        QCOMPARE(rows.first().toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("group"));
        QCOMPARE(rows.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Kata"));
    }

    void attackHeadingOnlyWhenTheAttackChanges()
    {
        const QVariantList rows = programme.entries(QStringLiteral("aikido"), QStringLiteral("k5"));
        QString last;
        for (const QVariant &r : rows) {
            const QVariantMap map = r.toMap();
            if (map.value(QStringLiteral("kind")) == QLatin1String("attack")) {
                const QString key = map.value(QStringLiteral("title")).toString() + map.value(QStringLiteral("extra")).toString();
                QVERIFY2(key != last, qPrintable(key));
                last = key;
            } else if (map.value(QStringLiteral("kind")) == QLatin1String("group")) {
                last.clear();
            }
        }
        QVERIFY(count(rows, QStringLiteral("attack")) > 0);
    }

    void everyEntryOnce()
    {
        for (const QVariant &d : programme.disciplines()) {
            const QString discipline = d.toMap().value(QStringLiteral("id")).toString();
            QStringList all;
            for (const QVariant &g : programme.grades(discipline)) {
                all << ids(programme.entries(discipline, g.toMap().value(QStringLiteral("id")).toString()));
            }
            QCOMPARE(QSet<QString>(all.begin(), all.end()).size(), all.size());
        }
    }

    void shinkiTohoGradesShowTheirOwnForms()
    {
        const QVariantList shoden = programme.entries(QStringLiteral("shinkitoho"), QStringLiteral("shoden"));
        QCOMPARE(count(shoden, QStringLiteral("entry")), 3 + 1 + 26 + 11);
        QCOMPARE(count(shoden, QStringLiteral("info")), 1);
        const QStringList chuden = ids(programme.entries(QStringLiteral("shinkitoho"), QStringLiteral("chuden")));
        QCOMPARE(chuden.size(), 10);
        QCOMPARE(chuden.first(), QStringLiteral("shinkitoho/odachi/12"));
    }

    void gradesWithoutMaterialAreLeftOut()
    {
        QStringList grades;
        for (const QVariant &g : programme.grades(QStringLiteral("shinkitoho"))) {
            grades << g.toMap().value(QStringLiteral("id")).toString();
            QVERIFY(g.toMap().value(QStringLiteral("count")).toInt() > 0);
        }
        QCOMPARE(grades, (QStringList{QStringLiteral("shoden"), QStringLiteral("chuden"), QStringLiteral("okuden"), QStringLiteral("junmenkyo")}));
    }

    void entryById()
    {
        const QVariantMap e = programme.entry(QStringLiteral("shinkitoho/odachi/12"));
        QCOMPARE(e.value(QStringLiteral("discipline")).toString(), QStringLiteral("shinkitoho"));
        QCOMPARE(e.value(QStringLiteral("grade")).toString(), QStringLiteral("chuden"));
        QVERIFY(programme.entry(QStringLiteral("nothing")).isEmpty());
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("mitori-test"));
    TstProgramme test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_programme.moc"
