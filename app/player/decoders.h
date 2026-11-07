#pragma once

#include <QObject>

class QThread;

// Qt Multimedia makes its backend when the first player (or camera) is made, and the FFmpeg
// backend then tries every video decoder of the GPU: 1.1 s on the UI thread with NVIDIA (vdpau,
// cuda, vaapi, qsv, vulkan), little on Android (MediaCodec is not tried). prepare() tries them on
// a thread of its own, so the first video opens without that wait. Qt's backend itself is made
// on the UI thread as ever: one made on the worker and handed over could reach a player before
// it belonged to the UI thread.
//
//   UI thread   prepare() ─────────────────────────── ready()      first player: list there
//   worker               decoders tried (a backend of its own, deleted)
class Decoders : public QObject
{
    Q_OBJECT

public:
    static Decoders &shared();

    // Once; later calls do nothing. A player made meanwhile waits for the decoder list, as it
    // would have waited for it anyway.
    void prepare();
    bool isReady() const { return m_ready; }

signals:
    void ready();

private:
    Decoders() = default;
    static void waitAtExit();

    QThread *m_thread = nullptr;
    bool m_ready = false;
};
