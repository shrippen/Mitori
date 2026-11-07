#include <QCameraDevice>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include "decoders.h"

// Qt's video backend made in the background (Decoders): made on the worker, then it belongs to the
// UI thread, where players and cameras make their children of it (without warnings).
class TstDecoders : public QObject
{
    Q_OBJECT

private slots:
    void preparedOnAWorker()
    {
        QTest::failOnWarning();
        QSignalSpy ready(&Decoders::shared(), &Decoders::ready);
        Decoders::shared().prepare();
        Decoders::shared().prepare();      // once
        QVERIFY(ready.wait(30000));
        QVERIFY(Decoders::shared().isReady());
        QCOMPARE(ready.count(), 1);

        QPlatformMediaIntegration *backend = QPlatformMediaIntegration::instance();
        QVERIFY(backend);
        QCOMPARE(backend->thread(), QThread::currentThread());

        // What the app makes of it afterwards: the camera list, a player.
        QMediaDevices::videoInputs();
        QMediaPlayer player;
        QVERIFY(player.isAvailable());
    }
};

QTEST_MAIN(TstDecoders)
#include "tst_decoders.moc"
