#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include "appearance.h"
#include "catalog.h"

// The style and language choice: saved and at once in the app, Breeze only where it exists,
// one controls style in a run, the start falls back to the default for a style the device does not offer.
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
    }

    // A choice counts at once, and the next start begins with it. Without a window nothing is
    // to be built anew.
    void choiceSaved()
    {
        QCOMPARE(Appearance::startStyle({}), Appearance::Style::Leinen);
        Appearance a;
        QSignalSpy changed(&a, &Appearance::changed);
        a.setStyle(Appearance::Style::Kante);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(a.running(), Appearance::Style::Kante);
        a.setStyle(Appearance::Style::Kante);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(Appearance::startStyle({}), Appearance::Style::Kante);
        QCOMPARE(Appearance().running(), Appearance::Style::Kante);
    }

    // Without Plasma's controls style no Breeze: not offered, and a saved Breeze starts as the default.
    void breezeOnlyWhereItIs()
    {
        Appearance a;
        const QVariantList styles = a.styles();
        for (const QVariant &s : styles) {
            QVERIFY(s.toMap().value(QStringLiteral("style")).toInt() != int(Appearance::Style::Breeze));
        }
        QCOMPARE(styles.first().toMap().value(QStringLiteral("style")).toInt(), int(Appearance::Style::Kante));
        a.setStyle(Appearance::Style::Breeze);
        QCOMPARE(Appearance::startStyle({}), Appearance::Style::Leinen);
        QCOMPARE(Appearance::controlsStyle(Appearance::Style::Material, {}), QStringLiteral("Material"));
        QCOMPARE(Appearance::controlsStyle(Appearance::Style::Kante, {}), QStringLiteral("Material"));
    }

    // Material only on Android (owner, 06.11.2026), here never, Plasma or not. A saved one starts
    // as the default, with that tile marked; the saved value stays. All styles a device offers
    // share one controls style, so every change works without a restart.
    void materialOnlyOnAndroid()
    {
        QTemporaryDir plasma;
        QVERIFY(QDir(plasma.path()).mkpath(QStringLiteral("org/kde/desktop")));
        const QStringList paths{plasma.path()};
        QSettings().setValue(QStringLiteral("look/style"), int(Appearance::Style::Material));

        for (const bool session : {true, false}) {
            if (session) {
                qputenv("KDE_FULL_SESSION", "true");
            } else {
                qunsetenv("KDE_FULL_SESSION");
            }
            QCOMPARE(Appearance::startStyle(paths), Appearance::Style::Leinen);
            Appearance a;
            QCOMPARE(a.style(), Appearance::Style::Leinen);
            for (const QVariant &s : a.styles()) {
                QVERIFY(s.toMap().value(QStringLiteral("style")).toInt() != int(Appearance::Style::Material));
            }
            QCOMPARE(QSettings().value(QStringLiteral("look/style")).toInt(), int(Appearance::Style::Material));
        }

        qputenv("KDE_FULL_SESSION", "true");
        QStringList controls;
        for (const Appearance::Style style : {Appearance::Style::Breeze, Appearance::Style::Kante,
                                              Appearance::Style::Leinen, Appearance::Style::Shinki}) {
            controls << Appearance::controlsStyle(style, paths);
        }
        controls.removeDuplicates();
        QCOMPARE(controls, QStringList{QStringLiteral("org.kde.desktop")});
        qunsetenv("KDE_FULL_SESSION");
        QSettings().remove(QStringLiteral("look/style"));
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

    // The language: the user's, or German or English whatever the system says; dates, numbers and
    // the programme data follow at once.
    void language()
    {
        Appearance a;
        QSignalSpy changed(&a, &Appearance::languageChanged);
        a.setLanguage(Appearance::Language::German);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(QLocale().language(), QLocale::German);
        QCOMPARE(QLocale().toString(1.5, 'f', 1), QStringLiteral("1,5"));
        QCOMPARE(Catalog::language(), Catalog::Language::German);
        QCOMPARE(Appearance().language(), Appearance::Language::German);
        a.setLanguage(Appearance::Language::English);
        QCOMPARE(QLocale().language(), QLocale::English);
        QCOMPARE(QLocale().toString(1.5, 'f', 1), QStringLiteral("1.5"));
        QCOMPARE(Catalog::language(), Catalog::Language::English);
        a.setLanguage(Appearance::Language::System);
        QCOMPARE(QLocale().language(), QLocale::system().language());
        QVERIFY(Appearance::localeName(Appearance::Language::System).isEmpty());
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
