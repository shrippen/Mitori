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
