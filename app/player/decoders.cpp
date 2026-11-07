#include "decoders.h"

#include <QCoreApplication>
#include <QThread>

#ifdef MITORI_MULTIMEDIA_PRIVATE
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#endif

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
    // Without Qt's private headers the first player makes the backend, as before.
    m_ready = true;
    emit ready();
#else
    QThread *ui = QCoreApplication::instance()->thread();
    m_thread = QThread::create([ui] {
        // The backend is a QObject: it belongs to the thread that made it. Made here, it goes
        // to the UI thread, where cameras and players make their children of it.
        QPlatformMediaIntegration *backend = QPlatformMediaIntegration::instance();
        if (backend && backend->thread() == QThread::currentThread()) {
            backend->moveToThread(ui);
        }
    });
    m_thread->setObjectName(QStringLiteral("Mitori decoders"));
    connect(m_thread, &QThread::finished, this, [this] {
        m_thread->deleteLater();
        m_thread = nullptr;
        m_ready = true;
        emit ready();
    });
    // The application must not go while the worker makes the backend (it is an application
    // static Qt takes down with the application).
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
