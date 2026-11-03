#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include "appearance.h"

// The style and language choice: saved for the next start, Breeze only where it exists, the
// start falls back to the default for a style the device does not offer.
class TstAppearance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
        qunsetenv("KDE_FULL_SESSION");
    }

    // Nothing chosen yet: Leinen (Kante's light linen), the wizard still to come.
    void defaults()
    {
        Appearance a;
        QCOMPARE(a.style(), Appearance::Style::Leinen);
        QCOMPARE(a.language(), Appearance::Language::System);
        QVERIFY(!a.welcomed());
        QVERIFY(!a.restartNeeded());
    }

    // A choice applies at the next start: until then a restart is needed.
    void choiceAppliesAtStart()
    {
        QCOMPARE(Appearance::startStyle({}), Appearance::Style::Leinen);
        Appearance a;
        QSignalSpy changed(&a, &Appearance::changed);
        a.setStyle(Appearance::Style::Kante);
        QCOMPARE(changed.count(), 1);
        QVERIFY(a.restartNeeded());
        QCOMPARE(a.running(), Appearance::Style::Leinen);
        QCOMPARE(Appearance::startStyle({}), Appearance::Style::Kante);
        Appearance next;
        QVERIFY(!next.restartNeeded());
        QCOMPARE(next.running(), Appearance::Style::Kante);
    }

    // Without Plasma's controls style no Breeze: not offered, and a saved Breeze starts as the default.
    void breezeOnlyWhereItIs()
    {
        Appearance a;
        const QVariantList styles = a.styles();
        for (const QVariant &s : styles) {
            QVERIFY(s.toMap().value(QStringLiteral("style")).toInt() != int(Appearance::Style::Breeze));
        }
        QCOMPARE(styles.first().toMap().value(QStringLiteral("style")).toInt(), int(Appearance::Style::Material));
        a.setStyle(Appearance::Style::Breeze);
        QCOMPARE(Appearance::startStyle({}), Appearance::Style::Leinen);
        QCOMPARE(Appearance::controlsStyle(Appearance::Style::Material, {}), QStringLiteral("Material"));
        QCOMPARE(Appearance::controlsStyle(Appearance::Style::Kante, {}), QStringLiteral("Material"));
    }

    // Shinki comes only with -DMITORI_SHINKI=ON (until every page reaches it).
    void shinkiOnlyInDevelopment()
    {
        Appearance a;
        bool offered = false;
        for (const QVariant &s : a.styles()) {
            offered = offered || s.toMap().value(QStringLiteral("style")).toInt() == int(Appearance::Style::Shinki);
        }
#ifdef MITORI_SHINKI
        QVERIFY(offered);
#else
        QVERIFY(!offered);
#endif
    }

    // The language: the user's, or German or English whatever the system says.
    void language()
    {
        Appearance a;
        a.setLanguage(Appearance::Language::German);
        QCOMPARE(Appearance::startLocale(), QStringLiteral("de"));
        QVERIFY(a.restartNeeded());
        a.setLanguage(Appearance::Language::English);
        QCOMPARE(Appearance::startLocale(), QStringLiteral("en"));
        a.setLanguage(Appearance::Language::System);
        QVERIFY(Appearance::startLocale().isEmpty());
    }

    void welcome()
    {
        Appearance a;
        a.welcome();
        QVERIFY(Appearance().welcomed());
    }
};

QTEST_GUILESS_MAIN(TstAppearance)
#include "tst_appearance.moc"
