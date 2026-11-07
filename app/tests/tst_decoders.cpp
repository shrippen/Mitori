#include <QCameraDevice>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include "decoders.h"

#ifdef Q_OS_LINUX
#include <sched.h>
#endif

// Time for the worker to start and be inside Qt's backend (loading the FFmpeg plugin).
constexpr int kWorkerStartMs = 100;

// Qt's video backend prepared in the background (Decoders): the app's backend belongs to the UI
// thread, where players and cameras make their children of it (without warnings), also when the
// app asks for it while the worker is busy.
class TstDecoders : public QObject
{
    Q_OBJECT

private slots:
    // One core, as a busy phone: the worker, at low priority, waits while the UI thread runs on.
    // (A backend made on the worker and handed over reached the UI still the worker's:
    // backend->thread() was "Mitori decoders" in most runs.)
    void initTestCase()
    {
#ifdef Q_OS_LINUX
        cpu_set_t one;
        CPU_ZERO(&one);
        CPU_SET(sched_getcpu(), &one);
        QCOMPARE(sched_setaffinity(0, sizeof(one), &one), 0);
#endif
    }

    void usedWhilePreparing()
    {
        QTest::failOnWarning();
        QSignalSpy ready(&Decoders::shared(), &Decoders::ready);
        Decoders::shared().prepare();
        Decoders::shared().prepare();      // once
        QThread::msleep(kWorkerStartMs);

        // What the app makes of it: the camera list, a player.
        QMediaDevices::videoInputs();
        QPlatformMediaIntegration *backend = QPlatformMediaIntegration::instance();
        QVERIFY(backend);
        QCOMPARE(backend->thread(), QThread::currentThread());
        QMediaPlayer player;
        QVERIFY(player.isAvailable());

        QVERIFY(ready.wait(30000));
        QVERIFY(Decoders::shared().isReady());
        QCOMPARE(ready.count(), 1);
        QCOMPARE(QPlatformMediaIntegration::instance(), backend);
        QCOMPARE(backend->thread(), QThread::currentThread());
        QMediaPlayer later;
        QVERIFY(later.isAvailable());
    }
};

QTEST_MAIN(TstDecoders)
#include "tst_decoders.moc"
