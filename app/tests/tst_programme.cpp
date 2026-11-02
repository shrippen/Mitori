#include <QSettings>
#include <QStandardPaths>
#include <QTest>

#include "programme.h"
#include "searchkey.h"

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

    // Search across disciplines: every word must match; case, accents and hyphens do not matter.
    void searchFindsTechniques()
    {
        const QVariantList ikkyo = programme.search(QStringLiteral("ikkyo"));
        QVERIFY(ikkyo.size() > 5);
        const QVariantMap first = ikkyo.first().toMap();
        QCOMPARE(first.value(QStringLiteral("discipline")).toString(), QStringLiteral("aikido"));
        QVERIFY(!first.value(QStringLiteral("gradeName")).toString().isEmpty());
        QVERIFY(!first.value(QStringLiteral("disciplineName")).toString().isEmpty());
        QVERIFY(!first.value(QStringLiteral("groupName")).toString().isEmpty());
        // Two words narrow it down; the order of the words does not matter.
        const QVariantList narrow = programme.search(QStringLiteral("shomen IKKYO"));
        QVERIFY(!narrow.isEmpty() && narrow.size() < ikkyo.size());
        QCOMPARE(programme.search(QStringLiteral("ikkyo shomen")).size(), narrow.size());
        // Hyphens and accents: "shiho nage" finds Shiho-nage, "kote gaeshi" Kote-gaeshi.
        QVERIFY(!programme.search(QStringLiteral("shiho nage")).isEmpty());
        QVERIFY(!programme.search(QStringLiteral("KOTE-GAESHI")).isEmpty());
        // Japanese.
        QVERIFY(!programme.search(QStringLiteral("一教")).isEmpty());
        // Without the hyphen or spaces, and Japanese with or without okurigana, find the same.
        const QVariantList joined = programme.search(QStringLiteral("shihonage"));
        QVERIFY(!joined.isEmpty());
        QCOMPARE(joined.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Shiho-nage"));
        // While typing, a joined word may end inside a hyphenated name.
        QVERIFY(!programme.search(QStringLiteral("shihon")).isEmpty());
        QVERIFY(!programme.search(QStringLiteral("kiriotoshi")).isEmpty());
        QCOMPARE(programme.search(QStringLiteral("kiriotoshi"), 1000).size(),
                 programme.search(QStringLiteral("kiri otoshi"), 1000).size());
        // A hyphenated word also matches when its parts are in different fields, each from the
        // start of a word ("ai" not in Kaiten-nage: ai-hanmi finds no Gyaku-hanmi).
        QVERIFY(!programme.search(QStringLiteral("ikkyo-irimi")).isEmpty());
        QCOMPARE(programme.search(QStringLiteral("ikkyo-irimi"), 1000).size(),
                 programme.search(QStringLiteral("ikkyo irimi"), 1000).size());
        const QVariantList ai = programme.search(QStringLiteral("ai-hanmi"), 1000);
        QVERIFY(!ai.isEmpty());
        for (const QVariant &r : ai) {
            QVERIFY2(!searchKey(r.toMap().value(QStringLiteral("attackExtra")).toString()).contains(u"gyaku"),
                     qPrintable(r.toMap().value(QStringLiteral("title")).toString()));
        }
        // Other dashes separate as a hyphen does (en dash in "Ô-dachi 33–43", U+2010, minus).
        QCOMPARE(searchKey(QStringLiteral("Ô-dachi 33–43")), QStringLiteral("o dachi 33 43"));
        QVERIFY(!programme.search(QStringLiteral("dachi 33 43")).isEmpty());
        QCOMPARE(programme.search(QStringLiteral("ikkyo‐irimi"), 1000).size(),
                 programme.search(QStringLiteral("ikkyo-irimi"), 1000).size());
        QCOMPARE(programme.search(QStringLiteral("ikkyo−irimi"), 1000).size(),
                 programme.search(QStringLiteral("ikkyo-irimi"), 1000).size());
        // Kana keep their dakuten: が is not か.
        QCOMPARE(searchKey(QStringLiteral("ガ")), QStringLiteral("ガ"));
        // Full-width Latin from a Japanese keyboard folds.
        QVERIFY(!programme.search(QStringLiteral("ikkyo")).isEmpty());
        QCOMPARE(programme.search(QStringLiteral("ｉｋｋｙｏ"), 1000).size(), programme.search(QStringLiteral("ikkyo"), 1000).size());
        QVERIFY(!programme.search(QStringLiteral("切り落し")).isEmpty());
        QCOMPARE(programme.search(QStringLiteral("切り落とし"), 1000).size(),
                 programme.search(QStringLiteral("切り落し"), 1000).size());
        // The attack's addition and Japanese, and the group, are searched too; rows carry them.
        const QVariantList gyaku = programme.search(QStringLiteral("gyaku hanmi"));
        QVERIFY(!gyaku.isEmpty());
        QVERIFY(searchKey(gyaku.first().toMap().value(QStringLiteral("attackExtra")).toString()).contains(u"gyaku"));
        QVERIFY(!programme.search(QStringLiteral("ai-hanmi")).isEmpty());
        QVERIFY(!programme.search(QStringLiteral("片手取り")).isEmpty());
        QVERIFY(!programme.search(QStringLiteral("ukemi")).isEmpty());
        // No duplicates, a limit, nothing for blanks.
        QStringList seen;
        for (const QVariant &r : ikkyo) {
            const QString id = r.toMap().value(QStringLiteral("id")).toString();
            QVERIFY(!seen.contains(id));
            seen << id;
        }
        QCOMPARE(programme.search(QStringLiteral("ikkyo"), 3).size(), 3);
        QVERIFY(programme.search(QStringLiteral("   ")).isEmpty());
        QVERIFY(programme.search(QStringLiteral("zzzzqqq")).isEmpty());
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
