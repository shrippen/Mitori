#pragma once

#include <QObject>
#include <QPointer>
#include <QVideoFrame>
#include <QVideoSink>
#include <QtQml/qqmlregistration.h>

// The DVD's code from a camera, for QML. The QR code holds exactly the code as printed in
// bundle.py's CSV ("7KQ2M-X9HDV-3TAPR-WE6YC"; ROADMAP.md, step 4). Never logs what it reads.
//
//   open() ──camera? permission (asked now, not at start)──► ready() / refused(reason)
//   watch(sink) ─frame, one at a time, off the UI thread─► found(code), then it stops
//                                                          foreign() and it reads on
//
// Tests: MITORI_CAMERA=none|denied|frames stands in for the device; "frames" opens without a
// camera, the frames come from the test.
class CodeScanner : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    // Whether the page runs the real camera (not while a test feeds the frames).
    Q_PROPERTY(bool liveCamera READ liveCamera CONSTANT)

public:
    enum class Refusal { NoCamera, Denied };
    Q_ENUM(Refusal)

    explicit CodeScanner(QObject *parent = nullptr);

    bool liveCamera() const;

    Q_INVOKABLE void open();
    Q_INVOKABLE void watch(QVideoSink *sink);
    Q_INVOKABLE void stop();

signals:
    void ready();
    void refused(CodeScanner::Refusal reason);
    void found(const QString &code);
    // A QR code without a valid code in it (length, characters, check character).
    void foreign();

private:
    void take(const QVideoFrame &frame);
    void report(const QString &text);

    QPointer<QVideoSink> m_sink;
    int m_watch = 0;            // a read that started for an earlier watch() does not count
    bool m_reading = false;
    qint64 m_lastRead = 0;      // ms since the epoch
};
