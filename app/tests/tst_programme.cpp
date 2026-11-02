#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
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

// A small discipline for the technique map: a kata without attack, one group title in kyu and dan
// grades, a variant of a combination in a later grade, an attack with and without addition, theory.
const char *const kMapCatalog = R"({"format": 1, "disziplinen": [{"id": "test", "kurzname": "Test",
  "grade": [{"id": "k5", "name": "5. Kyu"}, {"id": "k4", "name": "4. Kyu"}, {"id": "d1", "name": "1. Dan"}],
  "gruppen": [{"id": "kyu/kata", "titel": "Kata", "art": "ohne_angriff"},
              {"id": "kyu/tachi", "titel": "Tachi-waza", "jp": "立技", "art": "angriffe"},
              {"id": "dan/tachi", "titel": "Tachi-waza", "jp": "立技", "art": "angriffe"},
              {"id": "dan/theorie", "titel": "Theorie", "art": "text"}],
  "techniken": [
    {"id": "kamae", "gruppe": "kyu/kata", "grad": "k5", "angriff": null, "name": "Kamae"},
    {"id": "ikkyo-kd", "gruppe": "kyu/tachi", "grad": "k5", "name": "Ikkyo", "jp": "一教",
     "angriff": {"name": "Katate-dori", "zusatz": null, "jp": "片手取り"}},
    {"id": "irimi-kd", "gruppe": "kyu/tachi", "grad": "k5", "name": "Irimi-nage",
     "angriff": {"name": "Katate-dori", "zusatz": null}},
    {"id": "ikkyo-kd-ai", "gruppe": "kyu/tachi", "grad": "k5", "name": "Ikkyo",
     "angriff": {"name": "Katate-dori", "zusatz": "Ai-hanmi"}},
    {"id": "ikkyo-kd-ura", "gruppe": "kyu/tachi", "grad": "k4", "name": "Ikkyo", "zusatz": "ura",
     "angriff": {"name": "Katate-dori", "zusatz": null}},
    {"id": "kote-ts", "gruppe": "kyu/tachi", "grad": "k4", "name": "Kote-gaeshi",
     "angriff": {"name": "Tsuki", "zusatz": null}},
    {"id": "irimi-ts", "gruppe": "dan/tachi", "grad": "d1", "name": "Irimi-nage",
     "angriff": {"name": "Tsuki", "zusatz": null}},
    {"id": "essay", "gruppe": "dan/theorie", "grad": "d1", "name": "Essay",
     "angriff": {"name": "Schrift", "zusatz": null}}]}]})";

QVariantMap at(const QVariant &v, int i) { return v.toList().value(i).toMap(); }

