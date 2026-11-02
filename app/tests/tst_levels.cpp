#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include "levels.h"

// Levels against the platform's memory levels (MITORI_DEVICE_LEVELS): never the real screen.
class TstLevels : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
    }

    // A device with a volume of its own (Android): the gesture moves it, the player stays at 1;
    // levels stop at 0 and 1; leaving the player gives the screen its brightness back.
    void deviceVolume()
    {
        qputenv("MITORI_DEVICE_LEVELS", "memory");
        Levels l;
        QCOMPARE(l.brightness(), 0.5);
        QCOMPARE(l.volume(), 0.5);
        QSignalSpy changed(&l, &Levels::changed);
        QVERIFY(l.change(Levels::Level::Volume, 0.2));
        QCOMPARE(l.volume(), 0.7);
        QCOMPARE(l.playerVolume(), 1.0);
        QVERIFY(l.change(Levels::Level::Volume, 2));
        QCOMPARE(l.volume(), 1.0);
        QVERIFY(l.change(Levels::Level::Brightness, -0.3));
        QCOMPARE(l.brightness(), 0.2);
        QVERIFY(l.change(Levels::Level::Brightness, -1));
        QCOMPARE(l.brightness(), 0.0);
        QCOMPARE(changed.count(), 4);
        l.restore();
        QCOMPARE(l.brightness(), 0.5);
        QCOMPARE(Levels().volume(), 1.0);       // the device keeps it
    }

    // Without a device volume (the desktop): the player's own sound goes up and down and stays
    // for the next start.
    void ownVolume()
    {
        qputenv("MITORI_DEVICE_LEVELS", "memory-screen");
        {
            Levels l;
            QCOMPARE(l.volume(), 1.0);
            QVERIFY(l.change(Levels::Level::Volume, -0.4));
            QCOMPARE(l.playerVolume(), 0.6);
            QCOMPARE(l.volume(), 0.6);
        }
        QCOMPARE(Levels().playerVolume(), 0.6);
    }

    // The player's switches: sound and "screen on" stay for the next start (on by default);
    // the screen is held only while the player is open and the switch is on.
    void switches()
    {
        qputenv("MITORI_DEVICE_LEVELS", "memory");
        {
            Levels l;
            QVERIFY(l.sound());
            QVERIFY(l.awake());
            QVERIFY(!l.held());
            l.enter();
            QVERIFY(l.held());
            l.setAwake(Levels::Switch::Off);
            QVERIFY(!l.held());
            l.setAwake(Levels::Switch::On);
            QVERIFY(l.held());
            l.restore();
            QVERIFY(!l.held());
            l.setAwake(Levels::Switch::Off);
            l.setSound(Levels::Switch::Off);
        }
        Levels l;
        QVERIFY(!l.sound());
        QVERIFY(!l.awake());
        l.enter();
        QVERIFY(!l.held());
        l.restore();
    }
};

QTEST_GUILESS_MAIN(TstLevels)
#include "tst_levels.moc"
