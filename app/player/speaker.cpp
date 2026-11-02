#include "speaker.h"

#ifdef MITORI_SPEECH
#include <QTextToSpeech>
#endif

Speaker::Speaker(const QString &engine, QObject *parent)
    : QObject(parent)
{
#ifdef MITORI_SPEECH
    m_speech = engine.isEmpty() ? new QTextToSpeech(this) : new QTextToSpeech(engine, this);
    m_state = State::Starting;
    // Some engines (Android) load their voices after construction and report it as a new state.
    connect(m_speech, &QTextToSpeech::stateChanged, this, &Speaker::update);
    update();
#else
    Q_UNUSED(engine)
#endif
}

void Speaker::say(const QString &text)
{
    if (m_state != State::Ready || text.isEmpty()) {
        return;
    }
#ifdef MITORI_SPEECH
    m_speech->stop();
    m_speech->say(text);
#endif
}

// The first Japanese voice, once the engine is ready.
void Speaker::update()
{
#ifdef MITORI_SPEECH
    State next = m_state;
    if (m_speech->state() == QTextToSpeech::Error) {
        next = State::NoVoice;
    } else if (m_state != State::Ready) {
        const QList<QVoice> voices = m_speech->findVoices(QLocale::Japanese);
        if (voices.isEmpty()) {
            next = State::NoVoice;
        } else {
            m_speech->setVoice(voices.constFirst());
            next = State::Ready;
        }
    }
    if (next == m_state) {
        return;
    }
    m_state = next;
    emit stateChanged();
#endif
}