// The cell of a map grid at a row and column.
QVariantMap cell(const QVariantMap &grid, int row, int column)
{
    return at(at(grid.value(QStringLiteral("rows")), row).value(QStringLiteral("cells")), column);
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
        // Apostrophes vanish (San'nin-gake as sannin), brackets and ・ separate.
        QVERIFY(!programme.search(QStringLiteral("sannin")).isEmpty());
        QVERIFY(!programme.search(QStringLiteral("rechterfuss")).isEmpty());
        QVERIFY(!programme.search(QStringLiteral("片手取り合半身")).isEmpty());
        // Separators: emoji and dashes as spaces, ´ as an apostrophe, a decimal point stays.
        QCOMPARE(searchKey(QStringLiteral("Kote gaeshi🙂―gut")), QStringLiteral("kote gaeshi gut"));
        QCOMPARE(searchKey(QStringLiteral("San´nin ⸺ 3,5 Min. am 12.03.")), QStringLiteral("sannin 3,5 min am 12.03"));
        // Variation selectors of emoji vanish and a keycap separates, so the emoji still ends a word;
        // a full-width apostrophe is an apostrophe.
        QCOMPARE(searchKey(QStringLiteral("❤️Sumi otoshi 1️⃣Kote")), QStringLiteral("sumi otoshi 1 kote"));
        QCOMPARE(searchKey(QStringLiteral("San＇nin")), QStringLiteral("sannin"));
        // Okurigana also go beside a kanji beyond the BMP.
        QVERIFY(SearchText(QStringLiteral("𠮟り方")).matches(searchWords(QStringLiteral("𠮟る方"))));
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

    // The technique map up to a grade: one grid per group title, rows techniques, columns attacks
    // (with addition) in order of appearance, a cell with every entry of its combination.
    void techniqueMapUpToAGrade()
    {
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("map.json")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(kMapCatalog);
        file.close();
        Catalog catalog;
        QVERIFY2(catalog.load(file.fileName()), qPrintable(catalog.errorString()));
        const Programme p(catalog);

        QVariantList map = p.techniqueMap(QStringLiteral("test"), QStringLiteral("k4"));
        QCOMPARE(map.size(), 1);                                // the kata has no attack
        QVariantMap grid = map.first().toMap();
        QCOMPARE(grid.value(QStringLiteral("kind")).toString(), QStringLiteral("matrix"));
        QCOMPARE(grid.value(QStringLiteral("title")).toString(), QStringLiteral("Tachi-waza"));
        QCOMPARE(grid.value(QStringLiteral("jp")).toString(), QStringLiteral("立技"));

        QStringList columns;
        for (const QVariant &c : grid.value(QStringLiteral("columns")).toList()) {
            columns << c.toMap().value(QStringLiteral("title")).toString() + QLatin1Char('/') + c.toMap().value(QStringLiteral("extra")).toString();
        }
        QCOMPARE(columns, (QStringList{QStringLiteral("Katate-dori/"), QStringLiteral("Katate-dori/Ai-hanmi"), QStringLiteral("Tsuki/")}));
        QStringList rows;
        for (const QVariant &r : grid.value(QStringLiteral("rows")).toList()) {
            rows << r.toMap().value(QStringLiteral("title")).toString();
            QCOMPARE(r.toMap().value(QStringLiteral("cells")).toList().size(), columns.size());
        }
        QCOMPARE(rows, (QStringList{QStringLiteral("Ikkyo"), QStringLiteral("Irimi-nage"), QStringLiteral("Kote-gaeshi")}));
        QCOMPARE(at(grid.value(QStringLiteral("rows")), 0).value(QStringLiteral("jp")).toString(), QStringLiteral("一教"));

        // Ikkyo against Katate-dori: k5, its variant ura from k4 in the same cell; the first opens.
        const QVariantMap ikkyo = cell(grid, 0, 0);
        QCOMPARE(ikkyo.value(QStringLiteral("ids")).toStringList(), (QStringList{QStringLiteral("ikkyo-kd"), QStringLiteral("ikkyo-kd-ura")}));
        QCOMPARE(ikkyo.value(QStringLiteral("gradeName")).toString(), QStringLiteral("5. Kyu"));
        QCOMPARE(ikkyo.value(QStringLiteral("entry")).toMap().value(QStringLiteral("id")).toString(), QStringLiteral("ikkyo-kd"));
        QCOMPARE(cell(grid, 0, 1).value(QStringLiteral("ids")).toStringList(), QStringList{QStringLiteral("ikkyo-kd-ai")});
        QVERIFY(cell(grid, 0, 2).isEmpty());
        QVERIFY(cell(grid, 1, 2).isEmpty());                    // Irimi-nage against Tsuki: 1. Dan
        QCOMPARE(cell(grid, 2, 2).value(QStringLiteral("gradeName")).toString(), QStringLiteral("4. Kyu"));

        // Up to 1. Dan: the dan group of the same title joins the grid; theory stays out.
        map = p.techniqueMap(QStringLiteral("test"), QStringLiteral("d1"));
        QCOMPARE(map.size(), 1);
        grid = map.first().toMap();
        QCOMPARE(grid.value(QStringLiteral("rows")).toList().size(), 3);
        QCOMPARE(cell(grid, 1, 2).value(QStringLiteral("ids")).toStringList(), QStringList{QStringLiteral("irimi-ts")});
        QCOMPARE(cell(grid, 1, 2).value(QStringLiteral("gradeName")).toString(), QStringLiteral("1. Dan"));

        // Only 5. Kyu: no Tsuki yet.
        grid = p.techniqueMap(QStringLiteral("test"), QStringLiteral("k5")).first().toMap();
        QCOMPARE(grid.value(QStringLiteral("columns")).toList().size(), 2);
        QCOMPARE(grid.value(QStringLiteral("rows")).toList().size(), 2);
    }

    // Shinki Toho: the Ô-dachi forms up to a grade, numbered, each with the grade it comes in.
    void techniqueMapShinkiToho()
    {
        const QVariantList map = programme.techniqueMap(QStringLiteral("shinkitoho"), QStringLiteral("chuden"));
        QCOMPARE(map.size(), 1);
        const QVariantMap grid = map.first().toMap();
        QCOMPARE(grid.value(QStringLiteral("kind")).toString(), QStringLiteral("forms"));
        QCOMPARE(grid.value(QStringLiteral("title")).toString(), QStringLiteral("Ô-dachi 1–21"));
        const QVariantList cells = grid.value(QStringLiteral("cells")).toList();
        QCOMPARE(cells.size(), 21);
        QCOMPARE(at(cells, 0).value(QStringLiteral("entry")).toMap().value(QStringLiteral("nr")).toInt(), 1);
        QCOMPARE(at(cells, 0).value(QStringLiteral("gradeName")).toString(), QStringLiteral("Shoden"));
        QCOMPARE(at(cells, 11).value(QStringLiteral("ids")).toStringList(), QStringList{QStringLiteral("shinkitoho/odachi/12")});
        QCOMPARE(at(cells, 11).value(QStringLiteral("gradeName")).toString(), QStringLiteral("Chûden"));
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
