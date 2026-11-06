#include "codescanner.h"

#include <QCameraDevice>
#include <QCoreApplication>
#include <QDateTime>
#include <QFutureWatcher>
#include <QMediaDevices>
#include <QPermissions>
#include <QtConcurrent>

#include "bundlecrypto.h"
#include "qrreader.h"

namespace {

constexpr char kCameraEnv[] = "MITORI_CAMERA";
constexpr qint64 kReadGapMs = 100;      // a camera sends 30 frames a second; the battery needs not all

} // namespace

CodeScanner::CodeScanner(QObject *parent)
    : QObject(parent)
{
}

bool CodeScanner::liveCamera() const
{
    return qEnvironmentVariableIsEmpty(kCameraEnv);
}

void CodeScanner::open()
{
    const QByteArray stand = qgetenv(kCameraEnv);
    if (stand == "frames") {
        emit ready();
        return;
    }
    if (stand == "denied") {
        emit refused(Refusal::Denied);
        return;
    }
    if (stand == "none" || QMediaDevices::videoInputs().isEmpty()) {
        emit refused(Refusal::NoCamera);
        return;
    }

    const QCameraPermission camera;
    if (qApp->checkPermission(camera) == Qt::PermissionStatus::Granted) {
        emit ready();
        return;
    }

    // Android: a dialog the first time, a refusal at once when the user said never.
    qApp->requestPermission(camera, this, [this](const QPermission &answer) {
        if (answer.status() == Qt::PermissionStatus::Granted) {
            emit ready();
        } else {
            emit refused(Refusal::Denied);
        }
    });
}

void CodeScanner::watch(QVideoSink *sink)
{
    stop();
    m_sink = sink;
    if (sink) {
        // The camera's thread may send them: queued to this one, read on a worker.
        connect(sink, &QVideoSink::videoFrameChanged, this, &CodeScanner::take);
    }
}

void CodeScanner::stop()
{
    if (m_sink) {
        disconnect(m_sink, nullptr, this, nullptr);
    }
    m_sink = nullptr;
    ++m_watch;
}

void CodeScanner::take(const QVideoFrame &frame)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_reading || !m_sink || now - m_lastRead < kReadGapMs) {
        return;
    }
    m_reading = true;
    m_lastRead = now;

    const int watching = m_watch;
    auto *reading = new QFutureWatcher<QString>(this);
    connect(reading, &QFutureWatcher<QString>::finished, this, [this, reading, watching] {
        m_reading = false;
        const QString text = reading->result();
        reading->deleteLater();
        if (watching == m_watch && !text.isEmpty()) {
            report(text);
        }
    });
    reading->setFuture(QtConcurrent::run([frame] { return QrReader::read(frame); }));
}

void CodeScanner::report(const QString &text)
{
    const QString code = BundleCrypto::normalized(text);
    if (code.isEmpty()) {
        emit foreign();
        return;
    }
    stop();
    emit found(BundleCrypto::grouped(code));
}
