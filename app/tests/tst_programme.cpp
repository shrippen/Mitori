#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>

#include "courses.h"
#include "practicegrade.h"
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
const char *const kMapCatalog = R"({"format": 1, "stand": "2026.11.0", "disziplinen": [{"id": "test", "kurzname": "Test",
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

    // A new Courses with an empty book, and no practice grade or old grade stored.
    std::unique_ptr<Courses> emptyCourses(const QTemporaryDir &dir)
    {
        QSettings settings;
        settings.remove(QStringLiteral("practiceGrade"));
        settings.remove(QStringLiteral("programme/grade"));
        return std::make_unique<Courses>(dir.filePath(QStringLiteral("book.sqlite")), QUrl(),
                                         dir.filePath(QStringLiteral("cal.ics")), loaded());
    }

    static bool pass(Courses &courses, const QString &discipline, const QString &grade)
    {
        return courses.addGrade(discipline, grade, QStringLiteral("2024-03-01"), QString());
    }

private slots:
    void initTestCase()
    {
        QSettings().clear();
    }

    // The programme's chips add Hon-Menkyo and Menkyo-Kaiden with what the catalog says of them,
    // not Hôshu; grades() and the other disciplines stay as they are.
    void gradeChipsShowGradesWithoutForms()
    {
        const QString toho = QStringLiteral("shinkitoho");
        const auto idsOf = [](const QVariantList &grades) {
            QStringList result;
            for (const QVariant &g : grades) {
                result << g.toMap().value(QStringLiteral("id")).toString();
            }
            return result;
        };

        const QVariantList chips = programme.gradeChips(toho);
        const QStringList withForms = idsOf(programme.grades(toho));
        const QStringList noForms{QStringLiteral("honmenkyo"), QStringLiteral("kaiden")};
        QCOMPARE(idsOf(chips), withForms + noForms);
        QVERIFY(!withForms.contains(QStringLiteral("honmenkyo")));
        QVERIFY(chips.first().toMap().value(QStringLiteral("count")).toInt() > 0);

        const QVariantMap honmenkyo = chips.at(withForms.size()).toMap();
        QCOMPARE(honmenkyo.value(QStringLiteral("count")).toInt(), 0);
        QCOMPARE(honmenkyo.value(QStringLiteral("name")).toString(), QStringLiteral("Hon-Menkyo"));
        QCOMPARE(honmenkyo.value(QStringLiteral("jp")).toString(), QStringLiteral("本免許"));
        QCOMPARE(honmenkyo.value(QStringLiteral("bedeutung")).toString(), QStringLiteral("Diplom"));
        QVERIFY(honmenkyo.value(QStringLiteral("inhalt")).toString().contains(QStringLiteral("Ko-dachi")));
        QCOMPARE(honmenkyo.value(QStringLiteral("abstand")).toString(), QStringLiteral("4 Jahre"));

        for (const QString &other : {QStringLiteral("aikido"), QStringLiteral("daitoryu")}) {
            QCOMPARE(programme.gradeChips(other), programme.grades(other));
        }
    }

    // Today's practice draws from every grade up to the given one; only that one is current.
    void practiceEntriesUpToTheGrade()
    {
        const QVariantList rows = programme.practiceEntries(QStringLiteral("aikido"), QStringLiteral("k4"));
        const int k5 = count(programme.entries(QStringLiteral("aikido"), QStringLiteral("k5")), QStringLiteral("entry"));
        const int k4 = count(programme.entries(QStringLiteral("aikido"), QStringLiteral("k4")), QStringLiteral("entry"));
        QCOMPARE(rows.size(), k5 + k4);
        QVERIFY(!rows.first().toMap().value(QStringLiteral("current")).toBool());
        QVERIFY(rows.last().toMap().value(QStringLiteral("current")).toBool());
        QCOMPARE(rows.last().toMap().value(QStringLiteral("gradeName")).toString(), QStringLiteral("4. Kyu"));
        QCOMPARE(programme.practiceEntries(QStringLiteral("aikido"), QStringLiteral("k5")).size(), k5);
    }

    // Nothing entered, nothing chosen: the first grade. Then the one after the highest passed.
    void practiceGradeFollowsPassedGrades()
    {
        const QString aikido = QStringLiteral("aikido");
        QTemporaryDir dir;
        const auto courses = emptyCourses(dir);
        PracticeGrade practice(&programme, courses.get());
        QSignalSpy changed(&practice, &PracticeGrade::changed);
        QCOMPARE(practice.grade(aikido), QStringLiteral("k5"));
        QVERIFY(!practice.manual(aikido));

        QVERIFY(pass(*courses, aikido, QStringLiteral("k5")));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k4"));
        QCOMPARE(changed.size(), 1);

        // By rank, not by day; other disciplines stay.
        QVERIFY(pass(*courses, aikido, QStringLiteral("k3")));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k2"));
        QCOMPARE(practice.grade(QStringLiteral("daitoryu")), QStringLiteral("k5"));

        // A note changes Courses, not the practice grade: no signal.
        changed.clear();
        const QString notebook = courses->create(QStringLiteral("Kurs"), QStringLiteral("2026-01-01"),
                                                 QStringLiteral("2026-01-01"), QString(), QString());
        QVERIFY(!notebook.isEmpty());
        QCOMPARE(changed.size(), 0);
    }

    // After the last grade with forms (next: Hon-Menkyo, then none at all) it stays on that one;
    // a grade without forms is never one.
    void practiceGradeHasForms()
    {
        const QString toho = QStringLiteral("shinkitoho");
        QTemporaryDir dir;
        const auto courses = emptyCourses(dir);
        PracticeGrade practice(&programme, courses.get());
        const QVariantList grades = programme.grades(toho);
        const QString last = grades.last().toMap().value(QStringLiteral("id")).toString();

        QVERIFY(pass(*courses, toho, last));
        QCOMPARE(courses->nextGrade(toho), QStringLiteral("honmenkyo"));
        QCOMPARE(practice.grade(toho), last);

        QVERIFY(pass(*courses, toho, QStringLiteral("kaiden")));
        QCOMPARE(courses->nextGrade(toho), QString());
        QCOMPARE(practice.grade(toho), last);

        practice.setGrade(toho, QStringLiteral("kaiden"));
        QCOMPARE(practice.grade(toho), last);
        practice.setGrade(toho, QStringLiteral("nonsense"));
        QCOMPARE(practice.grade(toho), last);
        QVERIFY(!practice.manual(toho));
    }

    // A choice holds, also below the suggestion and across a new start.
    void practiceGradeByHand()
    {
        const QString aikido = QStringLiteral("aikido");
        QTemporaryDir dir;
        const auto courses = emptyCourses(dir);
        QVERIFY(pass(*courses, aikido, QStringLiteral("k4")));
        PracticeGrade practice(&programme, courses.get());
        QSignalSpy changed(&practice, &PracticeGrade::changed);
        QCOMPARE(practice.grade(aikido), QStringLiteral("k3"));

        practice.setGrade(aikido, QStringLiteral("k5"));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k5"));
        QVERIFY(practice.manual(aikido));
        QCOMPARE(changed.size(), 1);
        PracticeGrade restarted(&programme, courses.get());
        QCOMPARE(restarted.grade(aikido), QStringLiteral("k5"));

        // The suggested grade again: follows "My grades" from here on.
        practice.setGrade(aikido, QStringLiteral("k3"));
        QVERIFY(!practice.manual(aikido));
        QCOMPARE(changed.size(), 2);
        QVERIFY(pass(*courses, aikido, QStringLiteral("k3")));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k2"));
    }

    // A passed grade moves a choice forward unless it is further already.
    void practiceGradeAfterPassing()
    {
        const QString aikido = QStringLiteral("aikido");
        QTemporaryDir dir;
        const auto courses = emptyCourses(dir);
        QVERIFY(pass(*courses, aikido, QStringLiteral("k4")));
        PracticeGrade practice(&programme, courses.get());

        // Behind: moves to the new suggestion.
        practice.setGrade(aikido, QStringLiteral("k5"));
        QVERIFY(pass(*courses, aikido, QStringLiteral("k3")));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k2"));
        QVERIFY(!practice.manual(aikido));

        // Further: stays, until the suggestion passes it.
        practice.setGrade(aikido, QStringLiteral("k1"));
        QVERIFY(pass(*courses, aikido, QStringLiteral("k2")));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k1"));
        QVERIFY(practice.manual(aikido));
        QVERIFY(pass(*courses, aikido, QStringLiteral("k1")));
        QCOMPARE(practice.grade(aikido), courses->nextGrade(aikido));
        QVERIFY(!practice.manual(aikido));
    }

    // A passed grade taken back gives the choice back.
    void practiceGradeAfterRemoving()
    {
        const QString aikido = QStringLiteral("aikido");
        QTemporaryDir dir;
        const auto courses = emptyCourses(dir);
        QVERIFY(pass(*courses, aikido, QStringLiteral("k5")));
        PracticeGrade practice(&programme, courses.get());
        practice.setGrade(aikido, QStringLiteral("k5"));

        QVERIFY(pass(*courses, aikido, QStringLiteral("k4")));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k3"));
        const QVariantList grades = courses->grades();
        const auto k4 = std::find_if(grades.cbegin(), grades.cend(), [](const QVariant &g) {
            return g.toMap().value(QStringLiteral("grade")) == QLatin1String("k4");
        });
        QVERIFY(courses->removeGrade(k4->toMap().value(QStringLiteral("uid")).toString()));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k5"));
    }

    // The one old grade becomes the choice once: nothing moves on the update, a later passed
    // grade counts. A value without forms goes.
    void practiceGradeMigrates()
    {
        const QString aikido = QStringLiteral("aikido");
        const QString toho = QStringLiteral("shinkitoho");
        const QString oldAikido = QStringLiteral("programme/grade/") + aikido;
        const QString oldToho = QStringLiteral("programme/grade/") + toho;
        QTemporaryDir dir;
        const auto courses = emptyCourses(dir);
        QVERIFY(pass(*courses, aikido, QStringLiteral("k4")));
        QSettings().setValue(oldAikido, QStringLiteral("k5"));
        QSettings().setValue(oldToho, QStringLiteral("kaiden"));

        PracticeGrade practice(&programme, courses.get());
        QCOMPARE(practice.grade(aikido), QStringLiteral("k5"));
        QVERIFY(practice.manual(aikido));
        QVERIFY(!QSettings().contains(oldAikido));
        QVERIFY(!QSettings().contains(oldToho));
        QCOMPARE(practice.grade(toho), programme.grades(toho).first().toMap().value(QStringLiteral("id")).toString());

        // Once: a choice made after it stays.
        practice.setGrade(aikido, QStringLiteral("k1"));
        PracticeGrade restarted(&programme, courses.get());
        QCOMPARE(restarted.grade(aikido), QStringLiteral("k1"));

        practice.setGrade(aikido, QStringLiteral("k5"));
        QVERIFY(pass(*courses, aikido, QStringLiteral("k3")));
        QCOMPARE(practice.grade(aikido), QStringLiteral("k2"));
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
        programme.setDiscipline(QStringLiteral("daitoryu"));
        Programme other{loaded()};
        QCOMPARE(other.discipline(), QStringLiteral("daitoryu"));
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
        QCOMPARE(count(shoden, QStringLiteral("entry")), 3 + 1 + 1 + 11);
        QCOMPARE(count(shoden, QStringLiteral("info")), 1);
        const QStringList chuden = ids(programme.entries(QStringLiteral("shinkitoho"), QStringLiteral("chuden")));
        QCOMPARE(chuden.size(), 10);
        QCOMPARE(chuden.first(), QStringLiteral("shinkitoho/odachi/12"));
    }

    // The Kamae exercise is one entry (one flow); its 26 positions are its steps, Seigan twice.
    void kamaeIsOneEntry()
    {
        const QStringList shoden = ids(programme.entries(QStringLiteral("shinkitoho"), QStringLiteral("shoden")));
        QCOMPARE(shoden.filter(QStringLiteral("shinkitoho/kamae")), QStringList{QStringLiteral("shinkitoho/kamae")});
        const QVariantMap kamae = programme.entry(QStringLiteral("shinkitoho/kamae"));
        QCOMPARE(kamae.value(QStringLiteral("title")).toString(), QStringLiteral("Kamae-Übung"));
        QCOMPARE(kamae.value(QStringLiteral("jp")).toString(), QStringLiteral("構"));
        QVERIFY(kamae.value(QStringLiteral("video")).isValid());
        QVERIFY(kamae.value(QStringLiteral("comment")).isValid());
        const QVariantList steps = kamae.value(QStringLiteral("steps")).toList();
        QCOMPARE(steps.size(), 26);
        QCOMPARE(steps.first().toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Rei"));
        QCOMPARE(steps.at(2).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Seigan"));
        QCOMPARE(steps.at(11).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Seigan"));
        QCOMPARE(steps.at(2).toMap().value(QStringLiteral("jp")).toString(), QStringLiteral("正眼"));
        QVERIFY(programme.entry(QStringLiteral("shinkitoho/kamae/1")).isEmpty());
        QVERIFY(programme.entry(QStringLiteral("shinkitoho/odachi/1")).value(QStringLiteral("steps")).toList().isEmpty());
        // Today's practice and the search hold it once; a position finds it.
        QCOMPARE(ids(programme.practiceEntries(QStringLiteral("shinkitoho"), QStringLiteral("shoden"))).count(QStringLiteral("shinkitoho/kamae")), 1);
        QVERIFY(ids(programme.search(QStringLiteral("jodan ura gasumi"))).contains(QStringLiteral("shinkitoho/kamae")));
        QVERIFY(ids(programme.search(QStringLiteral("開太刀"))).contains(QStringLiteral("shinkitoho/kamae")));
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
        // The tooltip names every grade of the place, not only the first one's.
        QCOMPARE(ikkyo.value(QStringLiteral("gradeNames")).toStringList(), (QStringList{QStringLiteral("5. Kyu"), QStringLiteral("4. Kyu")}));
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

    // The grids stand in the order of the discipline's groups (Daitoryu: "Zatori · Yoko-seme" from
    // the kyu grades before "Sonstige Techniken", not after it, where it came in a dan grade).
    void techniqueMapInGroupOrder()
    {
        for (const QString &discipline : {QStringLiteral("aikido"), QStringLiteral("daitoryu")}) {
            const QVariantList grades = programme.grades(discipline);
            const QString last = grades.last().toMap().value(QStringLiteral("id")).toString();

            QStringList order;
            for (const QVariant &d : programme.disciplines()) {
                if (d.toMap().value(QStringLiteral("id")) != discipline) {
                    continue;
                }
                for (const QVariant &g : d.toMap().value(QStringLiteral("gruppen")).toList()) {
                    const QString title = g.toMap().value(QStringLiteral("titel")).toString();
                    if (!order.contains(title)) {
                        order << title;
                    }
                }
            }

            QList<qsizetype> positions;
            for (const QVariant &grid : programme.techniqueMap(discipline, last)) {
                positions << order.indexOf(grid.toMap().value(QStringLiteral("title")).toString());
            }
            QVERIFY(positions.size() > 1);
            QVERIFY2(!positions.contains(-1), qPrintable(discipline));
            QVERIFY2(std::is_sorted(positions.cbegin(), positions.cend()), qPrintable(discipline));
        }
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
    // Settings in the build directory (not ~/.qttest, which every build and review running at the
    // same time would share and overwrite).
    qputenv("XDG_CONFIG_HOME", TEST_DATA_DIR "/config");
    qputenv("XDG_DATA_HOME", TEST_DATA_DIR "/data");
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("mitori-test"));
    TstProgramme test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_programme.moc"
