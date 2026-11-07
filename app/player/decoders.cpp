#include "decoders.h"

#include <QCoreApplication>
#include <QThread>

#ifdef MITORI_MULTIMEDIA_PRIVATE
#include <QtCore/private/qfactoryloader_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qplatformmediaplugin_p.h>
#endif

namespace {

#ifdef MITORI_MULTIMEDIA_PRIVATE
// The backend whose decoders are tried; Qt's default where Mitori runs.
const QString kFFmpeg = QStringLiteral("ffmpeg");

// Qt's own backend is the app's, made by the first player on the UI thread. Here a second one,
// for the worker alone: making it tries the GPU's decoders, and the plugin keeps that list for
// every backend after (a static in Qt's FFmpeg plugin). So nothing made here is ever shared.
//   UI thread   … first player: Qt's backend, made here, decoder list already there
//   worker      plugin loaded, backend made (decoders tried), deleted
void tryDecoders()
{
    const QString chosen = qEnvironmentVariable("QT_MEDIA_BACKEND").toLower();
    if (!chosen.isEmpty() && chosen != kFFmpeg) {
        return;
    }
    // Never deleted: a loader going deletes the plugin's object, which Qt's own loader shares and
    // may use right then on the UI thread. The library stays loaded (PreventUnloadHint), and so
    // does its list.
    static auto *loader = new QFactoryLoader(QPlatformMediaPlugin_iid, QStringLiteral("/multimedia"));
    delete qLoadPlugin<QPlatformMediaIntegration, QPlatformMediaPlugin>(loader, kFFmpeg);
}
#endif

} // namespace

Decoders &Decoders::shared()
{
    static Decoders decoders;
    return decoders;
}

void Decoders::prepare()
{
    if (m_thread || m_ready) {
        return;
    }
#ifndef MITORI_MULTIMEDIA_PRIVATE
    // Without Qt's private headers the first player tries the decoders, as before.
    m_ready = true;
    emit ready();
#else
    m_thread = QThread::create(tryDecoders);
    m_thread->setObjectName(QStringLiteral("Mitori decoders"));
    connect(m_thread, &QThread::finished, this, [this] {
        m_thread->deleteLater();
        m_thread = nullptr;
        m_ready = true;
        emit ready();
    });
    // The application must not go while the worker uses the plugin.
    qAddPostRoutine(waitAtExit);
    m_thread->start(QThread::LowPriority);
#endif
}

void Decoders::waitAtExit()
{
    if (QThread *thread = shared().m_thread) {
        thread->wait();
    }
}
