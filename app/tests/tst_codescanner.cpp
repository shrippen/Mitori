#include <QElapsedTimer>
#include <QGuiApplication>
#include <QPainter>
#include <QSignalSpy>
#include <QTest>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <QtGlobal>

#include "codescanner.h"
#include "qrimage.h"
#include "qrreader.h"

namespace {

// From tests/data/bundle/README.md: nothing real.
const QString kCode = QStringLiteral("7KQ2M-X9HDV-3TAPR-WE6YT");
const QString kTypo = QStringLiteral("7KQ2M-X9HDV-3TAPR-WE6YA");
const QString kForeign = QStringLiteral("https://example.org/keine-dvd");
constexpr int kReadTimeoutMs = 5000;
constexpr int kSilenceMs = 400;

QImage blurred(const QImage &image)
{
    return image.scaled(image.size() / 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
        .scaled(image.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QImage blank(int size = 360)
{
    QImage image(size, size, QImage::Format_Grayscale8);
    image.fill(128);
    return image;
}

// The picture as a camera's frame, brightness in its own plane (NV12).
QVideoFrame nv12Frame(const QImage &gray)
{
    QVideoFrame frame(QVideoFrameFormat(gray.size(), QVideoFrameFormat::Format_NV12));
    if (!frame.map(QVideoFrame::WriteOnly)) {
        return {};
    }
    for (int y = 0; y < gray.height(); ++y) {
        memcpy(frame.bits(0) + y * frame.bytesPerLine(0), gray.constScanLine(y), size_t(gray.width()));
    }
    memset(frame.bits(1), 128, size_t(frame.mappedBytes(1)));
    frame.unmap();
    return frame;
}

QStringList logged;
void keepLog(QtMsgType, const QMessageLogContext &, const QString &message)
{
    logged << message;
}

} // namespace

class TestCodeScanner : public QObject
{
    Q_OBJECT

private:
    // A camera at work: the frame again and again, until the signal comes.
    static bool showUntil(QVideoSink &sink, const QImage &image, QSignalSpy &spy)
    {
        QElapsedTimer timer;
        timer.start();
        while (spy.isEmpty() && timer.elapsed() < kReadTimeoutMs) {
            sink.setVideoFrame(QVideoFrame(image));     // a new frame: the sink ignores the same one
            QTest::qWait(30);
        }
        return !spy.isEmpty();
    }

private slots:
    void readsValidCode()
    {
        QCOMPARE(QrReader::read(qrImage(kCode)), kCode);
    }

    void readsForeignText()
    {
        QCOMPARE(QrReader::read(qrImage(kForeign)), kForeign);
    }

    void readsTurnedAndSmall()
    {
        QCOMPARE(QrReader::read(qrImage(kCode).transformed(QTransform().rotate(90))), kCode);

        // The code somewhere in a bigger, gray picture, as on a table.
        QImage scene = blank(900);
        QPainter(&scene).drawImage(300, 420, qrImage(kCode, 240));
        QCOMPARE(QrReader::read(scene), kCode);
    }

    void readsNothing()
    {
        QCOMPARE(QrReader::read(QImage()), QString());
        QCOMPARE(QrReader::read(blank()), QString());
        QCOMPARE(QrReader::read(blurred(qrImage(kCode))), QString());
        QCOMPARE(QrReader::read(QVideoFrame()), QString());
    }

    void readsFrames()
    {
        QCOMPARE(QrReader::read(QVideoFrame(qrImage(kCode))), kCode);
        QCOMPARE(QrReader::read(nv12Frame(qrImage(kCode))), kCode);
        QCOMPARE(QrReader::read(nv12Frame(blank())), QString());
    }

    void findsCodeAndStops()
    {
        qputenv("MITORI_CAMERA", "frames");
        CodeScanner scanner;
        QVideoSink sink;
        QSignalSpy found(&scanner, &CodeScanner::found);
        scanner.watch(&sink);

        QVERIFY(showUntil(sink, qrImage(kCode), found));
        QCOMPARE(found.first().first().toString(), kCode);

        // It stopped: more frames, no second signal.
        found.clear();
        sink.setVideoFrame(QVideoFrame(qrImage(kCode)));
        QTest::qWait(kSilenceMs);
        QVERIFY(found.isEmpty());
    }

    void groupsBareCode()
    {
        CodeScanner scanner;
        QVideoSink sink;
        QSignalSpy found(&scanner, &CodeScanner::found);
        scanner.watch(&sink);
        QVERIFY(showUntil(sink, qrImage(QString(kCode).remove(QLatin1Char('-'))), found));
        QCOMPARE(found.first().first().toString(), kCode);
    }

    // Foreign text and a wrong check character: a sentence, the camera reads on.
    void keepsReadingAfterForeign_data()
    {
        QTest::addColumn<QString>("text");
        QTest::newRow("url") << kForeign;
        QTest::newRow("check character") << kTypo;
    }
    void keepsReadingAfterForeign()
    {
        QFETCH(QString, text);
        CodeScanner scanner;
        QVideoSink sink;
        QSignalSpy foreign(&scanner, &CodeScanner::foreign);
        QSignalSpy found(&scanner, &CodeScanner::found);
        scanner.watch(&sink);

        QVERIFY(showUntil(sink, qrImage(text), foreign));
        QVERIFY(found.isEmpty());
        QVERIFY(showUntil(sink, qrImage(kCode), found));
    }

    void staysSilentOnEmptyFrames()
    {
        CodeScanner scanner;
        QVideoSink sink;
        QSignalSpy foreign(&scanner, &CodeScanner::foreign);
        QSignalSpy found(&scanner, &CodeScanner::found);
        scanner.watch(&sink);

        sink.setVideoFrame(QVideoFrame(blank()));
        QTest::qWait(kSilenceMs);
        sink.setVideoFrame(QVideoFrame(blurred(qrImage(kCode))));
        QTest::qWait(kSilenceMs);
        QVERIFY(foreign.isEmpty());
        QVERIFY(found.isEmpty());
    }

    // Cancelled while a frame is being read: its result does not count.
    void stopDropsReadInProgress()
    {
        CodeScanner scanner;
        QVideoSink sink;
        QSignalSpy found(&scanner, &CodeScanner::found);
        scanner.watch(&sink);
        sink.setVideoFrame(QVideoFrame(qrImage(kCode)));
        scanner.unwatch(&sink);
        QTest::qWait(kSilenceMs);
        QVERIFY(found.isEmpty());
    }

    // The page sliding out says so late: the next page's reading goes on.
    void unwatchLeavesOtherSink()
    {
        CodeScanner scanner;
        QVideoSink old;
        QVideoSink current;
        QSignalSpy found(&scanner, &CodeScanner::found);
        scanner.watch(&current);
        scanner.unwatch(&old);
        QVERIFY(showUntil(current, qrImage(kCode), found));
    }

    void refuses()
    {
        CodeScanner scanner;
        QSignalSpy refused(&scanner, &CodeScanner::refused);
        QSignalSpy ready(&scanner, &CodeScanner::ready);

        qputenv("MITORI_CAMERA", "none");
        scanner.open();
        QCOMPARE(refused.takeFirst().first().value<CodeScanner::Refusal>(), CodeScanner::Refusal::NoCamera);

        qputenv("MITORI_CAMERA", "denied");
        scanner.open();
        QCOMPARE(refused.takeFirst().first().value<CodeScanner::Refusal>(), CodeScanner::Refusal::Denied);
        QVERIFY(ready.isEmpty());

        qputenv("MITORI_CAMERA", "frames");
        scanner.open();
        QCOMPARE(ready.size(), 1);
        QVERIFY(!scanner.liveCamera());
    }

    void neverLogsTheCode()
    {
        logged.clear();
        const QtMessageHandler before = qInstallMessageHandler(keepLog);
        {
            CodeScanner scanner;
            QVideoSink sink;
            QSignalSpy found(&scanner, &CodeScanner::found);
            scanner.watch(&sink);
            showUntil(sink, qrImage(kCode), found);
        }
        qInstallMessageHandler(before);
        const QString all = logged.join(QLatin1Char('\n'));
        QVERIFY2(!all.contains(kCode) && !all.contains(QString(kCode).remove(QLatin1Char('-'))), "the code is in the log");
    }
};

QTEST_MAIN(TestCodeScanner)
#include "tst_codescanner.moc"
