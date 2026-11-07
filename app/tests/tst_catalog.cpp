#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include "catalog.h"

class TstCatalog : public QObject
{
    Q_OBJECT

private slots:
    void loadsAllDisciplines()
    {
        Catalog catalog;
        QVERIFY2(catalog.load(QStringLiteral(MITORI_DATEN_JSON)), qPrintable(catalog.errorString()));
        QStringList ids;
        for (const QVariant &discipline : catalog.disciplines()) {
            const QVariantMap map = discipline.toMap();
            ids << map.value(QStringLiteral("id")).toString();
            QVERIFY(!map.value(QStringLiteral("grade")).toList().isEmpty());
        }
        QCOMPARE(ids, (QStringList{QStringLiteral("aikido"), QStringLiteral("daitoryu"), QStringLiteral("shinkitoho")}));
    }

    void dataVersionIsThere()
    {
        Catalog catalog;
        QVERIFY(catalog.load(QStringLiteral(MITORI_DATEN_JSON)));
        QVERIFY(QRegularExpression(QStringLiteral("^\\d{4}\\.\\d{1,2}\\.\\d+$")).match(catalog.dataVersion()).hasMatch());

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("alt.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"format": 1, "disziplinen": [{"id": "aikido"}]})");
        file.close();
        QVERIFY(!catalog.load(path));
        QVERIFY(catalog.errorString().contains(QLatin1String("data version")));
    }

    void loadsGlossary()
    {
        Catalog catalog;
        QVERIFY(catalog.load(QStringLiteral(MITORI_DATEN_JSON)));
        QVERIFY(catalog.glossary().size() > 219);       // the lexicon's words and more
        QVERIFY(catalog.glossarySource().contains(QLatin1String("shinkiryu.de")));
        const QVariantMap first = catalog.glossary().constFirst().toMap();
        QCOMPARE(first.value(QStringLiteral("wort")).toString(), QStringLiteral("Ago"));
        QCOMPARE(first.value(QStringLiteral("kana")).toString(), QStringLiteral("あご"));
    }

    // English: every field x reads as its x_en where the data have one (the grades, a Kamae
    // position, a comment, the admission list); ids, Japanese names and the videos stay.
    void englishInPlaceOfGerman()
    {
        Catalog catalog;
        QVERIFY(catalog.load(QStringLiteral(MITORI_DATEN_JSON)));
        const auto aikidoIn = [&catalog](Catalog::Language language) {
            Catalog::setLanguage(language);
            return catalog.disciplines().constFirst().toMap();
        };
        const QVariantMap german = aikidoIn(Catalog::Language::German);
        const QVariantMap english = aikidoIn(Catalog::Language::English);
        Catalog::setLanguage(Catalog::Language::German);

        const auto firstGrade = [](const QVariantMap &d) { return d.value(QStringLiteral("grade")).toList().constFirst().toMap(); };
        QCOMPARE(firstGrade(german).value(QStringLiteral("name")).toString(), QStringLiteral("5. Kyu"));
        QCOMPARE(firstGrade(english).value(QStringLiteral("name")).toString(), QStringLiteral("5th Kyu"));
        QCOMPARE(firstGrade(english).value(QStringLiteral("id")), firstGrade(german).value(QStringLiteral("id")));
        QCOMPARE(english.value(QStringLiteral("kurzname")).toString(), QStringLiteral("Aikido"));
        QVERIFY(english.value(QStringLiteral("stand")).toString().startsWith(QLatin1String("As of")));
        QVERIFY(german.value(QStringLiteral("stand")).toString().startsWith(QLatin1String("Stand")));

        Catalog::setLanguage(Catalog::Language::English);
        const QVariantMap toho = catalog.disciplines().constLast().toMap();
        const QVariantMap kamae = toho.value(QStringLiteral("kamae")).toMap();
        const QVariantList positions = kamae.value(QStringLiteral("positionen")).toList();
        QCOMPARE(positions.at(15).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Tsuki (right foot forward)"));
        QVERIFY(kamae.value(QStringLiteral("kommentar")).toMap().value(QStringLiteral("text")).toString().startsWith(QLatin1String("1. Rei\nOne stands")));
        QVERIFY(toho.value(QStringLiteral("zulassung")).toList().constFirst().toString().startsWith(QLatin1String("Minimum age")));
        QVERIFY(catalog.glossarySource().startsWith(QLatin1String("Lexicon")));
        QVERIFY(!catalog.videos().isEmpty());
        Catalog::setLanguage(Catalog::Language::German);
        QVERIFY(catalog.glossarySource().startsWith(QLatin1String("Lexikon")));
    }

    // The data's language as the app's catalogs pick theirs: the first of the locale's languages
    // with one, English without.
    void languageOfLocale()
    {
        QCOMPARE(Catalog::languageOf(QLocale(QStringLiteral("de_CH"))), Catalog::Language::German);
        QCOMPARE(Catalog::languageOf(QLocale(QStringLiteral("en_GB"))), Catalog::Language::English);
        QCOMPARE(Catalog::languageOf(QLocale(QStringLiteral("fr_FR"))), Catalog::Language::English);
        QCOMPARE(Catalog::languageOf(QLocale::c()), Catalog::Language::English);
    }

    void missingFileFails()
    {
        Catalog catalog;
        QVERIFY(!catalog.load(QStringLiteral("/nonexistent/mitori-daten.json")));
        QVERIFY(catalog.disciplines().isEmpty());
        QVERIFY(!catalog.errorString().isEmpty());
    }

    void otherFormatFails()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("daten.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"format": 99, "disziplinen": [{"id": "aikido"}]})");
        file.close();
        Catalog catalog;
        QVERIFY(!catalog.load(path));
        QVERIFY(catalog.disciplines().isEmpty());
    }

    void noDisciplinesFails()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("daten.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"format": 1})");
        file.close();
        Catalog catalog;
        QVERIFY(!catalog.load(path));
        QVERIFY(!catalog.errorString().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstCatalog)
#include "tst_catalog.moc"
